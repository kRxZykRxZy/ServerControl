#pragma once

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

namespace Tasks {

// Task execution callback
using OutputCallback = std::function<void(const std::string& taskId, const std::string& output)>;
using CompletionCallback = std::function<void(const std::string& taskId, int exitCode)>;

struct Task {
    std::string id;
    std::string command;
    std::string output;
    std::string current_dir;
    std::atomic<bool> running{true};
    std::thread thread;
    
    Task() = default;
    Task(Task&& other) noexcept;
    Task& operator=(Task&& other) noexcept;
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
};

class Manager {
public:
    static Manager& Instance();
    
    // Execute a command and return task ID
    std::string Execute(const std::string& command, const std::string& workingDir = "");
    
    // Kill a task
    void Kill(const std::string& taskId);
    
    // Get task status
    bool IsRunning(const std::string& taskId);
    
    // Get task output
    std::string GetOutput(const std::string& taskId);
    
    // Get current directory for task
    std::string GetCurrentDir(const std::string& taskId);
    
    // Set callbacks
    void SetOutputCallback(OutputCallback callback);
    void SetCompletionCallback(CompletionCallback callback);
    
    // Get all tasks
    std::map<std::string, Task>& GetTasks();
    
private:
    Manager() = default;
    void RunTask(Task& task);
    
    std::map<std::string, Task> tasks_;
    std::mutex tasks_mutex_;
    int task_counter_ = 0;
    OutputCallback output_callback_;
    CompletionCallback completion_callback_;
};

} // namespace Tasks
