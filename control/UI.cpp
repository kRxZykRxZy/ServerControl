#include "UI.h"
#include <ncurses.h>
#include <algorithm>
#include <sstream>
#include <climits>
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
        case Mode::SERVER_FILES: drawServerFiles(); break;
        case Mode::FILE_EDITOR: drawFileEditor(); break;
        case Mode::FILE_OPERATIONS: drawServerFiles(); break; // Reuse for now
        case Mode::MULTI_SERVER_EXEC: drawMultiServerExec(); break;
    }
    refresh();
}

void UI::drawServerFiles() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 📁 HOLOGRAPHIC FILE SYSTEM: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto files = tm.listFiles(s);
    
    attron(COLOR_PAIR(3));
    mvprintw(3, 2, "⌨  [N]ew | [E]dit | [D]elete | [R]ename | [U]pload | [↓]Download | [C]opy | [X]Cut | [V]Paste | [ESC] Back");
    attroff(COLOR_PAIR(3));
    
    int row = 5;
    int idx = 0;
    
    if (files.empty()) {
        attron(COLOR_PAIR(3));
        mvprintw(row, 4, "⚠ No files in storage. Press 'N' to create a new file or 'U' to upload.");
        attroff(COLOR_PAIR(3));
    } else {
        for (const auto& file : files) {
            if (idx == selectedFile) {
                attron(COLOR_PAIR(5) | A_BOLD);
            }
            
            mvprintw(row, 4, "┌──────────────────────────────────────────────────────────┐");
            
            std::string icon = file.is_dir ? "📁" : "📄";
            mvprintw(row + 1, 4, "│ %s %-30s", icon.c_str(), file.name.c_str());
            
            if (!file.is_dir) {
                double size_kb = file.size / 1024.0;
                std::string size_str = size_kb < 1024 ? 
                    std::to_string((int)size_kb) + " KB" : 
                    std::to_string((int)(size_kb/1024)) + " MB";
                mvprintw(row + 1, 45, "%10s │", size_str.c_str());
            } else {
                mvprintw(row + 1, 57, "│");
            }
            
            mvprintw(row + 2, 4, "│ 🕐 %s", file.modified.c_str());
            int spaces_needed = 49 - file.modified.length();
            for (int i = 0; i < spaces_needed; i++) mvprintw(row + 2, 11 + file.modified.length() + i, " ");
            mvprintw(row + 2, 57, "│");
            
            mvprintw(row + 3, 4, "└──────────────────────────────────────────────────────────┘");
            
            if (idx == selectedFile) {
                attroff(COLOR_PAIR(5) | A_BOLD);
            }
            
            row += 5;
            idx++;
            
            if (row > LINES - 5) break;
        }
    }
    
    // Show clipboard status
    if (!clipboardFile.empty()) {
        attron(COLOR_PAIR(1));
        mvprintw(LINES - 1, 2, "📋 Clipboard: %s (%s)", 
                 clipboardFile.c_str(), 
                 clipboardIsCut ? "CUT" : "COPY");
        attroff(COLOR_PAIR(1));
    }
}

void UI::drawFileEditor() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ ✏️  NANO-EDITOR 2050: %s ║", currentEditFile.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    // Display file content
    int displayRow = 3;
    int maxDisplayLines = LINES - 6;
    
    for (int i = editorScrollOffset; i < fileEditorLines.size() && i < editorScrollOffset + maxDisplayLines; i++) {
        int lineNum = i + 1;
        
        if (i == editorCursorRow) {
            attron(COLOR_PAIR(5));
        }
        
        attron(COLOR_PAIR(3));
        mvprintw(displayRow, 2, "%4d │", lineNum);
        attroff(COLOR_PAIR(3));
        
        mvprintw(displayRow, 9, "%s", fileEditorLines[i].c_str());
        
        if (i == editorCursorRow) {
            attroff(COLOR_PAIR(5));
        }
        
        displayRow++;
    }
    
    // Status bar
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(LINES - 3, 0, "─");
    for (int i = 1; i < COLS; i++) mvprintw(LINES - 3, i, "─");
    
    mvprintw(LINES - 2, 2, "Line: %d/%zu | Col: %d | Lines: %zu", 
             editorCursorRow + 1, fileEditorLines.size(), 
             editorCursorCol, fileEditorLines.size());
    attroff(COLOR_PAIR(4) | A_BOLD);
    
    attron(COLOR_PAIR(3));
    mvprintw(LINES - 1, 2, "[Ctrl+S] Save | [Ctrl+Q] Quit | [Ctrl+X] Cut Line | [Ctrl+C] Copy Line | [Ctrl+V] Paste");
    attroff(COLOR_PAIR(3));
}

