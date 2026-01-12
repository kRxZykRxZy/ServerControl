#pragma once
#include <vector>
#include "Server.h"
#include "Task.h"

class TaskManager {
public:
    TaskManager(const std::vector<Server>& servers);

    void runCommand(const std::string& cmd, int serverIndex);
    void listTasks();
    void showLogs(const std::string& taskId, int serverIndex);

private:
    std::vector<Server> servers;
};

