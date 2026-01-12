#include "UI.h"
#include <ncurses.h>

UI::UI(TaskManager& manager) : tm(manager) {}

void UI::run() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    while (true) {
        tm.refreshTasks();
        draw();

        int ch = getch();
        if (ch == 27 && mode == Mode::MAIN) break; // ESC exits
        handleInput(ch);
    }

    endwin();
}

/* ---------------- DRAW ---------------- */

void UI::draw() {
    clear();
    switch (mode) {
        case Mode::MAIN: drawMain(); break;
        case Mode::SERVER_MENU: drawServerMenu(); break;
        case Mode::SERVER_TERMINAL: drawServerTerminal(); break;
        case Mode::SERVER_LOGS: drawServerLogs(); break;
        case Mode::SERVER_KILL: drawServerKill(); break;
    }
    refresh();
}

void UI::drawMain() {
    mvprintw(0, 2, "SERVERS (ENTER = menu, ESC = quit)");
    auto& servers = tm.getServers();
    auto& tasks = tm.getTasks();

    for (int i = 0; i < servers.size(); i++) {
        int count = 0;
        for (auto& t : tasks)
            if (t.server == servers[i].name &&
                t.state == TaskState::RUNNING)
                count++;

        if (i == selectedServer) attron(A_REVERSE);
        mvprintw(2 + i, 4, "%s | running: %d",
                 servers[i].name.c_str(), count);
        if (i == selectedServer) attroff(A_REVERSE);
    }
}

void UI::drawServerMenu() {
    auto& s = tm.getServers()[selectedServer];
    mvprintw(2, 4, "SERVER: %s", s.name.c_str());
    mvprintw(4, 6, "[1] Terminal");
    mvprintw(5, 6, "[2] Logs");
    mvprintw(6, 6, "[3] Kill Tasks");
    mvprintw(8, 6, "[ESC] Back");
}

void UI::drawServerTerminal() {
    auto& s = tm.getServers()[selectedServer];
    mvprintw(0, 2, "TERMINAL: %s (ESC to exit)", s.name.c_str());
    mvprintw(LINES - 2, 2, "> %s", inputBuffer.c_str());
}

void UI::drawServerLogs() {
    auto& s = tm.getServers()[selectedServer];
    mvprintw(0, 2, "LOGS: %s (ESC to exit)", s.name.c_str());

    int row = 2;
    for (auto& t : tm.getTasks()) {
        if (t.server != s.name) continue;
        std::string logs = tm.getLogs(t);
        mvprintw(row++, 2, "[task %s]", t.id.c_str());
        mvprintw(row++, 4, "%s", logs.c_str());
    }
}

void UI::drawServerKill() {
    auto& s = tm.getServers()[selectedServer];
    mvprintw(0, 2, "KILL TASKS: %s (ENTER to kill, ESC back)", s.name.c_str());

    int row = 2;
    int idx = 0;
    for (auto& t : tm.getTasks()) {
        if (t.server != s.name ||
            t.state != TaskState::RUNNING)
            continue;

        if (idx == selectedTask) attron(A_REVERSE);
        mvprintw(row++, 4, "Task %s | %s",
                 t.id.c_str(), t.command.c_str());
        if (idx == selectedTask) attroff(A_REVERSE);
        idx++;
    }
}

/* ---------------- INPUT ---------------- */

void UI::handleInput(int ch) {
    if (mode == Mode::MAIN) {
        if (ch == KEY_UP) selectedServer--;
        if (ch == KEY_DOWN) selectedServer++;
        if (ch == '\n') mode = Mode::SERVER_MENU;
    }

    else if (mode == Mode::SERVER_MENU) {
        if (ch == '1') mode = Mode::SERVER_TERMINAL;
        if (ch == '2') mode = Mode::SERVER_LOGS;
        if (ch == '3') mode = Mode::SERVER_KILL;
        if (ch == 27) mode = Mode::MAIN;
    }

    else if (mode == Mode::SERVER_TERMINAL) {
        if (ch == '\n') {
            tm.runCommandOnServer(
                tm.getServers()[selectedServer],
                inputBuffer
            );
            inputBuffer.clear();
        }
        else if (ch == 27) mode = Mode::SERVER_MENU;
        else if (ch == KEY_BACKSPACE || ch == 127)
            if (!inputBuffer.empty()) inputBuffer.pop_back();
        else if (isprint(ch)) inputBuffer.push_back(ch);
    }

    else if (mode == Mode::SERVER_LOGS) {
        if (ch == 27) mode = Mode::SERVER_MENU;
    }

    else if (mode == Mode::SERVER_KILL) {
        if (ch == '\n') {
            auto tasks = tm.getTasksForServer(
                tm.getServers()[selectedServer]
            );
            if (!tasks.empty())
                tm.killTask(tasks[selectedTask]);
        }
        if (ch == KEY_UP) selectedTask--;
        if (ch == KEY_DOWN) selectedTask++;
        if (ch == 27) mode = Mode::SERVER_MENU;
    }
}
