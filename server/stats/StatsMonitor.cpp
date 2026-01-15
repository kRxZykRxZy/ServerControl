#include "StatsMonitor.h"
#include <thread>
#include <chrono>
#include <iostream>

namespace Stats {

Monitor& Monitor::Instance() {
    static Monitor instance;
    return instance;
}

void Monitor::Start(int intervalSeconds) {
    if (running_) return;
    
    interval_seconds_ = intervalSeconds;
    running_ = true;
    
    std::thread([this]() {
        MonitorLoop();
    }).detach();
}

void Monitor::Stop() {
    running_ = false;
}

Platform::SystemStats Monitor::GetCurrent() {
    return Platform::API::GetSystemStats();
}

void Monitor::SetStatsCallback(StatsCallback callback) {
    stats_callback_ = callback;
}

void Monitor::SetAlertCallback(AlertCallback callback, double cpuThreshold) {
    alert_callback_ = callback;
    cpu_threshold_ = cpuThreshold;
}

void Monitor::MonitorLoop() {
    const double alert_cooldown = 60.0; // Alert at most once per minute
    
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds_));
        
        auto stats = Platform::API::GetSystemStats();
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        // Call stats callback
        if (stats_callback_) {
            stats_callback_(stats);
        }
        
        // Check for CPU alert
        if (alert_callback_ && stats.cpu > cpu_threshold_) {
            double current_time = timestamp / 1000.0;
            if (current_time - last_alert_time_ > alert_cooldown) {
                std::string message = "CPU usage exceeded " + std::to_string((int)cpu_threshold_) + "%!";
                alert_callback_(stats, message);
                last_alert_time_ = current_time;
                
                // Also log to console
                std::cout << "⚠️  CPU ALERT: " << stats.cpu << "% on " 
                          << Platform::API::GetHostname() << std::endl;
            }
        }
    }
}

} // namespace Stats
