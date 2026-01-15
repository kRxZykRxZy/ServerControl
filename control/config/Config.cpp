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
        
        // Send discovery broadcast to general network
        std::string discover_msg = "DISCOVER_SERVER";
        udp::endpoint broadcast_endpoint(asio::ip::make_address_v4("255.255.255.255"), 2031); // Updated to 2031
        
        std::cout << "Scanning for servers on network...\n";
        
        // Scan 10.125.125.x subnet specifically
        std::cout << "Scanning 10.125.125.x subnet...\n";
        for (int host = 1; host < 255; host++) {
            std::string target_ip = "10.125.125." + std::to_string(host);
            try {
                udp::endpoint target_endpoint(asio::ip::make_address_v4(target_ip), 2031);
                socket.send_to(asio::buffer(discover_msg), target_endpoint);
            } catch (...) {
                // Skip invalid IPs
            }
        }
        
        // Try multiple times to discover servers via broadcast
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
                                int port = j.value("port", 2030);
                                int ws_main = j.value("ws_main", 2040);
                                int ws_stats = j.value("ws_stats", 2041);
                                int ws_files = j.value("ws_files", 2042);
                                int ws_desktop = j.value("ws_desktop", 2043);
                                
                                ServerConfig cfg;
                                cfg.name = hostname;
                                cfg.ip = ip;
                                cfg.port = port;
                                cfg.ws_main_port = ws_main;
                                cfg.ws_stats_port = ws_stats;
                                cfg.ws_files_port = ws_files;
                                cfg.ws_desktop_port = ws_desktop;
                                
                                servers.push_back(cfg);
                                std::cout << "Discovered server: " << hostname << " at " << ip << ":" << port 
                                          << " (WS: " << ws_main << "," << ws_stats << "," << ws_files << "," << ws_desktop << ")\n";
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
        ServerConfig cfg;
        cfg.name = "server01";
        cfg.ip = "127.0.0.1";
        cfg.port = 2030;
        cfg.ws_main_port = 2040;
        cfg.ws_stats_port = 2041;
        cfg.ws_files_port = 2042;
        cfg.ws_desktop_port = 2043;
        servers.push_back(cfg);
    }
    
    return servers;
}

