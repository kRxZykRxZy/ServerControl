#include "UI.h"
#include <ncurses.h>
#include <algorithm>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

UI::UI(TaskManager& manager) : tm(manager) {}

void UI::run() {
    initscr();
    start_color();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor
    timeout(1000); // 1 second timeout for getch
    
    // Initialize color pairs
    init_pair(1, COLOR_GREEN, COLOR_BLACK);   // Online/good
    init_pair(2, COLOR_RED, COLOR_BLACK);     // Error/critical
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Warning
    init_pair(4, COLOR_CYAN, COLOR_BLACK);    // Info
    init_pair(5, COLOR_WHITE, COLOR_BLUE);    // Selected

    while (true) {
        tm.refreshTasks();
        draw();

        int ch = getch();
        if (ch == ERR) continue; // Timeout, just redraw
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
        case Mode::SERVER_STATS: drawServerStats(); break;
    }
    refresh();
}

void UI::drawMain() {
    // Draw header box
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    
    mvprintw(0, 2, "║ SERVER CONTROL PANEL ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    mvprintw(2, 2, "Controls: ↑/↓=Navigate | SPACE=Select | ENTER=Menu | R=Refresh | ESC=Quit");
    
    auto& servers = tm.getServers();
    auto& tasks = tm.getTasks();
    auto& selected = tm.selectedServers();

    for (int i = 0; i < servers.size(); i++) {
        int runningTasks = 0;
        for (auto& t : tasks)
            if (t.server == servers[i].name && t.state == TaskState::RUNNING)
                runningTasks++;

        auto st = tm.getServerStats(servers[i]);
        
        int row = 4 + i * 2;
        
        // Highlight selected server
        if (i == selectedServer) {
            attron(COLOR_PAIR(5) | A_BOLD);
        }

        // Draw server box
        mvprintw(row, 2, "┌─────────────────────────────────────────────────────────────────┐");
        
        // Checkbox and server name
        mvprintw(row + 1, 2, "│ [");
        if (selected[i]) {
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(row + 1, 5, "X");
            attroff(COLOR_PAIR(1) | A_BOLD);
        } else {
            mvprintw(row + 1, 5, " ");
        }
        mvprintw(row + 1, 6, "] ");
        
        // Server name with status color
        if (st.cpu >= 0) {
            attron(COLOR_PAIR(1)); // Online - green
            mvprintw(row + 1, 9, "●");
            attroff(COLOR_PAIR(1));
        } else {
            attron(COLOR_PAIR(2)); // Offline - red
            mvprintw(row + 1, 9, "●");
            attroff(COLOR_PAIR(2));
        }
        
        mvprintw(row + 1, 11, "%-15s", servers[i].name.c_str());
        mvprintw(row + 1, 27, "│ ");
        
        // Stats
        if (st.cpu >= 0) {
            // CPU bar
            int cpuColor = st.cpu > 80 ? 2 : (st.cpu > 50 ? 3 : 1);
            attron(COLOR_PAIR(cpuColor));
            mvprintw(row + 1, 29, "CPU:%5.1f%%", st.cpu);
            attroff(COLOR_PAIR(cpuColor));
            
            // RAM usage
            int ramPercent = st.ramTotal > 0 ? (st.ramUsed * 100 / st.ramTotal) : 0;
            int ramColor = ramPercent > 80 ? 2 : (ramPercent > 50 ? 3 : 1);
            attron(COLOR_PAIR(ramColor));
            mvprintw(row + 1, 41, "RAM:%ld/%ldMB", st.ramUsed, st.ramTotal);
            attroff(COLOR_PAIR(ramColor));
            
            mvprintw(row + 1, 58, "Tasks:%d", runningTasks);
        } else {
            attron(COLOR_PAIR(2));
            mvprintw(row + 1, 29, "OFFLINE");
            attroff(COLOR_PAIR(2));
        }
        
        mvprintw(row + 1, 66, "│");
        mvprintw(row + 2, 2, "└─────────────────────────────────────────────────────────────────┘");

        if (i == selectedServer) {
            attroff(COLOR_PAIR(5) | A_BOLD);
        }
    }
    
    // Draw footer
    int footerRow = LINES - 2;
    mvprintw(footerRow, 2, "Selected servers: ");
    int count = 0;
    for (bool s : selected) if (s) count++;
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(footerRow, 21, "%d", count);
    attroff(COLOR_PAIR(1) | A_BOLD);
}

void UI::drawServerMenu() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ SERVER MENU: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    mvprintw(4, 6, "┌────────────────────────────┐");
    mvprintw(5, 6, "│  [1] 💻 Terminal           │");
    mvprintw(6, 6, "│  [2] 📋 View Logs          │");
    mvprintw(7, 6, "│  [3] 🗑️  Kill Tasks         │");
    mvprintw(8, 6, "│  [4] 📊 CPU/RAM Stats      │");
    mvprintw(9, 6, "└────────────────────────────┘");
    
    mvprintw(11, 6, "[ESC] Back to main menu");
}

