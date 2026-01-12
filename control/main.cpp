#include "Config.h"
#include "UI.h"

int main() {
    auto cfg = Config::load();
    std::vector<Server> servers;

    for (auto& c : cfg)
        servers.push_back({c.name, c.ip, c.port});

    UI ui(servers);
    ui.run();
}
