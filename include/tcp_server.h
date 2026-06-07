#ifndef AETHER_ROUTER_TCP_SERVER_H
#define AETHER_ROUTER_TCP_SERVER_H

#include <cstdint>

#include "asio.hpp"

namespace aether_router::tcp
{

using asio::ip::tcp;

class Server
{
    public:
        Server(asio::io_context &io_context, const std::uint16_t port);
        Server(const Server &server)            = delete;
        Server(Server &&server)                 = delete;
        Server &operator=(const Server &server) = delete;
        Server &operator=(Server &&server)      = delete;

    private:
        void Run(void);
        void AcceptSession(tcp::socket socket);

        asio::io_context &io_context_;
        tcp::acceptor acceptor_;
};

} // namespace aether_router::tcp

#endif // AETHER_ROUTER_TCP_SERVER_H
