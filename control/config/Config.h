#pragma once
#include <vector>
#include <string>

struct ServerConfig {
    std::string name;
    std::string ip;
    int port;
};

class Config {
public:
    static std::vector<ServerConfig> load();
};
