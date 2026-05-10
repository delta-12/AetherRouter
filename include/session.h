#ifndef AETHER_ROUTER_SESSION_H
#define AETHER_ROUTER_SESSION_H

#include "aether.h"

namespace aether_router::session
{

template<typename T>
struct Session
{
    Session(void);
    explicit Session(T socket);
    Session(const Session &session)            = delete;
    Session(Session &&session)                 = delete;
    Session &operator=(const Session &session) = delete;
    Session &operator=(Session &&session)      = delete;
    T socket_;
    std::uint8_t send_buffer_[AETHER_TRANSPORT_MTU]    = {0U};
    std::uint8_t receive_buffer_[AETHER_TRANSPORT_MTU] = {0U};
    std::uint8_t message_buffer_[AETHER_TRANSPORT_MTU] = {0U};
};

template<typename T>
Session<T>::Session(void)
{
}

template<typename T>
Session<T>::Session(T socket) : socket_(std::move(socket))
{
}

} // namespace aether_router::session

#endif // AETHER_ROUTER_SESSION_H
