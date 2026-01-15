#include "platform/PlatformAbstraction.h"
#include "websocket/WebSocketManager.h"
#include "stats/StatsMonitor.h"
#include "tasks/TaskExecutor.h"
#include <asio.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using asio::ip::tcp;
using asio::ip::udp;
using json = nlohmann::json;

// Global configuration
const int BASE_PORT = 2030;
const std::string SUBNET = "10.125.125.";
int http_port = BASE_PORT;
int discovery_port = BASE_PORT + 1;
int websocket_base_port = BASE_PORT + 10; // 2040-2043 for 4 WebSocket servers

// Forward declarations
void discovery_responder();
void run_http_server();

int main() {
    std::cout << "=== ServerControl 2050 - Modular Windows/Linux Server ===" << std::endl;
    std::cout << "Detecting network configuration..." << std::endl;
    
#ifdef _WIN32
    std::cout << "Platform: Windows" << std::endl;
    
    // Try to assign 10.125.125.x IP address on Windows
    std::string available_ip = Platform::API::FindAvailableIP(SUBNET, 10, 254);
    if (!available_ip.empty()) {
        std::cout << "Found available IP: " << available_ip << std::endl;
        
        // Try to assign the IP
        if (Platform::API::AssignIPAddress(available_ip, "255.255.255.0")) {
            std::cout << "✓ Assigned IP address: " << available_ip << std::endl;
        } else {
            std::cout << "⚠ Failed to assign IP address (requires admin privileges)" << std::endl;
            std::cout << "  Please run: netsh interface ip add address \"Ethernet\" " 
                      << available_ip << " 255.255.255.0" << std::endl;
        }
    } else {
        std::cout << "⚠ No available IP in " << SUBNET << "x range" << std::endl;
    }
#else
    std::cout << "Platform: Linux" << std::endl;
#endif
    
    // Get local IP
    std::string bind_ip = Platform::API::GetLocalIP();
    std::string hostname = Platform::API::GetHostname();
    
    std::cout << "\n✓ Network Configuration:" << std::endl;
    std::cout << "  Hostname:       " << hostname << std::endl;
    std::cout << "  Bind IP:        " << bind_ip << std::endl;
    std::cout << "  HTTP API:       " << bind_ip << ":" << http_port << std::endl;
    std::cout << "  UDP Discovery:  " << bind_ip << ":" << discovery_port << std::endl;
    
    // Initialize WebSocket Manager with 4 servers
    std::cout << "\nInitializing WebSocket servers..." << std::endl;
    WebSocket::Manager::Instance().InitializeServers(websocket_base_port);
    WebSocket::Manager::Instance().StartAll();
    
    std::cout << "  Main Control:   " << bind_ip << ":" << websocket_base_port << std::endl;
    std::cout << "  Stats/Monitor:  " << bind_ip << ":" << (websocket_base_port + 1) << std::endl;
    std::cout << "  File Ops:       " << bind_ip << ":" << (websocket_base_port + 2) << std::endl;
    std::cout << "  Remote Desktop: " << bind_ip << ":" << (websocket_base_port + 3) << std::endl;
    
    // Initialize Stats Monitor
    std::cout << "\nStarting stats monitor..." << std::endl;
    Stats::Monitor::Instance().SetStatsCallback([hostname](const Platform::SystemStats& stats) {
        // Broadcast stats to monitoring WebSocket
        json stats_msg = {
            {"type", "stats_update"},
            {"hostname", hostname},
            {"cpu", stats.cpu},
            {"ram_used", stats.ram_used},
            {"ram_total", stats.ram_total},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()}
        };
        WebSocket::Manager::Instance().Broadcast(WebSocket::ServerType::STATS_MONITORING, stats_msg.dump());
    });
    
    Stats::Monitor::Instance().SetAlertCallback([hostname](const Platform::SystemStats& stats, const std::string& message) {
        json alert_msg = {
            {"type", "cpu_alert"},
            {"cpu", stats.cpu},
            {"hostname", hostname},
            {"message", message},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()}
        };
        // Broadcast alert to both main control and stats monitoring
        WebSocket::Manager::Instance().Broadcast(WebSocket::ServerType::MAIN_CONTROL, alert_msg.dump());
        WebSocket::Manager::Instance().Broadcast(WebSocket::ServerType::STATS_MONITORING, alert_msg.dump());
    }, 90.0);
    
    Stats::Monitor::Instance().Start(1); // Update every 1 second
    
    // Initialize Task Manager
    Tasks::Manager::Instance().SetOutputCallback([](const std::string& taskId, const std::string& output) {
        json output_msg = {
            {"type", "task_output"},
            {"task_id", taskId},
            {"output", output},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()}
        };
        WebSocket::Manager::Instance().Broadcast(WebSocket::ServerType::MAIN_CONTROL, output_msg.dump());
    });
    
    Tasks::Manager::Instance().SetCompletionCallback([](const std::string& taskId, int exitCode) {
        json complete_msg = {
            {"type", "task_complete"},
            {"task_id", taskId},
            {"exit_code", exitCode}
        };
        WebSocket::Manager::Instance().Broadcast(WebSocket::ServerType::MAIN_CONTROL, complete_msg.dump());
    });
    
    // Start discovery responder in background
    std::thread(discovery_responder).detach();
    
    // Give background threads time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\n✓ All services started successfully!" << std::endl;
    std::cout << "✓ CPU monitoring active - will alert when CPU > 90%" << std::endl;
    std::cout << "✓ Server bound to: " << bind_ip << ":" << http_port << std::endl;
    std::cout << "\nWaiting for connections..." << std::endl;
    
    // Run HTTP server (simplified for now - full implementation would be in separate module)
    run_http_server();
    
    return 0;
}

