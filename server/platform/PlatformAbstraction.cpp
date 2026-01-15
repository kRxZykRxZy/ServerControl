#include "PlatformAbstraction.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <asio.hpp>
#else
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <psapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

using asio::ip::udp;

namespace Platform {

#ifndef _WIN32
// ============= LINUX IMPLEMENTATION =============

SystemStats API::GetSystemStats() {
    static long lastTotal = 0, lastIdle = 0;
    
    std::ifstream stat("/proc/stat");
    std::string cpu;
    long user, nice, system, idle;
    stat >> cpu >> user >> nice >> system >> idle;
    long total = user + nice + system + idle;
    long totald = total - lastTotal;
    long idled = idle - lastIdle;
    lastTotal = total; 
    lastIdle = idle;
    double cpu_usage = totald ? (100.0 * (totald - idled) / totald) : 0;
    
    std::ifstream mem("/proc/meminfo");
    long totalMem, freeMem;
    mem.ignore(256, ':'); mem >> totalMem;
    mem.ignore(256, ':'); mem >> freeMem;
    
    return {cpu_usage, (totalMem - freeMem) / 1024, totalMem / 1024};
}

int API::ExecuteCommand(const std::string& cmd, std::string& output) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return -1;
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }
    
    int exit_code = pclose(pipe);
    return WEXITSTATUS(exit_code);
}

void API::KillProcess(int pid) {
    std::string cmd = "kill -9 " + std::to_string(pid);
    system(cmd.c_str());
}

void API::Reboot() {
    std::thread([](){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        system("sudo reboot");
    }).detach();
}

void API::Shutdown() {
    std::thread([](){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        system("sudo shutdown -h now");
    }).detach();
}

std::string API::GetHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "unknown";
}

std::vector<NetworkInterface> API::GetNetworkInterfaces() {
    std::vector<NetworkInterface> interfaces;
    
    FILE* pipe = popen("ip -br addr | awk '{print $1,$3}'", "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            size_t space = line.find(' ');
            if (space != std::string::npos) {
                NetworkInterface iface;
                iface.name = line.substr(0, space);
                iface.ip = line.substr(space + 1);
                // Remove trailing newline
                if (!iface.ip.empty() && iface.ip.back() == '\n') {
                    iface.ip.pop_back();
                }
                iface.is_up = true;
                interfaces.push_back(iface);
            }
        }
        pclose(pipe);
    }
    
    return interfaces;
}

std::string API::GetLocalIP() {
    try {
        asio::io_context io_context;
        udp::resolver resolver(io_context);
        udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), asio::ip::host_name(), "");
        
        for (const auto& endpoint : endpoints) {
            auto addr = endpoint.endpoint().address();
            if (addr.is_v4() && !addr.is_loopback()) {
                return addr.to_string();
            }
        }
    } catch (...) {}
    return "127.0.0.1";
}

void API::SetExecutable(const std::string& path) {
    chmod(path.c_str(), 0755);
}

std::string API::GetHomeDirectory() {
    const char* home = getenv("HOME");
    return home ? home : "/root";
}

std::string API::GetOSInfo() {
    std::ifstream os_release("/etc/os-release");
    std::string line;
    while (std::getline(os_release, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            return line.substr(13, line.length() - 14);
        }
    }
    return "Unknown Linux";
}

std::string API::GetKernelVersion() {
    std::string output;
    ExecuteCommand("uname -r", output);
    // Remove trailing newline
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return output;
}

std::string API::GetUptime() {
    std::string output;
    ExecuteCommand("uptime -p", output);
    // Remove trailing newline
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return output;
}

std::vector<double> API::GetLoadAverage() {
    std::ifstream loadavg("/proc/loadavg");
    double load1, load5, load15;
    loadavg >> load1 >> load5 >> load15;
    return {load1, load5, load15};
}

bool API::AssignIPAddress(const std::string& ip, const std::string& netmask) {
    // Not implemented for Linux in this context
    return false;
}

std::string API::FindAvailableIP(const std::string& subnet, int startHost, int endHost) {
    // Not implemented for Linux in this context
    return "";
}

#else
// ============= WINDOWS IMPLEMENTATION =============

SystemStats API::GetSystemStats() {
    static ULARGE_INTEGER lastKernel = {0}, lastUser = {0}, lastIdle = {0};
    
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    
    ULARGE_INTEGER nowKernel, nowUser, nowIdle;
    nowKernel.LowPart = kernelTime.dwLowDateTime;
    nowKernel.HighPart = kernelTime.dwHighDateTime;
    nowUser.LowPart = userTime.dwLowDateTime;
    nowUser.HighPart = userTime.dwHighDateTime;
    nowIdle.LowPart = idleTime.dwLowDateTime;
    nowIdle.HighPart = idleTime.dwHighDateTime;
    
    double cpu_usage = 0.0;
    if (lastKernel.QuadPart != 0) {
        ULONGLONG kernelDiff = nowKernel.QuadPart - lastKernel.QuadPart;
        ULONGLONG userDiff = nowUser.QuadPart - lastUser.QuadPart;
        ULONGLONG idleDiff = nowIdle.QuadPart - lastIdle.QuadPart;
        ULONGLONG totalDiff = kernelDiff + userDiff;
        
        if (totalDiff > 0) {
            cpu_usage = 100.0 * (totalDiff - idleDiff) / totalDiff;
        }
    }
    
    lastKernel = nowKernel;
    lastUser = nowUser;
    lastIdle = nowIdle;
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    long ram_total = memInfo.ullTotalPhys / (1024 * 1024);
    long ram_used = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024);
    
    return {cpu_usage, ram_used, ram_total};
}