void UI::drawServerTerminal() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ TERMINAL: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    // Show command history
    int historyStartRow = 3;
    int historyDisplayCount = std::min(10, (int)commandHistory.size());
    
    attron(COLOR_PAIR(3));
    mvprintw(historyStartRow, 2, "Command History:");
    attroff(COLOR_PAIR(3));
    
    for (int i = 0; i < historyDisplayCount; i++) {
        int idx = commandHistory.size() - historyDisplayCount + i;
        attron(COLOR_PAIR(4));
        mvprintw(historyStartRow + 1 + i, 2, "> %s", commandHistory[idx].c_str());
        attroff(COLOR_PAIR(4));
    }
    
    // Command input at bottom
    int inputRow = LINES - 3;
    attron(A_BOLD | COLOR_PAIR(1));
    mvprintw(inputRow, 2, "┌");
    for (int i = 3; i < COLS - 3; i++) mvprintw(inputRow, i, "─");
    mvprintw(inputRow, COLS - 3, "┐");
    
    mvprintw(inputRow + 1, 2, "│ > ");
    attron(A_UNDERLINE);
    mvprintw(inputRow + 1, 6, "%-*s", COLS - 10, inputBuffer.c_str());
    attroff(A_UNDERLINE);
    mvprintw(inputRow + 1, COLS - 3, "│");
    
    mvprintw(inputRow + 2, 2, "└");
    for (int i = 3; i < COLS - 3; i++) mvprintw(inputRow + 2, i, "─");
    mvprintw(inputRow + 2, COLS - 3, "┘");
    attroff(A_BOLD | COLOR_PAIR(1));
    
    mvprintw(LINES - 1, 2, "[ESC] Back | [ENTER] Execute command");
}

void UI::drawServerLogs() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ TASK LOGS: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));

    int row = 3;
    int maxRows = LINES - 5;
    int taskCount = 0;
    
    for (auto& t : tm.getTasks()) {
        if (t.server != s.name) continue;
        if (row >= maxRows) break;
        
        taskCount++;
        
        // Task header
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(row++, 2, "Task #%s [%s]: %s", 
                 t.id.c_str(), 
                 t.state == TaskState::RUNNING ? "RUNNING" : "FINISHED",
                 t.command.c_str());
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        // Get and display logs
        std::string logs = tm.getLogs(t);
        if (!logs.empty()) {
            // Parse JSON response
            try {
                auto j = json::parse(logs);
                if (j.contains("logs")) {
                    std::string logContent = j["logs"];
                    
                    // Split into lines and display
                    std::istringstream iss(logContent);
                    std::string line;
                    int lineCount = 0;
                    
                    while (std::getline(iss, line) && row < maxRows && lineCount < 5) {
                        mvprintw(row++, 4, "%s", line.c_str());
                        lineCount++;
                    }
                    
                    if (lineCount == 5 && row < maxRows) {
                        attron(COLOR_PAIR(3));
                        mvprintw(row++, 4, "... (truncated)");
                        attroff(COLOR_PAIR(3));
                    }
                }
            } catch (...) {
                mvprintw(row++, 4, "(No logs available)");
            }
        } else {
            mvprintw(row++, 4, "(No logs available)");
        }
        
        row++; // Empty line between tasks
    }
    
    if (taskCount == 0) {
        attron(COLOR_PAIR(3));
        mvprintw(3, 2, "No tasks found for this server");
        attroff(COLOR_PAIR(3));
    }
    
    mvprintw(LINES - 1, 2, "[ESC] Back to menu");
}

void UI::drawServerKill() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ KILL TASKS: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));

    int row = 3;
    int idx = 0;
    int foundTasks = 0;
    
    for (auto& t : tm.getTasks()) {
        if (t.server != s.name || t.state != TaskState::RUNNING)
            continue;

        foundTasks++;
        
        if (idx == selectedTask) {
            attron(COLOR_PAIR(5) | A_BOLD);
        }
        
        mvprintw(row, 4, "┌──────────────────────────────────────────────┐");
        mvprintw(row + 1, 4, "│ Task ID: %-8s                         │", t.id.c_str());
        mvprintw(row + 2, 4, "│ Command: %-31s │", t.command.substr(0, 31).c_str());
        mvprintw(row + 3, 4, "└──────────────────────────────────────────────┘");
        
        if (idx == selectedTask) {
            attroff(COLOR_PAIR(5) | A_BOLD);
        }
        
        row += 5;
        idx++;
    }
    
    if (foundTasks == 0) {
        attron(COLOR_PAIR(3));
        mvprintw(3, 4, "No running tasks to kill");
        attroff(COLOR_PAIR(3));
    }
    
    mvprintw(LINES - 1, 2, "[↑/↓] Navigate | [ENTER] Kill selected task | [ESC] Back");
}

