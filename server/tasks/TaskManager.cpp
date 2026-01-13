#include "TaskManager.h"
#include <algorithm>

TaskManager& TaskManager::getInstance() {
    static TaskManager instance;
    return instance;
}

std::string TaskManager::createTask(const std::string& command) {
    std::lock_guard<std::mutex> lock(tasks_mtx);
    
    std::string task_id = std::to_string(++task_counter);
    Task t;
    t.command = command;
    t.id = task_id;
    
    tasks[task_id] = std::move(t);
    tasks[task_id].thread = std::thread(runTask, std::ref(tasks[task_id]));
    tasks[task_id].thread.detach();
    
    return task_id;
}

bool TaskManager::getTaskStatus(const std::string& task_id, bool& running) {
    std::lock_guard<std::mutex> lock(tasks_mtx);
    auto it = tasks.find(task_id);
    if (it == tasks.end()) {
        return false;
    }
    running = it->second.running.load();
    return true;
}

std::string TaskManager::getTaskOutput(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mtx);
    auto it = tasks.find(task_id);
    if (it == tasks.end()) {
        return "";
    }
    return it->second.output;
}

std::vector<std::string> TaskManager::listTasks() {
    std::lock_guard<std::mutex> lock(tasks_mtx);
    std::vector<std::string> task_ids;
    for (const auto& pair : tasks) {
        task_ids.push_back(pair.first);
    }
    return task_ids;
}

bool TaskManager::killTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mtx);
    auto it = tasks.find(task_id);
    if (it == tasks.end()) {
        return false;
    }
    // Mark as not running - the thread will exit naturally
    it->second.running = false;
    return true;
}
