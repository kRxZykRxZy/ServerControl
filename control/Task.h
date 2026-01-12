#pragma once
#include <string>

enum class TaskState {
    RUNNING,
    FINISHED
};

struct Task {
    std::string id;
    std::string server;
    std::string command;
    TaskState state;
};
