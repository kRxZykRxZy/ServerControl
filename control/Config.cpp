#include "Config.h"
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <set>
#include <chrono>

using asio::ip::udp;
using json = nlohmann::json;

std::vector<ServerConfig> Config::load() {
    std::vector<ServerConfig> servers;
    std::set<std::string> discovered_ips;
    
    try {
        asio::io_context io;
        udp::socket socket(io, udp::endpoint(udp::v4(), 0));
        socket.set_option(asio::socket_base::broadcast(true));
        
        // Send discovery broadcast
        std::string discover_msg = "DISCOVER_SERVER";
        udp::endpoint broadcast_endpoint(asio::ip::make_address_v4("255.255.255.255"), 8081);
        
        // Try multiple times to discover servers
        for (int i = 0; i < 3; i++) {
            socket.send_to(asio::buffer(discover_msg), broadcast_endpoint);
            
            // Wait for responses with timeout
            socket.non_blocking(true);
            auto start = std::chrono::steady_clock::now();
            
            while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
                try {
                    char recv_buf[1024];
                    udp::endpoint sender_endpoint;
                    size_t len = socket.receive_from(asio::buffer(recv_buf), sender_endpoint);
                    
                    if (len > 0) {
                        std::string response(recv_buf, len);
                        auto j = json::parse(response);
                        
                        if (j.contains("type") && j["type"] == "SERVER_RESPONSE") {
                            std::string ip = sender_endpoint.address().to_string();
                            
                            // Avoid duplicates
                            if (discovered_ips.find(ip) == discovered_ips.end()) {
                                discovered_ips.insert(ip);
                                
                                std::string hostname = j.value("hostname", "unknown");
                                int port = j.value("port", 8080);
                                
                                servers.push_back({hostname, ip, port});
                                std::cout << "Discovered server: " << hostname << " at " << ip << ":" << port << "\n";
                            }
                        }
                    }
                } catch (asio::system_error& e) {
                    // Socket would block - no data available, continue waiting
                    if (e.code() != asio::error::would_block) {
                        std::cerr << "Socket error during discovery: " << e.what() << "\n";
                    }
                } catch (json::exception& e) {
                    std::cerr << "JSON parse error: " << e.what() << "\n";
                }
            }
        }
        
    } catch (std::exception& e) {
        std::cerr << "Discovery error: " << e.what() << "\n";
    }
    
    // If no servers discovered, fall back to manual configuration
    if (servers.empty()) {
        std::cout << "No servers discovered, using default configuration\n";
        servers = {
            {"server01", "127.0.0.1", 8080},
        };
    }
    
    return servers;
}

