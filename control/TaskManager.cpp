#include "TaskManager.h"
#include "HttpClient.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TaskManager::TaskManager(const std::vector<Server>& s)
    : servers(s), selected(s.size(), false) {}


// ---------------- SERVER SELECTION ----------------

void TaskManager::toggleServer(size_t index) {
    if (index < selected.size())
        selected[index] = !selected[index];
}

const std::vector<bool>& TaskManager::selectedServers() const {
    return selected;
}


// ---------------- TASK EXECUTION ----------------

void TaskManager::execOnServer(
    const Server& server,
    const std::string& cmd,
    const std::unordered_map<std::string, std::string>& env
) {
    json payload;
    payload["cmd"] = cmd;
    payload["env"] = env;

    std::string res = HttpClient::post(
        server.ip,
        server.port,
        "/exec",
        payload.dump()
    );

    auto r = json::parse(res);
    std::string taskId = r["task_id"];

    tasks.push_back({
        taskId,
        server.name,
        cmd,
        TaskState::RUNNING
    });
}

void TaskManager::runCommand(const std::string& cmd) {
    for (size_t i = 0; i < servers.size(); i++) {
        if (!selected[i]) continue;
        execOnServer(servers[i], cmd, {});
    }
}

void TaskManager::runJointCommand(const std::string& cmd) {
    int total = 0;
    for (bool s : selected)
        if (s) total++;

    int workerId = 0;
    for (size_t i = 0; i < servers.size(); i++) {
        if (!selected[i]) continue;

        execOnServer(
            servers[i],
            cmd,
            {
                {"WORKER_ID", std::to_string(workerId++)},
                {"TOTAL_WORKERS", std::to_string(total)}
            }
        );
    }
}


// ---------------- TASK TRACKING ----------------

void TaskManager::refreshTasks() {
    for (auto& s : servers) {
        std::string res = HttpClient::get(
            s.ip,
            s.port,
            "/tasks"
        );

        auto j = json::parse(res);
        for (auto& jt : j) {
            std::string id = jt["id"];
            bool running = jt["running"];

            for (auto& t : tasks) {
                if (t.id == id && t.server == s.name) {
                    t.state = running
                        ? TaskState::RUNNING
                        : TaskState::FINISHED;
                }
            }
        }
    }
}

const std::vector<Task>& TaskManager::getTasks() const {
    return tasks;
}


// ---------------- LOGS & CONTROL ----------------

std::string TaskManager::getLogs(const Task& task) {
    for (auto& s : servers) {
        if (s.name == task.server) {
            return HttpClient::get(
                s.ip,
                s.port,
                "/logs?id=" + task.id
            );
        }
    }
    return {};
}

void TaskManager::killTask(const Task& task) {
    for (auto& s : servers) {
        if (s.name == task.server) {
            HttpClient::post(
                s.ip,
                s.port,
                "/kill?id=" + task.id,
                ""
            );
        }
    }
}
