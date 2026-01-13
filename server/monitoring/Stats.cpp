#include "Stats.h"
#include "../network/WebSocket.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <unistd.h>

using json = nlohmann::json;

extern void broadcast_ws(const std::string& message);
extern std::string get_hostname();

Stats getStats() {
    static long lastTotal = 0, lastIdle = 0;
    std::ifstream stat("/proc/stat");
    std::string cpu;
    long user, nice, system, idle;
    stat >> cpu >> user >> nice >> system >> idle;
    long total = user + nice + system + idle;
    long totald = total - lastTotal;
    long idled = idle - lastIdle;
    lastTotal = total; lastIdle = idle;
    double cpu_usage = totald ? (100.0*(totald - idled)/totald) : 0;
    std::ifstream mem("/proc/meminfo");
    long totalMem, freeMem;
    mem.ignore(256, ':'); mem >> totalMem;
    mem.ignore(256, ':'); mem >> freeMem;
    return {cpu_usage, (totalMem - freeMem)/1024, totalMem/1024};
}

void cpu_monitor() {
    double last_alert_time = 0;
    const double alert_cooldown = 60.0;
    
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto stats = getStats();
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        // Broadcast stats update
        json stats_msg = {
            {"type", "stats_update"},
            {"cpu", stats.cpu},
            {"ram_used", stats.ram_used},
            {"ram_total", stats.ram_total},
            {"timestamp", timestamp}
        };
        broadcast_ws(stats_msg.dump());
        
        // Check for CPU alert (>90%)
        if (stats.cpu > 90.0) {
            double current_time = timestamp / 1000.0;
            if (current_time - last_alert_time > alert_cooldown) {
                json alert_msg = {
                    {"type", "cpu_alert"},
                    {"cpu", stats.cpu},
                    {"hostname", get_hostname()},
                    {"message", "CPU usage exceeded 90%!"},
                    {"timestamp", timestamp}
                };
                broadcast_ws(alert_msg.dump());
                last_alert_time = current_time;
                
                std::cout << "⚠️  CPU ALERT: " << stats.cpu << "% on " << get_hostname() << std::endl;
            }
        }
    }
}