void UI::drawServerStats() {
    auto& s = tm.getServers()[selectedServer];
    auto st = tm.getServerStats(s);
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ SERVER STATS: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    int row = 4;
    
    // CPU Stats
    attron(A_BOLD);
    mvprintw(row++, 4, "CPU Usage:");
    attroff(A_BOLD);
    
    int cpuColor = st.cpu > 80 ? 2 : (st.cpu > 50 ? 3 : 1);
    attron(COLOR_PAIR(cpuColor) | A_BOLD);
    mvprintw(row++, 6, "%.2f%%", st.cpu);
    attroff(COLOR_PAIR(cpuColor) | A_BOLD);
    
    // Draw CPU bar
    int barWidth = 50;
    int filledWidth = (int)(st.cpu * barWidth / 100);
    mvprintw(row, 6, "[");
    attron(COLOR_PAIR(cpuColor));
    for (int i = 0; i < filledWidth; i++) {
        mvprintw(row, 7 + i, "█");
    }
    attroff(COLOR_PAIR(cpuColor));
    for (int i = filledWidth; i < barWidth; i++) {
        mvprintw(row, 7 + i, "░");
    }
    mvprintw(row++, 7 + barWidth, "]");
    row++;
    
    // RAM Stats
    attron(A_BOLD);
    mvprintw(row++, 4, "Memory Usage:");
    attroff(A_BOLD);
    
    long ramPercent = st.ramTotal > 0 ? (st.ramUsed * 100 / st.ramTotal) : 0;
    int ramColor = ramPercent > 80 ? 2 : (ramPercent > 50 ? 3 : 1);
    
    attron(COLOR_PAIR(ramColor) | A_BOLD);
    mvprintw(row++, 6, "%ld MB / %ld MB (%ld%%)", st.ramUsed, st.ramTotal, ramPercent);
    attroff(COLOR_PAIR(ramColor) | A_BOLD);
    
    // Draw RAM bar
    filledWidth = st.ramTotal > 0 ? (int)(st.ramUsed * barWidth / st.ramTotal) : 0;
    mvprintw(row, 6, "[");
    attron(COLOR_PAIR(ramColor));
    for (int i = 0; i < filledWidth; i++) {
        mvprintw(row, 7 + i, "█");
    }
    attroff(COLOR_PAIR(ramColor));
    for (int i = filledWidth; i < barWidth; i++) {
        mvprintw(row, 7 + i, "░");
    }
    mvprintw(row++, 7 + barWidth, "]");
    
    mvprintw(LINES - 1, 2, "[ESC] Back to menu");
}

/* ---------------- INPUT ---------------- */

void UI::handleInput(int ch) {
    auto& servers = tm.getServers();
    
    if (mode == Mode::MAIN) {
        if (ch == KEY_UP && selectedServer > 0) selectedServer--;
        if (ch == KEY_DOWN && selectedServer < servers.size() - 1) selectedServer++;
        if (ch == ' ') tm.toggleServer(selectedServer);
        if (ch == '\n') mode = Mode::SERVER_MENU;
        if (ch == 'r' || ch == 'R') tm.refreshTasks();
    }

    else if (mode == Mode::SERVER_MENU) {
        if (ch == '1') mode = Mode::SERVER_TERMINAL;
        if (ch == '2') mode = Mode::SERVER_LOGS;
        if (ch == '3') mode = Mode::SERVER_KILL;
        if (ch == '4') mode = Mode::SERVER_STATS;
        if (ch == 27) mode = Mode::MAIN;
    }

    else if (mode == Mode::SERVER_TERMINAL) {
        if (ch == '\n' && !inputBuffer.empty()) {
            commandHistory.push_back(inputBuffer);
            tm.runCommandOnServer(
                servers[selectedServer],
                inputBuffer
            );
            inputBuffer.clear();
        }
        else if (ch == 27) mode = Mode::SERVER_MENU;
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            if (!inputBuffer.empty()) inputBuffer.pop_back();
        else if (isprint(ch)) inputBuffer.push_back(ch);
    }

    else if (mode == Mode::SERVER_LOGS) {
        if (ch == 27) mode = Mode::SERVER_MENU;
    }

    else if (mode == Mode::SERVER_KILL) {
        auto tasks = tm.getTasksForServer(servers[selectedServer]);
        
        // Filter for running tasks only
        std::vector<Task> runningTasks;
        for (auto& t : tasks) {
            if (t.state == TaskState::RUNNING)
                runningTasks.push_back(t);
        }
        
        if (ch == '\n' && !runningTasks.empty() && selectedTask < runningTasks.size()) {
            tm.killTask(runningTasks[selectedTask]);
            selectedTask = 0;
        }
        if (ch == KEY_UP && selectedTask > 0) selectedTask--;
        if (ch == KEY_DOWN && selectedTask < runningTasks.size() - 1) selectedTask++;
        if (ch == 27) {
            mode = Mode::SERVER_MENU;
            selectedTask = 0;
        }
    }
    
    else if (mode == Mode::SERVER_STATS) {
        if (ch == 27) mode = Mode::SERVER_MENU;
    }
}
