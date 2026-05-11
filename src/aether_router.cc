#include <atomic>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "aether.h"
#include "toml++/toml.hpp"

#include "serial.h"
#include "tcp_server.h"
#include "version.h"

std::atomic<bool> AetherRouter_Running(true);

void AetherRouter_SignalHandler(const int signal);

int main(const int argc, const char *const argv[])
{
    std::signal(SIGINT, AetherRouter_SignalHandler);

    std::cout << "Aether Router Info:\n"
              << "    Branch: " << AETHER_ROUTER_GIT_BRANCH << "\n"
              << "    Commit: " << AETHER_ROUTER_GIT_COMMIT_HASH << " (" << AETHER_ROUTER_GIT_DIRTY << ")\n"
              << "    Tag: " << AETHER_ROUTER_GIT_TAG << "\n";

    toml::parse_result config;

    if (2 != argc)
    {
        std::cerr << "Usage: " << argv[0U] << "<config>\n";
        return EXIT_FAILURE;
    }

    try
    {
        config = toml::parse_file(argv[1U]);
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Error parsing config file: " << exception.what() << "\n";
        return EXIT_FAILURE;
    }

    a_SetLogLevel(config["aether"]["log_level"].value_or(A_LOG_LEVEL_INFO));
    a_Err_t error = a_Initialize(A_TRANSPORT_PEER_ID_MAX);
    if (A_ERR_NONE != error)
    {
        std::cerr << "Error initializing aether: " << a_Err_ToString(error) << "\n";
        return EXIT_FAILURE;
    }

    std::vector<std::unique_ptr<aether_router::tcp::Server>> tcp_servers;
    const auto *const                                        tcp_ports = config["tcp"]["ports"].as_array();
    if (nullptr != tcp_ports)
    {
        for (auto &&tcp_port : *tcp_ports)
        {
            tcp_servers.emplace_back(std::make_unique<aether_router::tcp::Server>(static_cast<std::uint16_t>(tcp_port.as_integer()->get())));
        }
    }

    std::vector<std::unique_ptr<aether_router::serial::Port>> serial_ports;
    const auto *const                                         serial_port_configs = config["serial"].as_array();
    if (nullptr != serial_port_configs)
    {
        for (auto &&serial_port_config : *serial_port_configs)
        {
            const auto table = serial_port_config.as_table();

            serial_ports.emplace_back(
                std::make_unique<aether_router::serial::Port>(
                    table->get_as<std::string>("device")->get(),
                    static_cast<std::uint32_t>(table->get_as<int64_t>("baudrate")->get())));
        }
    }

    while (AetherRouter_Running)
    {
        for (std::unique_ptr<aether_router::tcp::Server> &tcp_server : tcp_servers)
        {
            tcp_server->Run();
        }

        for (std::unique_ptr<aether_router::serial::Port> &serial_port : serial_ports)
        {
            serial_port->Run();
        }

        a_Task();
    }

    a_Deinitialize();
    return EXIT_SUCCESS;
}

void AetherRouter_SignalHandler(const int signal)
{
    if (SIGINT == signal)
    {
        std::cout << "\nShutting down...\n";
        AetherRouter_Running = false;
    }
}
