#include "WebSocket.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <functional>

using json = nlohmann::json;

std::set<connection_hdl, std::owner_less<connection_hdl>> ws_connections;
std::mutex ws_connections_mtx;
websocket_server ws_server;

extern int websocket_port;

void broadcast_ws(const std::string& message) {
    std::lock_guard<std::mutex> lock(ws_connections_mtx);
    for (auto hdl : ws_connections) {
        try {
            ws_server.send(hdl, message, websocketpp::frame::opcode::text);
        } catch (...) {
        }
    }
}

void on_ws_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(ws_connections_mtx);
    ws_connections.insert(hdl);
    std::cout << "WebSocket client connected. Total clients: " << ws_connections.size() << std::endl;
}

void on_ws_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(ws_connections_mtx);
    ws_connections.erase(hdl);
    std::cout << "WebSocket client disconnected. Total clients: " << ws_connections.size() << std::endl;
}

void on_ws_message(websocket_server* server, connection_hdl hdl, message_ptr msg) {
    try {
        json j = json::parse(msg->get_payload());
        std::string type = j["type"];
        
        if (type == "ping") {
            json pong = {{"type", "pong"}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()}};
            server->send(hdl, pong.dump(), websocketpp::frame::opcode::text);
        }
    } catch (...) {
    }
}

void run_websocket_server() {
    try {
        ws_server.set_access_channels(websocketpp::log::alevel::none);
        ws_server.set_error_channels(websocketpp::log::elevel::none);
        
        ws_server.init_asio();
        ws_server.set_reuse_addr(true);
        
        ws_server.set_open_handler(&on_ws_open);
        ws_server.set_close_handler(&on_ws_close);
        ws_server.set_message_handler(std::bind(&on_ws_message, &ws_server, std::placeholders::_1, std::placeholders::_2));
        
        bool listening = false;
        for (int attempt = 0; attempt < 10 && !listening; attempt++) {
            try {
                ws_server.listen(websocket_port);
                ws_server.start_accept();
                listening = true;
                std::cout << "WebSocket server started on port " << websocket_port << "\n";
            } catch (std::exception& e) {
                if (attempt < 9) {
                    websocket_port++;
                    std::cout << "Port in use, trying port " << websocket_port << "\n";
                } else {
                    throw;
                }
            }
        }
        
        ws_server.run();
    } catch (std::exception& e) {
        std::cerr << "WebSocket server error: " << e.what() << std::endl;
    }
}
