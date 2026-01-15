#include "TaskExecutor.h"
#include "../platform/PlatformAbstraction.h"
#include <sstream>
#include <chrono>

namespace Tasks {

Task::Task(Task&& other) noexcept 
    : id(std::move(other.id))
    , command(std::move(other.command))
    , output(std::move(other.output))
    , current_dir(std::move(other.current_dir))
    , running(other.running.load())
    , thread(std::move(other.thread))
{}

Task& Task::operator=(Task&& other) noexcept {
    if (this != &other) {
        id = std::move(other.id);
        command = std::move(other.command);
        output = std::move(other.output);
        current_dir = std::move(other.current_dir);
        running.store(other.running.load());
        thread = std::move(other.thread);
    }
    return *this;
}

Manager& Manager::Instance() {
    static Manager instance;
    return instance;
}

std::string Manager::Execute(const std::string& command, const std::string& workingDir) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    std::string task_id = std::to_string(++task_counter_);
    Task task;
    task.id = task_id;
    task.command = command;
    task.current_dir = workingDir.empty() ? Platform::API::GetHomeDirectory() : workingDir;
    
    tasks_[task_id] = std::move(task);
    tasks_[task_id].thread = std::thread(&Manager::RunTask, this, std::ref(tasks_[task_id]));
    tasks_[task_id].thread.detach();
    
    return task_id;
}

void Manager::Kill(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    if (tasks_.count(taskId)) {
        tasks_[taskId].running = false;
        // TODO: Actually kill the process
    }
}

bool Manager::IsRunning(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    if (tasks_.count(taskId)) {
        return tasks_[taskId].running.load();
    }
    return false;
}

std::string Manager::GetOutput(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    if (tasks_.count(taskId)) {
        return tasks_[taskId].output;
    }
    return "";
}

std::string Manager::GetCurrentDir(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    if (tasks_.count(taskId)) {
        return tasks_[taskId].current_dir;
    }
    return "";
}

void Manager::SetOutputCallback(OutputCallback callback) {
    output_callback_ = callback;
}

void Manager::SetCompletionCallback(CompletionCallback callback) {
    completion_callback_ = callback;
}

std::map<std::string, Task>& Manager::GetTasks() {
    return tasks_;
}

void Manager::RunTask(Task& task) {
    // Handle cd commands specially to track directory changes
    std::string cmd = task.command;
    if (cmd.find("cd ") == 0) {
        // Extract directory from cd command
        std::string new_dir = cmd.substr(3);
        // Trim whitespace
        new_dir.erase(0, new_dir.find_first_not_of(" \t\n\r\f\v"));
        new_dir.erase(new_dir.find_last_not_of(" \t\n\r\f\v") + 1);
        
        // Expand relative paths
        if (new_dir == "~" || new_dir.find("~/") == 0) {
            new_dir = Platform::API::GetHomeDirectory() + new_dir.substr(1);
        } else if (!new_dir.empty() && new_dir[0] != '/' && new_dir[0] != '\\' && 
                   !(new_dir.length() >= 2 && new_dir[1] == ':')) { // Not absolute path on Windows either
            // Relative path
            new_dir = task.current_dir + "/" + new_dir;
        }
        
        task.current_dir = new_dir;
        task.running = false;
        
        // Notify directory change
        if (output_callback_) {
            output_callback_(task.id, "Changed directory to: " + task.current_dir + "\n");
        }
        
        if (completion_callback_) {
            completion_callback_(task.id, 0);
        }
        return;
    }
    
    // Execute command in current directory
    std::string full_cmd;
#ifdef _WIN32
    full_cmd = "cd /d \"" + task.current_dir + "\" && " + cmd;
#else
    full_cmd = "cd \"" + task.current_dir + "\" && " + cmd;
#endif
    
    std::string output;
    int exit_code = Platform::API::ExecuteCommand(full_cmd, output);
    
    task.output = output;
    task.running = false;
    
    // Stream output in chunks
    if (output_callback_) {
        output_callback_(task.id, output);
    }
    
    if (completion_callback_) {
        completion_callback_(task.id, exit_code);
    }
}

} // namespace Tasks
