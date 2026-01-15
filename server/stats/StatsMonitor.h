#pragma once

#include "../platform/PlatformAbstraction.h"
#include <string>
#include <functional>

namespace Stats {

// Stats update callback
using StatsCallback = std::function<void(const Platform::SystemStats&)>;
using AlertCallback = std::function<void(const Platform::SystemStats&, const std::string& message)>;

class Monitor {
public:
    static Monitor& Instance();
    
    // Start monitoring (calls callback every interval)
    void Start(int intervalSeconds = 1);
    
    // Stop monitoring
    void Stop();
    
    // Get current stats
    Platform::SystemStats GetCurrent();
    
    // Set stats update callback
    void SetStatsCallback(StatsCallback callback);
    
    // Set alert callback (triggered when CPU > threshold)
    void SetAlertCallback(AlertCallback callback, double cpuThreshold = 90.0);
    
    bool IsRunning() const { return running_; }
    
private:
    Monitor() = default;
    void MonitorLoop();
    
    bool running_ = false;
    int interval_seconds_ = 1;
    StatsCallback stats_callback_;
    AlertCallback alert_callback_;
    double cpu_threshold_ = 90.0;
    double last_alert_time_ = 0.0;
};

} // namespace Stats