int API::ExecuteCommand(const std::string& cmd, std::string& output) {
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return -1;
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }
    
    return _pclose(pipe);
}

void API::KillProcess(int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
}

void API::Reboot() {
    std::thread([](){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        system("shutdown /r /t 10");
    }).detach();
}

void API::Shutdown() {
    std::thread([](){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        system("shutdown /s /t 10");
    }).detach();
}

std::string API::GetHostname() {
    char hostname[256];
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        return std::string(hostname);
    }
    return "unknown";
}

std::vector<NetworkInterface> API::GetNetworkInterfaces() {
    std::vector<NetworkInterface> interfaces;
    
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAddresses, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        
        while (pCurrAddresses) {
            if (pCurrAddresses->FirstUnicastAddress) {
                NetworkInterface iface;
                
                // Convert wide string name to narrow
                int nameLen = WideCharToMultiByte(CP_UTF8, 0, pCurrAddresses->FriendlyName, -1, NULL, 0, NULL, NULL);
                char* nameBuf = new char[nameLen];
                WideCharToMultiByte(CP_UTF8, 0, pCurrAddresses->FriendlyName, -1, nameBuf, nameLen, NULL, NULL);
                iface.name = nameBuf;
                delete[] nameBuf;
                
                // Get IP address
                sockaddr_in* pAddr = (sockaddr_in*)pCurrAddresses->FirstUnicastAddress->Address.lpSockaddr;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(pAddr->sin_addr), ipStr, INET_ADDRSTRLEN);
                iface.ip = ipStr;
                
                iface.is_up = (pCurrAddresses->OperStatus == IfOperStatusUp);
                
                interfaces.push_back(iface);
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    
    free(pAddresses);
    return interfaces;
}

std::string API::GetLocalIP() {
    try {
        asio::io_context io_context;
        udp::resolver resolver(io_context);
        udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), asio::ip::host_name(), "");
        
        for (const auto& endpoint : endpoints) {
            auto addr = endpoint.endpoint().address();
            if (addr.is_v4() && !addr.is_loopback()) {
                return addr.to_string();
            }
        }
    } catch (...) {}
    return "127.0.0.1";
}

void API::SetExecutable(const std::string& path) {
    // Not needed on Windows - executability is determined by file extension
}

std::string API::GetHomeDirectory() {
    const char* userprofile = getenv("USERPROFILE");
    if (userprofile) return userprofile;
    
    const char* homepath = getenv("HOMEPATH");
    if (homepath) return homepath;
    
    return "C:\\Users\\Default";
}

std::string API::GetOSInfo() {
    std::string output;
    ExecuteCommand("wmic os get Caption /value", output);
    
    // Parse output
    size_t pos = output.find("Caption=");
    if (pos != std::string::npos) {
        std::string caption = output.substr(pos + 8);
        size_t endPos = caption.find('\r');
        if (endPos != std::string::npos) {
            caption = caption.substr(0, endPos);
        }
        return caption;
    }
    
    return "Windows";
}

std::string API::GetKernelVersion() {
    std::string output;
    ExecuteCommand("ver", output);
    return output.empty() ? "Unknown" : output;
}

std::string API::GetUptime() {
    ULONGLONG uptime = GetTickCount64() / 1000; // Convert to seconds
    
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int minutes = (uptime % 3600) / 60;
    
    std::ostringstream oss;
    if (days > 0) oss << days << " days, ";
    if (hours > 0) oss << hours << " hours, ";
    oss << minutes << " minutes";
    
    return oss.str();
}

std::vector<double> API::GetLoadAverage() {
    // Windows doesn't have load average, return CPU usage as approximation
    auto stats = GetSystemStats();
    return {stats.cpu / 100.0, stats.cpu / 100.0, stats.cpu / 100.0};
}

bool API::AssignIPAddress(const std::string& ip, const std::string& netmask) {
    // Use netsh to assign IP address
    std::string cmd = "netsh interface ip add address \"Ethernet\" " + ip + " " + netmask;
    std::string output;
    int result = ExecuteCommand(cmd, output);
    return result == 0;
}

std::string API::FindAvailableIP(const std::string& subnet, int startHost, int endHost) {
    // Ping sweep to find available IP in 10.125.125.x range
    for (int i = startHost; i <= endHost; i++) {
        std::string testIP = subnet + std::to_string(i);
        std::string cmd = "ping -n 1 -w 100 " + testIP + " >nul 2>&1";
        
        int result = system(cmd.c_str());
        if (result != 0) {
            // IP not responding, likely available
            return testIP;
        }
    }
    
    return ""; // No available IP found
}

#endif

} // namespace Platform
