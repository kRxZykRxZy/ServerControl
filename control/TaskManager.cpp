#include "TaskManager.h"
#include "HttpClient.h"
#include <iostream>

TaskManager::TaskManager(const std::vector<Server>& s)
    : servers(s) {}

void TaskManager::runCommand(const std::string& cmd, int idx) {
    auto& srv = servers[idx];
    std::string res = HttpClient::post(
        srv.ip, srv.port, "/exec", cmd);
    std::cout << "[" << srv.name << "] " << res << "\n";
}

void TaskManager::listTasks() {
    for (auto& s : servers) {
        std::string res = HttpClient::get(
            s.ip, s.port, "/tasks");
        std::cout << "[" << s.name << "] " << res << "\n";
    }
}

void TaskManager::showLogs(const std::string& id, int idx) {
    auto& s = servers[idx];
    std::string res = HttpClient::get(
        s.ip, s.port, "/logs?id=" + id);
    std::cout << res << "\n";
}

