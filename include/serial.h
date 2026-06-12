#ifndef AETHER_ROUTER_SERIAL_H
#define AETHER_ROUTER_SERIAL_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "session.h"

namespace aether_router::serial
{

// TODO handle port close
// TODO remove socket associated with serial port here if applicable
class Port
{
    public:
        Port(const std::string &device, const std::uint32_t baud_rate);
        Port(const Port &port)            = delete;
        Port(Port &&port)                 = delete;
        Port &operator=(const Port &port) = delete;
        Port &operator=(Port &&port)      = delete;
        ~Port();
        void Reset(void);
        std::size_t Read(std::uint8_t *const buffer, const std::size_t size);
        std::size_t Write(const std::uint8_t *const buffer, const std::size_t size);

    private:
        std::string device_;
        session::Session<int> session_;
};

} // namespace aether_router::serial

#endif // AETHER_ROUTER_SERIAL_H
