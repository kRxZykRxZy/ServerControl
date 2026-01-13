#pragma once
#include <asio.hpp>
#include <string>

using asio::ip::tcp;

std::string http_response(const std::string& body);
void handle_client(tcp::socket socket);
