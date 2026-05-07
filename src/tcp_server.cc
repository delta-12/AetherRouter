#include "tcp_server.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "aether.h"
#include "asio.hpp"

#include "session.h"

namespace aether_router::tcp
{

using asio::ip::tcp;
using Session = session::Session<tcp::socket>;

const char *                          kLogTag = "TCP_SERVER";
std::vector<std::unique_ptr<Session>> sessions;

static a_Err_t SessionStop(void *arg);
static std::size_t SessionSend(const std::uint8_t *const data, const std::size_t size, void *arg);
static std::size_t SessionReceive(std::uint8_t *const data, const std::size_t size, void *arg);

Server::Server(const std::uint16_t port) : acceptor_(io_, tcp::endpoint(tcp::v4(), port))
{
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.non_blocking(true);

    A_LOG_DEBUG(kLogTag, "Listening on port %u", port);
}

void Server::Run(void)
{
    asio::error_code error;
    tcp::socket      socket(io_);

    acceptor_.accept(socket, error);

    if (!error)
    {
        AcceptSession(std::move(socket));
    }
    else if ((asio::error::would_block != error) && (asio::error::try_again != error))
    {
        // TODO handle error
        A_LOG_ERROR(kLogTag, "Accept session error: %s", error.message().c_str());
    }
}

void Server::AcceptSession(tcp::socket socket)
{
    socket.non_blocking(true);

    tcp::endpoint endpoint = socket.remote_endpoint();
    A_LOG_DEBUG(kLogTag, "New connection from %s:%u", endpoint.address().to_string().c_str(), endpoint.port());

    std::unique_ptr<Session> session = std::make_unique<Session>(std::move(socket));

    a_Socket_Functions_t socket_functions = {.start = nullptr, .stop = SessionStop, .send = SessionSend, .receive = SessionReceive, .arg = session.get()};
    a_Socket_t           aether_socket;

    a_Err_t error = a_Socket_Initialize(&aether_socket,
                                        A_SOCKET_TYPE_TCP,
                                        socket_functions,
                                        session->send_buffer_,
                                        sizeof(session->send_buffer_),
                                        session->receive_buffer_,
                                        sizeof(session->receive_buffer_));

    A_LOG_VERBOSE(kLogTag, "Socket initialization error: %s", a_Err_ToString(error));

    if (A_ERR_NONE == error)
    {
        error = a_AddSocket(&aether_socket, session->message_buffer_, sizeof(session->message_buffer_), false);

        A_LOG_VERBOSE(kLogTag, "Socket add error: %s", a_Err_ToString(error));
    }

    if (A_ERR_NONE == error)
    {
        sessions.push_back(std::move(session));
    }
}

static a_Err_t SessionStop(void *arg)
{
    const Session *const removal = static_cast<Session *>(arg);

    sessions.erase(std::remove_if(sessions.begin(), sessions.end(), [removal](const std::unique_ptr<Session> &session)
    {
        return session.get() == removal;
    }),
                   sessions.end());

    return A_ERR_NONE;
}

static std::size_t SessionSend(const std::uint8_t *const data, const std::size_t size, void *arg)
{
    Session *const   session = static_cast<Session *>(arg);
    asio::error_code error;

    std::size_t sent = asio::write(session->socket_, asio::buffer(data, size), error);

    if (error)
    {
        sent = SIZE_MAX;
    }

    A_LOG_VERBOSE(kLogTag, "Session send error: %s", error.message().c_str());

    return sent;
}

static std::size_t SessionReceive(std::uint8_t *const data, const std::size_t size, void *arg)
{
    Session *const   session = static_cast<Session *>(arg);
    asio::error_code error;

    std::size_t received = session->socket_.read_some(asio::buffer(data, size), error);

    if ((asio::error::would_block == error) || (asio::error::try_again == error))
    {
        received = 0U;
    }
    else if (error)
    {
        received = SIZE_MAX;
    }

    A_LOG_VERBOSE(kLogTag, "Session receive error: %s", error.message().c_str());

    return received;
}

} // namespace aether_router::tcp