void discovery_responder() {
    try {
        asio::io_context io;
        udp::socket socket(io, udp::endpoint(udp::v4(), discovery_port));
        
        std::cout << "UDP Discovery listening on port " << discovery_port << "\n";
        
        for(;;) {
            char recv_buf[128];
            udp::endpoint remote_endpoint;
            size_t len = socket.receive_from(asio::buffer(recv_buf), remote_endpoint);
            
            std::string msg(recv_buf, len);
            if (msg == "DISCOVER_SERVER") {
                json response = {
                    {"type", "SERVER_RESPONSE"},
                    {"hostname", Platform::API::GetHostname()},
                    {"port", http_port},
                    {"ws_main", websocket_base_port},
                    {"ws_stats", websocket_base_port + 1},
                    {"ws_files", websocket_base_port + 2},
                    {"ws_desktop", websocket_base_port + 3},
                    {"discovery_port", discovery_port}
                };
                std::string resp_str = response.dump();
                socket.send_to(asio::buffer(resp_str), remote_endpoint);
            }
        }
    } catch(std::exception& e) {
        std::cerr << "Discovery error: " << e.what() << "\n";
    }
}

void run_http_server() {
    try {
        asio::io_context io;
        asio::ip::address addr = asio::ip::make_address(Platform::API::GetLocalIP());
        tcp::endpoint endpoint(addr, http_port);
        tcp::acceptor acceptor(io, endpoint);
        
        for(;;) {
            tcp::socket socket(io);
            acceptor.accept(socket);
            
            // Simplified HTTP handler - full implementation would use HTTP module
            std::thread([](tcp::socket sock) {
                try {
                    // Read request
                    asio::streambuf buf;
                    asio::read_until(sock, buf, "\r\n\r\n");
                    
                    // Send simple response
                    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"ok\"}";
                    asio::write(sock, asio::buffer(response));
                } catch(...) {}
            }, std::move(socket)).detach();
        }
    } catch(std::exception& e) {
        std::cerr << "HTTP server error: " << e.what() << std::endl;
    }
}
