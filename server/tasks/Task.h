#pragma once
#include <string>
#include <atomic>
#include <thread>

struct Task {
    std::string id;
    std::string command;
    std::string output;
    std::atomic<bool> running{true};
    std::thread thread;
    
    Task() = default;
    Task(Task&& other) noexcept 
        : id(std::move(other.id))
        , command(std::move(other.command))
        , output(std::move(other.output))
        , running(other.running.load())
        , thread(std::move(other.thread))
    {}
    
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            id = std::move(other.id);
            command = std::move(other.command);
            output = std::move(other.output);
            running.store(other.running.load());
            thread = std::move(other.thread);
        }
        return *this;
    }
    
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
};

void runTask(Task& t);
