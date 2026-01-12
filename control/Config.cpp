#include "Config.h"

std::vector<ServerConfig> Config::load() {
    return {
        {"server01", "10.0.0.1", 9001},
        {"server02", "10.0.0.2", 9001},
        {"server03", "10.0.0.3", 9001}
    };
}

