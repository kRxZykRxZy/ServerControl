#include <asio.hpp>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <atomic>
#include <cstdlib>
#include <csignal>
#include <nlohmann/json.hpp>
#include <chrono>
#include <unistd.h>
#include <filesystem>
#include <algorithm>

// Include our modular components
#include "tasks/Task.h"
#include "monitoring/Stats.h"
#include "network/WebSocket.h"
#include "core/Utils.h"
#include "system/RemoteDesktop.h"

using asio::ip::tcp;
using asio::ip::udp;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Global configuration
int http_port = 8080;
int discovery_port = 8081;
int websocket_port = 8082;

// Task management
std::map<std::string, Task> tasks;
std::mutex tasks_mtx;
int task_counter = 0;

// Remote desktop instance
RemoteDesktop* remote_desktop = nullptr;
std::mutex remote_mtx;

// Forward declarations
std::string http_response(const std::string& body);
void handle_client(tcp::socket socket);
void discovery_responder();

int main(){
    try{
        std::cout << "=== ServerControl 2050 - Modular Edition ===" << std::endl;
        std::cout << "Auto-detecting available ports..." << std::endl;
        
        // Find available ports
        http_port = find_available_port(8080);
        if (http_port == -1) {
            std::cerr << "ERROR: Could not find available port for HTTP server" << std::endl;
            return 1;
        }
        
        int temp_discovery = find_available_port(8081);
        if (temp_discovery == http_port) {
            temp_discovery = find_available_port(http_port + 1);
        }
        discovery_port = (temp_discovery != -1) ? temp_discovery : http_port + 1;
        
        int temp_websocket = find_available_port(8082);
        if (temp_websocket == http_port || temp_websocket == discovery_port) {
            temp_websocket = find_available_port(std::max(http_port, discovery_port) + 1);
        }
        websocket_port = (temp_websocket != -1) ? temp_websocket : std::max(http_port, discovery_port) + 1;
        
        std::cout << "\n✓ Ports allocated:" << std::endl;
        std::cout << "  HTTP API:       0.0.0.0:" << http_port << std::endl;
        std::cout << "  UDP Discovery:  0.0.0.0:" << discovery_port << std::endl;
        std::cout << "  WebSocket:      0.0.0.0:" << websocket_port << std::endl;
        std::cout << "\n";
        
        // Initialize remote desktop
        remote_desktop = new RemoteDesktop();
        
        // Start discovery responder in background
        std::thread(discovery_responder).detach();
        
        // Start CPU monitoring thread
        std::thread(cpu_monitor).detach();
        
        // Start WebSocket server in background
        std::thread(run_websocket_server).detach();
        
        // Give background threads time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), http_port));
        
        std::cout << "✓ All services started successfully!" << std::endl;
        std::cout << "✓ CPU monitoring active - will alert when CPU > 90%" << std::endl;
        std::cout << "✓ Remote desktop ready" << std::endl;
        std::cout << "✓ Server ready on hostname: " << get_hostname() << std::endl;
        std::cout << "\nWaiting for connections..." << std::endl;
        
        for(;;){
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::thread(handle_client,std::move(socket)).detach();
        }
    }catch(std::exception& e){ 
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}

// Include the rest of the server implementation from server.cpp
// This would normally be split into separate compilation units