void UI::drawMultiServerExec() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 🚀 QUANTUM MULTI-SERVER ORCHESTRATOR ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& servers = tm.getServers();
    auto& selected = tm.selectedServers();
    
    int selectedCount = 0;
    for (bool s : selected) if (s) selectedCount++;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(3, 4, "⚡ %d servers selected for parallel execution", selectedCount);
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    int row = 5;
    mvprintw(row++, 4, "Selected Servers:");
    for (int i = 0; i < servers.size(); i++) {
        if (selected[i]) {
            attron(COLOR_PAIR(1));
            mvprintw(row++, 6, "◉ %s", servers[i].name.c_str());
            attroff(COLOR_PAIR(1));
        }
    }
    
    row += 2;
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "Command History:");
    attroff(COLOR_PAIR(3));
    
    int historyStart = std::max(0, (int)commandHistory.size() - 5);
    for (int i = historyStart; i < commandHistory.size(); i++) {
        attron(COLOR_PAIR(4));
        mvprintw(row++, 6, "> %s", commandHistory[i].c_str());
        attroff(COLOR_PAIR(4));
    }
    
    // Command input at bottom
    int inputRow = LINES - 3;
    attron(A_BOLD | COLOR_PAIR(1));
    mvprintw(inputRow, 2, "┌");
    for (int i = 3; i < COLS - 3; i++) mvprintw(inputRow, i, "─");
    mvprintw(inputRow, COLS - 3, "┐");
    
    mvprintw(inputRow + 1, 2, "│ 🚀 > ");
    attron(A_UNDERLINE);
    mvprintw(inputRow + 1, 9, "%-*s", COLS - 13, inputBuffer.c_str());
    attroff(A_UNDERLINE);
    mvprintw(inputRow + 1, COLS - 3, "│");
    
    mvprintw(inputRow + 2, 2, "└");
    for (int i = 3; i < COLS - 3; i++) mvprintw(inputRow + 2, i, "─");
    mvprintw(inputRow + 2, COLS - 3, "┘");
    attroff(A_BOLD | COLOR_PAIR(1));
    
    mvprintw(LINES - 1, 2, "[ENTER] Execute on all selected | [ESC] Back");
}

