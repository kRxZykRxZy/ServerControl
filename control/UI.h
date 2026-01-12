#pragma once
#include <vector>
#include <string>
#include "Server.h"
#include "Task.h"

class UI {
public:
    UI(const std::vector<Server>& servers);
    void run();

private:
    std::vector<Server> servers;
    std::vector<bool> selected;

    std::string command;

    void draw();
    void handleInput(int ch);
};

