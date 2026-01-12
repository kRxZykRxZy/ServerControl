#pragma once
#include <vector>
#include <string>
#include "Server.h"
#include "TaskManager.h"

class UI {
public:
    UI(TaskManager& tm);
    void run();

private:
    TaskManager& tm;
    int selectedServer = 0;

    enum class Mode {
        MAIN,
        SERVER_MENU,
        SERVER_TERMINAL,
        SERVER_LOGS,
        SERVER_KILL,
        SERVER_STATS
    } mode = Mode::MAIN;

    std::string inputBuffer;
    std::vector<std::string> commandHistory;
    int selectedTask = 0;

    void draw();
    void handleInput(int ch);

    void drawMain();
    void drawServerMenu();
    void drawServerTerminal();
    void drawServerLogs();
    void drawServerKill();
    void drawServerStats();
};
