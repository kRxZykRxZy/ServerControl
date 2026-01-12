#include "HttpClient.h"
#include <asio.hpp>
#include <sstream>

using asio::ip::tcp;

static std::string request(const std::string& req,
                           const std::string& ip, int port) {
    asio::io_context io;
    tcp::socket socket(io);
    socket.connect(tcp::endpoint(asio::ip::make_address(ip), port));
    asio::write(socket, asio::buffer(req));

    asio::streambuf buf;
    asio::read(socket, buf, asio::transfer_all());

    std::istream is(&buf);
    std::string line, body;
    while (std::getline(is, line)) {
        if (line == "\r") break;
    }
    while (std::getline(is, line))
        body += line + "\n";
    return body;
}

std::string HttpClient::post(const std::string& ip, int port,
                             const std::string& path,
                             const std::string& body) {
    std::stringstream ss;
    ss << "POST " << path << " HTTP/1.1\r\n"
       << "Host: " << ip << "\r\n"
       << "Content-Length: " << body.size() << "\r\n\r\n"
       << body;
    return request(ss.str(), ip, port);
}

std::string HttpClient::get(const std::string& ip, int port,
                            const std::string& path) {
    std::stringstream ss;
    ss << "GET " << path << " HTTP/1.1\r\n"
       << "Host: " << ip << "\r\n\r\n";
    return request(ss.str(), ip, port);
}

