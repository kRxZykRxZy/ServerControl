#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <mutex>
#include <string>
#include <functional>

typedef websocketpp::server<websocketpp::config::asio> websocket_server;
typedef websocket_server::message_ptr message_ptr;
using websocketpp::connection_hdl;

namespace WebSocket {

// WebSocket server types
enum class ServerType {
    MAIN_CONTROL,      // Main control WebSocket
    STATS_MONITORING,  // Stats and monitoring updates
    FILE_OPERATIONS,   // File upload/download streaming
    REMOTE_DESKTOP     // Remote desktop framebuffer stream
};

// Individual WebSocket server instance
class Server {
public:
    Server(int port, ServerType type);
    ~Server();
    
    // Start the server
    void Start();
    
    // Stop the server
    void Stop();
    
    // Broadcast message to all connected clients
    void Broadcast(const std::string& message);
    
    // Send message to specific client
    void Send(connection_hdl hdl, const std::string& message);
    
    // Get connection count
    size_t GetConnectionCount() const;
    
    // Set message handler
    void SetMessageHandler(std::function<void(connection_hdl, message_ptr)> handler);
    
    int GetPort() const { return port_; }
    ServerType GetType() const { return type_; }
    
private:
    void OnOpen(connection_hdl hdl);
    void OnClose(connection_hdl hdl);
    void OnMessage(connection_hdl hdl, message_ptr msg);
    
    websocket_server server_;
    std::set<connection_hdl, std::owner_less<connection_hdl>> connections_;
    std::mutex connections_mutex_;
    int port_;
    ServerType type_;
    std::function<void(connection_hdl, message_ptr)> message_handler_;
    bool running_;
};

// Manager for all WebSocket servers
class Manager {
public:
    static Manager& Instance();
    
    // Initialize all four WebSocket servers
    void InitializeServers(int basePort);
    
    // Start all servers
    void StartAll();
    
    // Stop all servers
    void StopAll();
    
    // Get server by type
    Server* GetServer(ServerType type);
    
    // Broadcast to specific server type
    void Broadcast(ServerType type, const std::string& message);
    
    // Broadcast to all servers
    void BroadcastAll(const std::string& message);
    
private:
    Manager() = default;
    std::vector<std::unique_ptr<Server>> servers_;
};

} // namespace WebSocket
