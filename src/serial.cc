#include "serial.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "aether.h"
#include "asio.hpp"

namespace aether_router::serial
{

const char *kLogTag = "SERIAL";

static std::size_t SessionSend(const std::uint8_t *const data, const std::size_t size, void *arg);
static std::size_t SessionReceive(std::uint8_t *const data, const std::size_t size, void *arg);

Port::Port(asio::io_context &io_context, const std::string &device, const uint32_t baud_rate) : io_context_(io_context), serial_port_(io_context_, device)
{
    serial_port_.set_option(asio::serial_port_base::baud_rate(baud_rate));
    serial_port_.set_option(asio::serial_port_base::character_size(8));
    serial_port_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    serial_port_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
    serial_port_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));

    AsyncRead();

    A_LOG_DEBUG(kLogTag, "Listening on device %s with baudrate %u", device.c_str(), baud_rate);

    a_SocketFunctions_t aether_socket_functions = {.start = nullptr, .stop = nullptr, .send = SessionSend, .receive = SessionReceive, .arg = this};
    a_Socket_t          aether_socket;
    a_Session_t         aether_session;

    a_Err_t error = a_InitializeSocket(&aether_socket,
                                       A_SOCKET_TYPE_SERIAL,
                                       aether_socket_functions,
                                       session_.send_buffer_,
                                       sizeof(session_.send_buffer_),
                                       session_.receive_buffer_,
                                       sizeof(session_.receive_buffer_));

    A_LOG_VERBOSE(kLogTag, "Socket initialization error: %s", a_Err_ToString(error));

    if (A_ERR_NONE == error)
    {
        error = a_AddSession(&aether_session, &aether_socket, session_.message_buffer_, sizeof(session_.message_buffer_), true);

        A_LOG_VERBOSE(kLogTag, "Session add error: %s", a_Err_ToString(error));
    }
}

Port::~Port()
{
    if (serial_port_.is_open())
    {
        serial_port_.cancel();
        serial_port_.close();
    }
}

std::size_t Port::Read(std::uint8_t *const buffer, const std::size_t size)
{
    std::size_t read_size = size;

    if (read_size > read_queue_.size())
    {
        read_size = read_queue_.size();
    }

    (void)std::copy(read_queue_.begin(), read_queue_.begin() + read_size, buffer);
    (void)read_queue_.erase(read_queue_.begin(), read_queue_.begin() + read_size);

    return read_size;
}

std::size_t Port::Write(const std::uint8_t *const buffer, const std::size_t size)
{
    (void)write_queue_.insert(write_queue_.end(), buffer, buffer + size);

    if (!writing_)
    {
        AsyncWrite();
    }

    return size;
}

void Port::AsyncRead(void)
{
    (void)serial_port_.async_read_some(
        asio::buffer(read_buffer_),
        [this](const asio::error_code &error, const std::size_t size)
    {
        if (asio::error::operation_aborted == error)
        {
            // Async operation was cancelled on purpose
        }
        else if (error)
        {
            A_LOG_ERROR(kLogTag, "Failed to read data with error %s", error.message().c_str());
        }
        else
        {
            read_queue_.insert(read_queue_.end(), read_buffer_.begin(), read_buffer_.begin() + size);
            AsyncRead();
        }
    });
}

void Port::AsyncWrite(void)
{
    if (!write_queue_.empty() && !writing_)
    {
        writing_ = true;

        write_buffer_.assign(write_queue_.begin(), write_queue_.end());
        write_queue_.clear();

        asio::async_write(serial_port_,
                          asio::buffer(write_buffer_),
                          [this](const asio::error_code &error, const std::size_t size)
        {
            A_UNUSED(size);

            if (asio::error::operation_aborted == error)
            {
                // Async operation was cancelled on purpose
            }
            else if (error)
            {
                A_LOG_ERROR(kLogTag, "Failed to write data with error %s", error.message().c_str());
            }
            else if (!write_queue_.empty())
            {
                AsyncWrite();
            }
            else
            {
                writing_ = false;
            }
        });
    }
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
