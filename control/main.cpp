#include "config/Config.h"
#include "ui/UI.h"
#include "core/TaskManager.h"

int main() {
    auto cfg = Config::load();
    std::vector<Server> servers;

    for (auto& c : cfg)
        servers.push_back({c.name, c.ip, c.port});

    TaskManager tm(servers);
    UI ui(tm);
    ui.run();
}
