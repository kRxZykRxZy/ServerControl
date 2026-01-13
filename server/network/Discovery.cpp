#include "Discovery.h"
#include "../core/Utils.h"
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

using asio::ip::udp;
using json = nlohmann::json;

extern int http_port;
extern int discovery_port;
extern int websocket_port;

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
                    {"hostname", get_hostname()},
                    {"ip", get_local_ip()},
                    {"port", http_port},
                    {"ws_port", websocket_port},
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
