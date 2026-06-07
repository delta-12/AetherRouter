#ifndef AETHER_ROUTER_SERIAL_H
#define AETHER_ROUTER_SERIAL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "aether.h"
#include "asio.hpp"

#include "session.h"

namespace aether_router::serial
{

// TODO handle port close
// TODO remove socket associated with serial port here if applicable
class Port
{
    public:
        Port(asio::io_context &io_context, const std::string& device, const uint32_t baud_rate);
        Port(Port &port)                  = delete;
        Port(Port &&port)                 = delete;
        Port &operator=(const Port &port) = delete;
        Port &operator=(Port &&port)      = delete;
        ~Port();
        std::size_t Read(std::uint8_t *const buffer, const std::size_t size);
        std::size_t Write(const std::uint8_t *const buffer, const std::size_t size);

    private:
        void AsyncRead(void);
        void AsyncWrite(void);

        asio::io_context &io_context_;
        asio::serial_port serial_port_;
        session::Session<void *> session_;
        std::array<std::uint8_t, AETHER_TRANSPORT_MTU> read_buffer_;
        std::deque<std::uint8_t> read_queue_;
        std::vector<std::uint8_t> write_buffer_;
        std::deque<std::uint8_t> write_queue_;
        bool writing_ = false;
};

} // namespace aether_router::serial

#endif // AETHER_ROUTER_SERIAL_H
