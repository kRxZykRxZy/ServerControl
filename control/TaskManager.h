#pragma once
#include <vector>
#include "Server.h"
#include "Task.h"

class TaskManager {
public:
    TaskManager(const std::vector<Server>& servers);

    void runOnSelected(const std::string& cmd,
                       const std::vector<bool>& selected);

    std::vector<Task> fetchTasks();

private:
    std::vector<Server> servers;
};
