#include "NetworkConfig.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <algorithm>

NetworkConfig& NetworkConfig::getInstance() {
    static NetworkConfig instance;
    return instance;
}

std::vector<std::string> NetworkConfig::getAvailableIPs() {
    std::vector<std::string> ips;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        return ips;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            char host[NI_MAXHOST];
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                              host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0) {
                std::string ip(host);
                // Skip loopback but include it for testing
                if (ip != "127.0.0.1") {
                    ips.push_back(ip);
                }
            }
        }
    }
    
    freeifaddrs(ifaddr);
    
    // Always add localhost as fallback
    if (ips.empty()) {
        ips.push_back("127.0.0.1");
    }
    
    return ips;
}

std::string NetworkConfig::selectBindIP(const std::string& subnet_prefix) {
    auto ips = getAvailableIPs();
    
    if (ips.empty()) {
        return "0.0.0.0";
    }
    
    // If subnet prefix specified, try to find matching IP
    if (!subnet_prefix.empty()) {
        for (const auto& ip : ips) {
            if (ip.find(subnet_prefix) == 0) {
                return ip;
            }
        }
    }
    
    // Return first non-loopback IP, or any IP
    for (const auto& ip : ips) {
        if (ip.find("127.") != 0) {
            return ip;
        }
    }
    
    return ips[0];
}

bool NetworkConfig::isIPAvailable(const std::string& ip) {
    auto ips = getAvailableIPs();
    return std::find(ips.begin(), ips.end(), ip) != ips.end();
}
