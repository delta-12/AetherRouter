#ifndef AETHER_ROUTER_TCP_SERVER_H
#define AETHER_ROUTER_TCP_SERVER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "asio.hpp"

namespace aether_router::tcp
{

using asio::ip::tcp;

class Server
{
    public:
        Server(const std::uint16_t port);
        Server(const Server &server)            = delete;
        Server(Server &&server)                 = delete;
        Server &operator=(const Server &server) = delete;
        Server &operator=(Server &&server)      = delete;
        void Run(void);

    private:
        void AcceptSession(tcp::socket socket);

        asio::io_context io_;
        tcp::acceptor acceptor_;
};

} // namespace aether_router

#endif // AETHER_ROUTER_TCP_SERVER_H
