#include <iostream>
#include "Config.h"
#include "TaskManager.h"

int main() {
    auto cfg = Config::load();
    std::vector<Server> servers;

    for (auto& c : cfg)
        servers.push_back({c.name, c.ip, c.port});

    TaskManager tm(servers);

    std::string cmd;
    while (true) {
        std::cout << "cluster> ";
        std::getline(std::cin, cmd);

        if (cmd == "exit") break;
        if (cmd == "tasks") {
            tm.listTasks();
            continue;
        }

        std::cout << "Server index:\n";
        for (int i = 0; i < servers.size(); i++)
            std::cout << i << ": " << servers[i].name << "\n";

        int idx;
        std::cin >> idx;
        std::cin.ignore();

        tm.runCommand(cmd, idx);
    }
}

