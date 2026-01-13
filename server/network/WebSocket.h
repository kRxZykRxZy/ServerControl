#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <string>
#include <set>
#include <mutex>

typedef websocketpp::server<websocketpp::config::asio> websocket_server;
typedef websocket_server::message_ptr message_ptr;
using websocketpp::connection_hdl;

extern std::set<connection_hdl, std::owner_less<connection_hdl>> ws_connections;
extern std::mutex ws_connections_mtx;
extern websocket_server ws_server;

void broadcast_ws(const std::string& message);
void on_ws_open(connection_hdl hdl);
void on_ws_close(connection_hdl hdl);
void on_ws_message(websocket_server* server, connection_hdl hdl, message_ptr msg);
void run_websocket_server();
