#include "serial.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <system_error>

#include "aether.h"

namespace aether_router::serial
{

const char *kLogTag = "SERIAL";

static speed_t BaudRateToSpeed(const std::uint32_t baud_rate);
static a_Err_t SessionStop(void *arg);
static std::size_t SessionSend(const std::uint8_t *const data, const std::size_t size, void *arg);
static std::size_t SessionReceive(std::uint8_t *const data, const std::size_t size, void *arg);

Port::Port(const std::string &device, const std::uint32_t baud_rate) : device_(device)
{
    struct termios tty;

    session_.socket = open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (session_.socket < 0)
    {
        throw std::system_error(errno, std::generic_category(), "Failed to open device " + device_);
    }

    if (0 != tcgetattr(session_.socket, &tty))
    {
        close(session_.socket);
        throw std::system_error(errno, std::generic_category(), "Failed to get attributes for device " + device_);
    }

    cfmakeraw(&tty);

    // 8-bit character size
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No parity
    tty.c_cflag &= ~PARENB;
    tty.c_iflag &= ~INPCK;

    // One stop bit
    tty.c_cflag &= ~CSTOPB;

    // Disable both hardware and software flow control
    tty.c_cflag &= ~CRTSCTS;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Ignores modem control lines
    tty.c_cflag |= (CREAD | CLOCAL);

    // Read returns immediately with however many bytes are in the buffer
    // Consistent with O_NONBLOCK
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    const speed_t speed = BaudRateToSpeed(baud_rate);
    if (B0 == speed)
    {
        close(session_.socket);
        throw std::invalid_argument("Unsupported baud rate " + std::to_string(baud_rate));
    }
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    if (0 != tcsetattr(session_.socket, TCSANOW, &tty))
    {
        close(session_.socket);
        throw std::system_error(errno, std::generic_category(), "Failed to get attributes for device " + device_);
    }

    Reset();

    A_LOG_DEBUG(kLogTag, "Listening on device %s with baudrate %u", device_.c_str(), baud_rate);

    const a_SocketFunctions_t aether_socket_functions = {.start = nullptr, .stop = SessionStop, .send = SessionSend, .receive = SessionReceive, .arg = this};
    a_Socket_t                aether_socket;
    a_Session_t               aether_session;

    a_Err_t error = a_InitializeSocket(&aether_socket,
                                       A_SOCKET_TYPE_SERIAL,
                                       aether_socket_functions,
                                       session_.send_buffer,
                                       sizeof(session_.send_buffer),
                                       session_.receive_buffer,
                                       sizeof(session_.receive_buffer));

    A_LOG_VERBOSE(kLogTag, "Socket initialization error: %s", a_Err_ToString(error));

    if (A_ERR_NONE == error)
    {
        error = a_AddSession(&aether_session, &aether_socket, session_.message_buffer, sizeof(session_.message_buffer), true);

        A_LOG_VERBOSE(kLogTag, "Session add error: %s", a_Err_ToString(error));
    }
}

Port::~Port()
{
    if (session_.socket >= 0)
    {
        Reset();
        close(session_.socket);

        session_.socket = -1;

        A_LOG_DEBUG(kLogTag, "Closed device %s", device_.c_str());
    }
}

void Port::Reset(void)
{
    (void)tcdrain(session_.socket);
    (void)tcflush(session_.socket, TCIOFLUSH);

    A_LOG_VERBOSE(kLogTag, "Reset device %s", device_.c_str());
}

std::size_t Port::Read(std::uint8_t *const buffer, const std::size_t size)
{
    const ssize_t bytes_read = read(session_.socket, buffer, size);
    std::size_t   bytes      = SIZE_MAX;

    if (bytes_read >= 0)
    {
        bytes = static_cast<std::size_t>(bytes_read);
    }
    else if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
    {
        bytes = 0U;
    }

    return bytes;
}

std::size_t Port::Write(const std::uint8_t *const buffer, const std::size_t size)
{
    const ssize_t bytes_written = write(session_.socket, buffer, size);
    std::size_t   bytes         = SIZE_MAX;

    if (bytes_written >= 0)
    {
        bytes = static_cast<std::size_t>(bytes_written);
    }

    return bytes;
}

static speed_t BaudRateToSpeed(const std::uint32_t baud_rate)
{
    speed_t speed = B0;

    switch (baud_rate)
    {
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    case 230400:
        speed = B230400;
        break;
    case 460800:
        speed = B460800;
        break;
    case 500000:
        speed = B500000;
        break;
    case 576000:
        speed = B576000;
        break;
    case 921600:
        speed = B921600;
        break;
    case 1000000:
        speed = B1000000;
        break;
    case 1152000:
        speed = B1152000;
        break;
    case 1500000:
        speed = B1500000;
        break;
    case 2000000:
        speed = B2000000;
        break;
    default:
        break;
    }

    return speed;
}

static a_Err_t SessionStop(void *arg)
{
    Port *const port = static_cast<Port *>(arg);

    port->Reset();

    return A_ERR_NONE;
}

static std::size_t SessionSend(const std::uint8_t *const data, const std::size_t size, void *arg)
{
    Port *const port = static_cast<Port *>(arg);

    return port->Write(data, size);
}

static std::size_t SessionReceive(std::uint8_t *const data, const std::size_t size, void *arg)
{
    Port *const port = static_cast<Port *>(arg);

    return port->Read(data, size);
}

} // namespace aether_router::serial
