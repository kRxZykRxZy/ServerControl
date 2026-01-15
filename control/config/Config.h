#pragma once
#include <vector>
#include <string>

struct ServerConfig {
    std::string name;
    std::string ip;
    int port;
    int ws_main_port = 2040;
    int ws_stats_port = 2041;
    int ws_files_port = 2042;
    int ws_desktop_port = 2043;
};

class Config {
public:
    static std::vector<ServerConfig> load();
};
