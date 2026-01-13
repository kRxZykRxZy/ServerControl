#include "HttpClient.h"
#include <asio.hpp>
#include <sstream>

using asio::ip::tcp;

static std::string request(const std::string& req,
                           const std::string& ip, int port) {
    try {
        asio::io_context io;
        tcp::socket socket(io);
        socket.connect(tcp::endpoint(asio::ip::make_address(ip), port));
        asio::write(socket, asio::buffer(req));

        asio::streambuf buf;
        asio::error_code ec;
        
        // Read until end of headers
        asio::read_until(socket, buf, "\r\n\r\n", ec);
        
        // Read the response body
        std::istream is(&buf);
        std::string line, body;
        
        // Skip headers
        while (std::getline(is, line) && line != "\r") {}
        
        // Read remaining content from buffer
        std::ostringstream ss;
        ss << &buf;
        body = ss.str();
        
        // Try to read more if socket is still open
        asio::read(socket, buf, asio::transfer_all(), ec);
        if (!ec || ec == asio::error::eof) {
            ss.str("");
            ss << &buf;
            body += ss.str();
        }
        
        return body;
    } catch (...) {
        return "{}";
    }
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

