#pragma once
#include <string>

// Helper functions
bool ends_with(const std::string& str, const std::string& suffix);
std::string sanitize_filename(const std::string& filename);
std::string get_hostname();
std::string base64_encode(const std::string& input);
std::string base64_decode(const std::string& input);
int find_available_port(int start_port, int max_attempts = 10);

// Helper to get local IP address
std::string get_local_ip();
