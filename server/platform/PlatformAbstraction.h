#pragma once

#include <string>
#include <vector>

namespace Platform {

// System statistics structure
struct SystemStats {
    double cpu;           // CPU usage percentage (0-100)
    long ram_used;        // RAM used in MB
    long ram_total;       // Total RAM in MB
};

// Network interface information
struct NetworkInterface {
    std::string name;
    std::string ip;
    std::string netmask;
    bool is_up;
};

// Cross-platform API abstraction
class API {
public:
    // System statistics
    static SystemStats GetSystemStats();
    
    // Command execution
    static int ExecuteCommand(const std::string& cmd, std::string& output);
    static void KillProcess(int pid);
    
    // System control
    static void Reboot();
    static void Shutdown();
    
    // Network operations
    static std::string GetHostname();
    static std::vector<NetworkInterface> GetNetworkInterfaces();
    static std::string GetLocalIP();
    
    // File operations
    static void SetExecutable(const std::string& path);
    static std::string GetHomeDirectory();
    
    // System information
    static std::string GetOSInfo();
    static std::string GetKernelVersion();
    static std::string GetUptime();
    static std::vector<double> GetLoadAverage();
    
    // IP Assignment (Windows-specific for 10.125.125.x)
    static bool AssignIPAddress(const std::string& ip, const std::string& netmask);
    static std::string FindAvailableIP(const std::string& subnet, int startHost, int endHost);
};

} // namespace Platform
