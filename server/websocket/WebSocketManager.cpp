#include "WebSocketManager.h"
#include <iostream>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace WebSocket {

Server::Server(int port, ServerType type)
    : port_(port), type_(type), running_(false) {
}

Server::~Server() {
    Stop();
}

void Server::Start() {
    if (running_) return;
    
    try {
        server_.set_access_channels(websocketpp::log::alevel::none);
        server_.set_error_channels(websocketpp::log::elevel::none);
        
        server_.init_asio();
        server_.set_reuse_addr(true);
        
        server_.set_open_handler([this](connection_hdl hdl) { OnOpen(hdl); });
        server_.set_close_handler([this](connection_hdl hdl) { OnClose(hdl); });
        server_.set_message_handler([this](connection_hdl hdl, message_ptr msg) { OnMessage(hdl, msg); });
        
        // Try to listen on the configured port
        bool listening = false;
        int attempt_port = port_;
        for (int attempt = 0; attempt < 10 && !listening; attempt++) {
            try {
                server_.listen(attempt_port);
                server_.start_accept();
                listening = true;
                port_ = attempt_port;
                
                const char* type_name = "Unknown";
                switch (type_) {
                    case ServerType::MAIN_CONTROL: type_name = "Main Control"; break;
                    case ServerType::STATS_MONITORING: type_name = "Stats/Monitoring"; break;
                    case ServerType::FILE_OPERATIONS: type_name = "File Operations"; break;
                    case ServerType::REMOTE_DESKTOP: type_name = "Remote Desktop"; break;
                }
                
                std::cout << "WebSocket server (" << type_name << ") started on port " << port_ << "\n";
            } catch (std::exception& e) {
                if (attempt < 9) {
                    attempt_port++;
                } else {
                    throw;
                }
            }
        }
        
        running_ = true;
        
        // Run in separate thread
        std::thread([this]() {
            try {
                server_.run();
            } catch (std::exception& e) {
                std::cerr << "WebSocket server error: " << e.what() << std::endl;
            }
        }).detach();
        
    } catch (std::exception& e) {
        std::cerr << "Failed to start WebSocket server: " << e.what() << std::endl;
    }
}

void Server::Stop() {
    if (!running_) return;
    
    try {
        server_.stop_listening();
        
        // Close all connections
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto hdl : connections_) {
            try {
                server_.close(hdl, websocketpp::close::status::going_away, "Server shutdown");
            } catch (...) {}
        }
        connections_.clear();
        
        server_.stop();
        running_ = false;
    } catch (...) {}
}

void Server::Broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    for (auto hdl : connections_) {
        try {
            server_.send(hdl, message, websocketpp::frame::opcode::text);
        } catch (...) {
            // Connection closed, will be cleaned up later
        }
    }
}

void Server::Send(connection_hdl hdl, const std::string& message) {
    try {
        server_.send(hdl, message, websocketpp::frame::opcode::text);
    } catch (...) {}
}

size_t Server::GetConnectionCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(connections_mutex_));
    return connections_.size();
}

void Server::SetMessageHandler(std::function<void(connection_hdl, message_ptr)> handler) {
    message_handler_ = handler;
}

void Server::OnOpen(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.insert(hdl);
    
    const char* type_name = "Unknown";
    switch (type_) {
        case ServerType::MAIN_CONTROL: type_name = "Main Control"; break;
        case ServerType::STATS_MONITORING: type_name = "Stats/Monitoring"; break;
        case ServerType::FILE_OPERATIONS: type_name = "File Operations"; break;
        case ServerType::REMOTE_DESKTOP: type_name = "Remote Desktop"; break;
    }
    
    std::cout << "WebSocket client connected to " << type_name << ". Total clients: " << connections_.size() << std::endl;
}

void Server::OnClose(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(hdl);
    
    const char* type_name = "Unknown";
    switch (type_) {
        case ServerType::MAIN_CONTROL: type_name = "Main Control"; break;
        case ServerType::STATS_MONITORING: type_name = "Stats/Monitoring"; break;
        case ServerType::FILE_OPERATIONS: type_name = "File Operations"; break;
        case ServerType::REMOTE_DESKTOP: type_name = "Remote Desktop"; break;
    }
    
    std::cout << "WebSocket client disconnected from " << type_name << ". Total clients: " << connections_.size() << std::endl;
}

void Server::OnMessage(connection_hdl hdl, message_ptr msg) {
    if (message_handler_) {
        message_handler_(hdl, msg);
    } else {
        // Default ping/pong handler
        try {
            json j = json::parse(msg->get_payload());
            std::string type = j["type"];
            
            if (type == "ping") {
                json pong = {
                    {"type", "pong"}, 
                    {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count()}
                };
                Send(hdl, pong.dump());
            }
        } catch (...) {
            // Invalid message, ignore
        }
    }
}

// Manager implementation
Manager& Manager::Instance() {
    static Manager instance;
    return instance;
}

void Manager::InitializeServers(int basePort) {
    servers_.clear();
    
    servers_.push_back(std::make_unique<Server>(basePort, ServerType::MAIN_CONTROL));
    servers_.push_back(std::make_unique<Server>(basePort + 1, ServerType::STATS_MONITORING));
    servers_.push_back(std::make_unique<Server>(basePort + 2, ServerType::FILE_OPERATIONS));
    servers_.push_back(std::make_unique<Server>(basePort + 3, ServerType::REMOTE_DESKTOP));
}

void Manager::StartAll() {
    for (auto& server : servers_) {
        server->Start();
    }
}

void Manager::StopAll() {
    for (auto& server : servers_) {
        server->Stop();
    }
}

Server* Manager::GetServer(ServerType type) {
    for (auto& server : servers_) {
        if (server->GetType() == type) {
            return server.get();
        }
    }
    return nullptr;
}

void Manager::Broadcast(ServerType type, const std::string& message) {
    Server* server = GetServer(type);
    if (server) {
        server->Broadcast(message);
    }
}

void Manager::BroadcastAll(const std::string& message) {
    for (auto& server : servers_) {
        server->Broadcast(message);
    }
}

} // namespace WebSocket
