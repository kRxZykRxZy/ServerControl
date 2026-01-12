#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "Server.h"
#include "Task.h"

class TaskManager {
public:
    TaskManager(const std::vector<Server>& servers);

    // ---- server selection ----
    void toggleServer(size_t index);
    const std::vector<bool>& selectedServers() const;

    // ---- task execution ----
    void runCommand(const std::string& cmd);
    void runJointCommand(const std::string& cmd);

    // ---- task management ----
    void refreshTasks();
    const std::vector<Task>& getTasks() const;

    std::string getLogs(const Task& task);
    void killTask(const Task& task);

    // ===== ADD THESE THREE HERE =====
    const std::vector<Server>& getServers() const;
    std::vector<Task> getTasksForServer(const Server& s) const;
    void runCommandOnServer(const Server& s, const std::string& cmd);

private:
    std::vector<Server> servers;
    std::vector<bool> selected;
    std::vector<Task> tasks;

    void execOnServer(
        const Server& server,
        const std::string& cmd,
        const std::unordered_map<std::string, std::string>& env
    );
};
