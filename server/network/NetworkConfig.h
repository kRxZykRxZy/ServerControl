#pragma once
#include <string>
#include <vector>

// Network configuration for IP-based server deployment
class NetworkConfig {
public:
    static NetworkConfig& getInstance();
    
    // Get available network interfaces and their IPs
    std::vector<std::string> getAvailableIPs();
    
    // Select an IP to bind to (from a specific subnet if specified)
    std::string selectBindIP(const std::string& subnet_prefix = "");
    
    // Check if an IP is available on this machine
    bool isIPAvailable(const std::string& ip);
    
    // Get the current bind IP
    std::string getBindIP() const { return bind_ip; }
    
    // Set the bind IP
    void setBindIP(const std::string& ip) { bind_ip = ip; }
    
    // Standard port for all servers
    static const int STANDARD_PORT = 2030;
    
private:
    NetworkConfig() : bind_ip("0.0.0.0") {}
    std::string bind_ip;
};
