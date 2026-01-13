#pragma once
#include "Task.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

class TaskManager {
public:
    static TaskManager& getInstance();
    
    std::string createTask(const std::string& command);
    bool getTaskStatus(const std::string& task_id, bool& running);
    std::string getTaskOutput(const std::string& task_id);
    std::vector<std::string> listTasks();
    bool killTask(const std::string& task_id);
    
private:
    TaskManager() : task_counter(0) {}
    std::map<std::string, Task> tasks;
    std::mutex tasks_mtx;
    int task_counter;
};