void UI::drawMain() {
    // Draw futuristic header with animated effect
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    
    mvprintw(0, 2, "║ ⚡ QUANTUM SERVER CONTROL NEXUS 2050 ⚡ ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    attron(COLOR_PAIR(3));
    mvprintw(2, 2, "⌨  ↑/↓=Navigate | SPACE=Quantum-Select | ENTER=Access | R=Sync | ESC=Terminate");
    attroff(COLOR_PAIR(3));
    
    auto& servers = tm.getServers();
    auto& tasks = tm.getTasks();
    auto& selected = tm.selectedServers();

    for (int i = 0; i < servers.size(); i++) {
        int runningTasks = 0;
        for (auto& t : tasks)
            if (t.server == servers[i].name && t.state == TaskState::RUNNING)
                runningTasks++;

        auto st = tm.getServerStats(servers[i]);
        
        int row = 4 + i * 3;
        
        // Highlight selected server with futuristic glow
        if (i == selectedServer) {
            attron(COLOR_PAIR(5) | A_BOLD);
        }

        // Draw server box with enhanced design
        mvprintw(row, 2, "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
        
        // Checkbox and server name with holographic indicator
        mvprintw(row + 1, 2, "┃ [");
        if (selected[i]) {
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(row + 1, 5, "✓");
            attroff(COLOR_PAIR(1) | A_BOLD);
        } else {
            mvprintw(row + 1, 5, " ");
        }
        mvprintw(row + 1, 6, "] ");
        
        // Server name with status hologram
        if (st.cpu >= 0) {
            attron(COLOR_PAIR(1)); // Online - green hologram
            mvprintw(row + 1, 9, "◉");
            attroff(COLOR_PAIR(1));
        } else {
            attron(COLOR_PAIR(2)); // Offline - red
            mvprintw(row + 1, 9, "◉");
            attroff(COLOR_PAIR(2));
        }
        
        mvprintw(row + 1, 11, "%-15s", servers[i].name.c_str());
        mvprintw(row + 1, 27, "┃ ");
        
        // Neural network stats display
        if (st.cpu >= 0) {
            // CPU neural indicator
            int cpuColor = st.cpu > 80 ? 2 : (st.cpu > 50 ? 3 : 1);
            attron(COLOR_PAIR(cpuColor) | A_BOLD);
            mvprintw(row + 1, 29, "⚡CPU:%5.1f%%", st.cpu);
            attroff(COLOR_PAIR(cpuColor) | A_BOLD);
            
            // RAM biometric display
            int ramPercent = st.ramTotal > 0 ? (st.ramUsed > LONG_MAX / 100 
                ? (st.ramUsed / st.ramTotal) * 100 
                : (st.ramUsed * 100) / st.ramTotal) : 0;
            int ramColor = ramPercent > 80 ? 2 : (ramPercent > 50 ? 3 : 1);
            attron(COLOR_PAIR(ramColor) | A_BOLD);
            mvprintw(row + 1, 42, "🧠RAM:%ld/%ldMB", st.ramUsed, st.ramTotal);
            attroff(COLOR_PAIR(ramColor) | A_BOLD);
            
            attron(COLOR_PAIR(4));
            mvprintw(row + 1, 60, "⚙%dTask", runningTasks);
            attroff(COLOR_PAIR(4));
        } else {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(row + 1, 29, "⚠ NEURAL LINK OFFLINE");
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
        
        mvprintw(row + 1, 67, "┃");
        mvprintw(row + 2, 2, "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");

        if (i == selectedServer) {
            attroff(COLOR_PAIR(5) | A_BOLD);
        }
    }
    
    // Draw futuristic footer with quantum stats
    int footerRow = LINES - 2;
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(footerRow, 2, "⚡ Quantum-Selected Nodes: ");
    int count = 0;
    for (bool s : selected) if (s) count++;
    mvprintw(footerRow, 30, "%d", count);
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    attron(COLOR_PAIR(4));
    mvprintw(footerRow, 35, "│ 🌐 Neural Network: ACTIVE");
    attroff(COLOR_PAIR(4));
}

void UI::drawServerMenu() {
    auto& s = tm.getServers()[selectedServer];
    
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ ⚡ NEURAL COMMAND CENTER: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    mvprintw(4, 6, "┌────────────────────────────────────────┐");
    mvprintw(5, 6, "│  [1] 💻 Quantum Terminal               │");
    mvprintw(6, 6, "│  [2] 📋 Neural Log Viewer              │");
    mvprintw(7, 6, "│  [3] 🗑️  Process Terminator             │");
    mvprintw(8, 6, "│  [4] 📊 Biometric Stats Monitor        │");
    mvprintw(9, 6, "│  [5] 📁 Holographic File System        │");
    mvprintw(10, 6, "│  [6] 🚀 Multi-Server Orchestrator      │");
    mvprintw(11, 6, "└────────────────────────────────────────┘");
    
    attron(COLOR_PAIR(3));
    mvprintw(13, 6, "⌨  [ESC] Exit to main dashboard");
    attroff(COLOR_PAIR(3));
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
    
    // Prevent overflow in percentage calculation
    long ramPercent = 0;
    if (st.ramTotal > 0) {
        ramPercent = (st.ramUsed > LONG_MAX / 100) 
            ? (st.ramUsed / st.ramTotal) * 100 
            : (st.ramUsed * 100) / st.ramTotal;
    }
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
        if (ch == '5') {
            mode = Mode::SERVER_FILES;
            selectedFile = 0;
        }
        if (ch == '6') mode = Mode::MULTI_SERVER_EXEC;
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
    
    else if (mode == Mode::SERVER_FILES) {
        auto files = tm.listFiles(servers[selectedServer]);
        
        if (ch == KEY_UP && selectedFile > 0) selectedFile--;
        if (ch == KEY_DOWN && selectedFile < files.size() - 1) selectedFile++;
        
        // File operations
        if (ch == 'n' || ch == 'N') {
            // New file
            mode = Mode::FILE_EDITOR;
            currentEditFile = "newfile.txt";
            fileEditorLines.clear();
            fileEditorLines.push_back("");
            editorCursorRow = 0;
            editorCursorCol = 0;
        }
        if (ch == 'e' || ch == 'E') {
            // Edit file
            if (selectedFile < files.size() && !files[selectedFile].is_dir) {
                currentEditFile = files[selectedFile].name;
                std::string content = tm.readFile(servers[selectedServer], currentEditFile);
                fileEditorLines.clear();
                std::istringstream iss(content);
                std::string line;
                while (std::getline(iss, line)) {
                    fileEditorLines.push_back(line);
                }
                if (fileEditorLines.empty()) fileEditorLines.push_back("");
                mode = Mode::FILE_EDITOR;
                editorCursorRow = 0;
                editorCursorCol = 0;
            }
        }
        if (ch == 'd' || ch == 'D') {
            // Delete file
            if (selectedFile < files.size()) {
                tm.deleteFile(servers[selectedServer], files[selectedFile].name);
            }
        }
        if (ch == 'c' || ch == 'C') {
            // Copy file
            if (selectedFile < files.size()) {
                clipboardFile = files[selectedFile].name;
                clipboardIsCut = false;
            }
        }
        if (ch == 'x' || ch == 'X') {
            // Cut file
            if (selectedFile < files.size()) {
                clipboardFile = files[selectedFile].name;
                clipboardIsCut = true;
            }
        }
        if (ch == 'v' || ch == 'V') {
            // Paste file
            if (!clipboardFile.empty()) {
                std::string content = tm.readFile(servers[selectedServer], clipboardFile);
                std::string newName = "copy_of_" + clipboardFile;
                tm.writeFile(servers[selectedServer], newName, content);
                if (clipboardIsCut) {
                    tm.deleteFile(servers[selectedServer], clipboardFile);
                    clipboardFile.clear();
                }
            }
        }
        
        if (ch == 27) mode = Mode::SERVER_MENU;
    }
    
    else if (mode == Mode::FILE_EDITOR) {
        // Editor controls
        if (ch == 19) { // Ctrl+S
            // Save file
            std::string content;
            for (const auto& line : fileEditorLines) {
                content += line + "\n";
            }
            tm.writeFile(servers[selectedServer], currentEditFile, content);
            mode = Mode::SERVER_FILES;
        }
        else if (ch == 17 || ch == 27) { // Ctrl+Q or ESC
            // Quit without saving
            mode = Mode::SERVER_FILES;
        }
        else if (ch == KEY_UP && editorCursorRow > 0) {
            editorCursorRow--;
            if (editorCursorRow < editorScrollOffset) editorScrollOffset--;
        }
        else if (ch == KEY_DOWN && editorCursorRow < fileEditorLines.size() - 1) {
            editorCursorRow++;
            if (editorCursorRow >= editorScrollOffset + (LINES - 6)) editorScrollOffset++;
        }
        else if (ch == '\n') {
            // Insert new line
            std::string currentLine = fileEditorLines[editorCursorRow];
            std::string afterCursor = currentLine.substr(editorCursorCol);
            fileEditorLines[editorCursorRow] = currentLine.substr(0, editorCursorCol);
            fileEditorLines.insert(fileEditorLines.begin() + editorCursorRow + 1, afterCursor);
            editorCursorRow++;
            editorCursorCol = 0;
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (editorCursorCol > 0) {
                fileEditorLines[editorCursorRow].erase(editorCursorCol - 1, 1);
                editorCursorCol--;
            } else if (editorCursorRow > 0) {
                // Join with previous line
                std::string currentLine = fileEditorLines[editorCursorRow];
                editorCursorRow--;
                editorCursorCol = fileEditorLines[editorCursorRow].length();
                fileEditorLines[editorCursorRow] += currentLine;
                fileEditorLines.erase(fileEditorLines.begin() + editorCursorRow + 1);
            }
        }
        else if (isprint(ch)) {
            fileEditorLines[editorCursorRow].insert(editorCursorCol, 1, ch);
            editorCursorCol++;
        }
    }
    
    else if (mode == Mode::MULTI_SERVER_EXEC) {
        if (ch == '\n' && !inputBuffer.empty()) {
            commandHistory.push_back(inputBuffer);
            
            // Execute on all selected servers
            for (size_t i = 0; i < servers.size(); i++) {
                if (tm.selectedServers()[i]) {
                    tm.runCommandOnServer(servers[i], inputBuffer);
                }
            }
            inputBuffer.clear();
        }
        else if (ch == 27) mode = Mode::MAIN;
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            if (!inputBuffer.empty()) inputBuffer.pop_back();
        else if (isprint(ch)) inputBuffer.push_back(ch);
    }
}
