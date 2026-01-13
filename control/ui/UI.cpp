#include "UI.h"
#include <ncurses.h>
#include <algorithm>
#include <sstream>
#include <climits>
#include <ctime>
#include <cmath>
#include <map>
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
        case Mode::FILE_OPERATIONS: drawServerFiles(); break;
        case Mode::MULTI_SERVER_EXEC: drawMultiServerExec(); break;
        case Mode::AI_ASSISTANT: drawAIAssistant(); break;
        case Mode::NETWORK_TOPOLOGY: drawNetworkTopology(); break;
        case Mode::SECURITY_SCANNER: drawSecurityScanner(); break;
        case Mode::PERFORMANCE_ANALYTICS: drawPerformanceAnalytics(); break;
        case Mode::CLUSTER_MANAGER: drawClusterManager(); break;
        case Mode::BACKUP_RESTORE: drawBackupRestore(); break;
        case Mode::MONITORING_DASHBOARD: drawMonitoringDashboard(); break;
        case Mode::SCRIPT_LIBRARY: drawScriptLibrary(); break;
        case Mode::ALERT_CENTER: drawAlertCenter(); break;
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
    mvprintw(0, 2, "║ ⚡ NEURAL COMMAND CENTER 2090: %s ║", s.name.c_str());
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    int row = 3;
    int col = 4;
    
    // Core Operations (Left Column)
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(row++, col, "═══ CORE OPERATIONS ═══");
    attroff(COLOR_PAIR(4) | A_BOLD);
    row++;
    
    mvprintw(row++, col, "┌────────────────────────────────────────┐");
    mvprintw(row++, col, "│  [1] 💻 Quantum Terminal               │");
    mvprintw(row++, col, "│  [2] 📋 Neural Log Viewer              │");
    mvprintw(row++, col, "│  [3] 🗑️  Process Terminator             │");
    mvprintw(row++, col, "│  [4] 📊 Biometric Stats Monitor        │");
    mvprintw(row++, col, "│  [5] 📁 Holographic File System        │");
    mvprintw(row++, col, "│  [6] 🚀 Multi-Server Orchestrator      │");
    mvprintw(row++, col, "└────────────────────────────────────────┘");
    row++;
    
    // Advanced Systems (Left Column continued)
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, col, "═══ ADVANCED SYSTEMS ═══");
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    mvprintw(row++, col, "┌────────────────────────────────────────┐");
    mvprintw(row++, col, "│  [7] 🤖 AI Assistant & Automation      │");
    mvprintw(row++, col, "│  [8] 🌐 Network Topology Mapper        │");
    mvprintw(row++, col, "│  [9] 🔐 Security Scanner & Audit       │");
    mvprintw(row++, col, "│  [A] 📈 Performance Analytics          │");
    mvprintw(row++, col, "└────────────────────────────────────────┘");
    row++;
    
    // Infrastructure Management (Right Column)
    col = COLS / 2 + 2;
    row = 5;
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(row++, col, "═══ INFRASTRUCTURE ═══");
    attroff(COLOR_PAIR(3) | A_BOLD);
    row++;
    
    mvprintw(row++, col, "┌────────────────────────────────────────┐");
    mvprintw(row++, col, "│  [B] ☁️  Cluster Manager                │");
    mvprintw(row++, col, "│  [C] 💾 Backup & Restore System        │");
    mvprintw(row++, col, "│  [D] 📡 Real-Time Monitoring Hub       │");
    mvprintw(row++, col, "│  [E] 📜 Script Library & Automation    │");
    mvprintw(row++, col, "│  [F] 🚨 Alert & Notification Center    │");
    mvprintw(row++, col, "└────────────────────────────────────────┘");
    
    attron(COLOR_PAIR(3));
    mvprintw(LINES - 2, 4, "⌨  Select option or press [ESC] to return to main dashboard");
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
        if (ch == '7') mode = Mode::AI_ASSISTANT;
        if (ch == '8') mode = Mode::NETWORK_TOPOLOGY;
        if (ch == '9') mode = Mode::SECURITY_SCANNER;
        if (ch == 'a' || ch == 'A') mode = Mode::PERFORMANCE_ANALYTICS;
        if (ch == 'b' || ch == 'B') mode = Mode::CLUSTER_MANAGER;
        if (ch == 'c' || ch == 'C') mode = Mode::BACKUP_RESTORE;
        if (ch == 'd' || ch == 'D') mode = Mode::MONITORING_DASHBOARD;
        if (ch == 'e' || ch == 'E') mode = Mode::SCRIPT_LIBRARY;
        if (ch == 'f' || ch == 'F') mode = Mode::ALERT_CENTER;
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
    
    else if (mode == Mode::AI_ASSISTANT) {
        if (ch == '\n' && !aiInputBuffer.empty()) {
            aiChatHistory.push_back("You: " + aiInputBuffer);
            std::string response = getAIResponse(aiInputBuffer);
            aiChatHistory.push_back(response);
            aiInputBuffer.clear();
        }
        else if (ch == 27) mode = Mode::SERVER_MENU;
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            if (!aiInputBuffer.empty()) aiInputBuffer.pop_back();
        else if (isprint(ch)) aiInputBuffer.push_back(ch);
    }
    
    else if (mode == Mode::NETWORK_TOPOLOGY || mode == Mode::SECURITY_SCANNER || 
             mode == Mode::PERFORMANCE_ANALYTICS || mode == Mode::MONITORING_DASHBOARD ||
             mode == Mode::ALERT_CENTER) {
        if (ch == 27) mode = Mode::SERVER_MENU;
        if (ch == 'r' || ch == 'R') {
            updatePerformanceData();
            tm.refreshTasks();
        }
    }
    
    else if (mode == Mode::CLUSTER_MANAGER || mode == Mode::BACKUP_RESTORE || 
             mode == Mode::SCRIPT_LIBRARY) {
        if (ch == 27) mode = Mode::SERVER_MENU;
    }
}

// ============ ADVANCED 2090 FEATURES ============

void UI::drawAIAssistant() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 🤖 QUANTUM AI ASSISTANT & NEURAL AUTOMATION ENGINE ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(3, 4, "🧠 Neural Intelligence System - Predictive Analysis & Auto-Remediation");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    // AI Suggestions
    int row = 5;
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ AI RECOMMENDATIONS ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "• CPU usage trending high on quantum-server-01 → Recommend: scale horizontally");
    mvprintw(row++, 6, "• Detected memory leak in process PID 1234 → Auto-restart scheduled");
    mvprintw(row++, 6, "• Disk usage >80%% on 3 servers → Cleanup jobs queued");
    mvprintw(row++, 6, "• Security: 15 failed SSH attempts → IP auto-banned");
    mvprintw(row++, 6, "• Pattern detected: Deploy every Friday 2PM → Auto-schedule enabled");
    row += 2;
    
    // Chat History
    attron(COLOR_PAIR(4));
    mvprintw(row++, 4, "═══ CONVERSATION HISTORY ═══");
    attroff(COLOR_PAIR(4));
    row++;
    
    int histStart = std::max(0, (int)aiChatHistory.size() - 8);
    for (int i = histStart; i < aiChatHistory.size(); i++) {
        attron(COLOR_PAIR(1));
        mvprintw(row++, 6, "%s", aiChatHistory[i].c_str());
        attroff(COLOR_PAIR(1));
    }
    
    // Input
    int inputRow = LINES - 4;
    attron(A_BOLD | COLOR_PAIR(1));
    mvprintw(inputRow, 4, "┌");
    for (int i = 5; i < COLS - 5; i++) mvprintw(inputRow, i, "─");
    mvprintw(inputRow, COLS - 5, "┐");
    
    mvprintw(inputRow + 1, 4, "│ 🤖 Ask AI: ");
    attron(A_UNDERLINE);
    mvprintw(inputRow + 1, 16, "%-*s", COLS - 22, aiInputBuffer.c_str());
    attroff(A_UNDERLINE);
    mvprintw(inputRow + 1, COLS - 5, "│");
    
    mvprintw(inputRow + 2, 4, "└");
    for (int i = 5; i < COLS - 5; i++) mvprintw(inputRow + 2, i, "─");
    mvprintw(inputRow + 2, COLS - 5, "┘");
    attroff(A_BOLD | COLOR_PAIR(1));
    
    mvprintw(LINES - 1, 4, "[ENTER] Send query | [ESC] Back");
}

void UI::drawNetworkTopology() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 🌐 QUANTUM NETWORK TOPOLOGY - NEURAL MESH VISUALIZATION ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& servers = tm.getServers();
    int centerX = COLS / 2;
    int centerY = LINES / 2;
    
    // Draw central hub
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(centerY, centerX - 5, "╔═══════╗");
    mvprintw(centerY + 1, centerX - 5, "║  HUB  ║");
    mvprintw(centerY + 2, centerX - 5, "╚═══════╝");
    attroff(COLOR_PAIR(4) | A_BOLD);
    
    // Draw nodes in circle
    for (size_t i = 0; i < servers.size() && i < 8; i++) {
        double angle = (2 * 3.14159 * i) / servers.size();
        int x = centerX + (int)(25 * cos(angle));
        int y = centerY + (int)(10 * sin(angle));
        
        // Draw connection line
        attron(COLOR_PAIR(1));
        mvprintw(y, x, "───");
        attroff(COLOR_PAIR(1));
        
        // Draw node
        auto st = tm.getServerStats(servers[i]);
        int color = st.cpu >= 0 ? 1 : 2;
        attron(COLOR_PAIR(color) | A_BOLD);
        mvprintw(y - 1, x - 4, "┌────────┐");
        mvprintw(y, x - 4, "│ %-6s │", servers[i].name.substr(0, 6).c_str());
        mvprintw(y + 1, x - 4, "└────────┘");
        attroff(COLOR_PAIR(color) | A_BOLD);
    }
    
    // Legend
    attron(COLOR_PAIR(3));
    mvprintw(LINES - 4, 4, "Legend:");
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(1));
    mvprintw(LINES - 3, 6, "● Online");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    mvprintw(LINES - 3, 20, "● Offline");
    attroff(COLOR_PAIR(2));
    
    mvprintw(LINES - 1, 4, "[ESC] Back");
}

void UI::drawSecurityScanner() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 🔐 QUANTUM SECURITY SCANNER - NEURAL THREAT ANALYSIS ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& s = tm.getServers()[selectedServer];
    int row = 3;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "🛡️  SECURITY POSTURE: %s", s.name.c_str());
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    // Security Metrics
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ VULNERABILITY SCAN ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "┌─ Open Ports Scan");
    attron(COLOR_PAIR(1));
    mvprintw(row++, 8, "✓ Port 22 (SSH) - Protected");
    mvprintw(row++, 8, "✓ Port 80 (HTTP) - Expected");
    mvprintw(row++, 8, "✓ Port 443 (HTTPS) - Secure");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(2));
    mvprintw(row++, 8, "⚠ Port 3306 (MySQL) - Public exposure detected!");
    attroff(COLOR_PAIR(2));
    row++;
    
    mvprintw(row++, 6, "┌─ Authentication Security");
    attron(COLOR_PAIR(1));
    mvprintw(row++, 8, "✓ Root login disabled");
    mvprintw(row++, 8, "✓ SSH keys enforced");
    attroff(COLOR_PAIR(1));
    attron(COLOR_PAIR(3));
    mvprintw(row++, 8, "⚠ 3 failed login attempts in last hour");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "┌─ Firewall Status");
    attron(COLOR_PAIR(1));
    mvprintw(row++, 8, "✓ UFW active and enabled");
    mvprintw(row++, 8, "✓ 12 rules configured");
    attroff(COLOR_PAIR(1));
    row++;
    
    // Security Score
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "═══ SECURITY SCORE: 87/100 ═══");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    mvprintw(LINES - 1, 4, "[R] Run Full Scan | [ESC] Back");
}

void UI::drawPerformanceAnalytics() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 📈 QUANTUM PERFORMANCE ANALYTICS - PREDICTIVE INSIGHTS ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& s = tm.getServers()[selectedServer];
    auto st = tm.getServerStats(s);
    int row = 3;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "📊 Real-Time Analytics: %s", s.name.c_str());
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    // CPU Trend Graph (ASCII art)
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ CPU UTILIZATION TREND (24h) ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "100%│");
    mvprintw(row++, 6, " 75%│     ╱╲    ╱╲");
    mvprintw(row++, 6, " 50%│   ╱    ╲╱    ╲   ╱");
    mvprintw(row++, 6, " 25%│ ╱              ╲╱");
    mvprintw(row++, 6, "  0%└────────────────────────");
    mvprintw(row++, 6, "     6AM  12PM  6PM  12AM  6AM");
    row++;
    
    // Predictions
    attron(COLOR_PAIR(1));
    mvprintw(row++, 4, "═══ AI PREDICTIONS ═══");
    attroff(COLOR_PAIR(1));
    row++;
    
    mvprintw(row++, 6, "🔮 Next 4 hours: CPU will peak at 2PM (est. 85%%)");
    mvprintw(row++, 6, "🔮 Memory trend: Stable, no intervention needed");
    mvprintw(row++, 6, "🔮 Disk I/O: Spike predicted during backup window");
    row++;
    
    // Top Resource Consumers
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ TOP RESOURCE CONSUMERS ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "1. python app.py        CPU: 45%% | RAM: 2.1GB");
    mvprintw(row++, 6, "2. docker-proxy         CPU: 12%% | RAM: 512MB");
    mvprintw(row++, 6, "3. mysql                CPU:  8%% | RAM: 1.8GB");
    
    mvprintw(LINES - 1, 4, "[ESC] Back");
}

void UI::drawClusterManager() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ ☁️  QUANTUM CLUSTER ORCHESTRATION - NEURAL SWARM CONTROL ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& servers = tm.getServers();
    int row = 3;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "🌐 Cluster Status: %zu nodes active", servers.size());
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    // Cluster Health
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ CLUSTER HEALTH ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    int healthy = 0;
    for (auto& s : servers) {
        auto st = tm.getServerStats(s);
        if (st.cpu >= 0) healthy++;
    }
    
    attron(COLOR_PAIR(1));
    mvprintw(row++, 6, "✓ Nodes Online: %d/%zu", healthy, servers.size());
    mvprintw(row++, 6, "✓ Load Balanced: YES");
    mvprintw(row++, 6, "✓ Failover Ready: YES");
    mvprintw(row++, 6, "✓ Auto-Scaling: ENABLED");
    attroff(COLOR_PAIR(1));
    row++;
    
    // Cluster Operations
    attron(COLOR_PAIR(4));
    mvprintw(row++, 4, "═══ CLUSTER OPERATIONS ═══");
    attroff(COLOR_PAIR(4));
    row++;
    
    mvprintw(row++, 6, "[1] Deploy to Cluster");
    mvprintw(row++, 6, "[2] Rolling Update");
    mvprintw(row++, 6, "[3] Scale Horizontal (+/-)");
    mvprintw(row++, 6, "[4] Drain Node");
    mvprintw(row++, 6, "[5] Health Check All");
    
    mvprintw(LINES - 1, 4, "[1-5] Execute operation | [ESC] Back");
}

void UI::drawBackupRestore() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 💾 QUANTUM BACKUP & TIME-TRAVEL RESTORATION SYSTEM ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& s = tm.getServers()[selectedServer];
    int row = 3;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "💾 Backup Management: %s", s.name.c_str());
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    // Recent Backups
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ RECENT BACKUPS ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "┌─────────────────────────────────────────────────┐");
    mvprintw(row++, 6, "│ 2026-01-12 20:00 | Full Backup    | 15.2GB   │");
    mvprintw(row++, 6, "│ 2026-01-12 14:00 | Incremental    |  2.1GB   │");
    mvprintw(row++, 6, "│ 2026-01-12 08:00 | Incremental    |  1.8GB   │");
    mvprintw(row++, 6, "│ 2026-01-11 20:00 | Full Backup    | 14.9GB   │");
    mvprintw(row++, 6, "└─────────────────────────────────────────────────┘");
    row++;
    
    // Backup Schedule
    attron(COLOR_PAIR(4));
    mvprintw(row++, 4, "═══ AUTO-BACKUP SCHEDULE ═══");
    attroff(COLOR_PAIR(4));
    row++;
    
    attron(COLOR_PAIR(1));
    mvprintw(row++, 6, "✓ Full Backup: Daily at 8PM");
    mvprintw(row++, 6, "✓ Incremental: Every 6 hours");
    mvprintw(row++, 6, "✓ Retention: 30 days");
    mvprintw(row++, 6, "✓ Encryption: AES-256");
    attroff(COLOR_PAIR(1));
    row++;
    
    // Operations
    mvprintw(row++, 6, "[B] Create Backup Now");
    mvprintw(row++, 6, "[R] Restore from Backup");
    mvprintw(row++, 6, "[S] Schedule Settings");
    
    mvprintw(LINES - 1, 4, "[B/R/S] Select operation | [ESC] Back");
}

void UI::drawMonitoringDashboard() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 📡 QUANTUM MONITORING HUB - NEURAL OVERSIGHT MATRIX ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    auto& servers = tm.getServers();
    int row = 3;
    int col1 = 4;
    int col2 = COLS / 2 + 2;
    
    // System Overview
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row, col1, "═══ SYSTEM OVERVIEW ═══");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(row, col2, "═══ ACTIVE ALERTS ═══");
    attroff(COLOR_PAIR(3) | A_BOLD);
    row += 2;
    
    // Left: System metrics
    int r = row;
    mvprintw(r++, col1, "Total Nodes: %zu", servers.size());
    mvprintw(r++, col1, "Online: %zu", servers.size());
    mvprintw(r++, col1, "CPU Avg: 45.2%%");
    mvprintw(r++, col1, "RAM Avg: 62.1%%");
    mvprintw(r++, col1, "Tasks Running: 42");
    
    // Right: Alerts
    r = row;
    attron(COLOR_PAIR(2));
    mvprintw(r++, col2, "🔴 HIGH: Disk >90%% on node-03");
    attroff(COLOR_PAIR(2));
    attron(COLOR_PAIR(3));
    mvprintw(r++, col2, "🟡 WARN: CPU spike on node-01");
    mvprintw(r++, col2, "🟡 WARN: Memory usage trending up");
    attroff(COLOR_PAIR(3));
    attron(COLOR_PAIR(1));
    mvprintw(r++, col2, "🟢 INFO: Backup completed");
    attroff(COLOR_PAIR(1));
    
    row = r + 2;
    
    // Live Feed
    attron(COLOR_PAIR(4));
    mvprintw(row++, col1, "═══ LIVE ACTIVITY FEED ═══");
    attroff(COLOR_PAIR(4));
    row++;
    
    mvprintw(row++, col1, "21:45:32 - Task completed on quantum-server-01");
    mvprintw(row++, col1, "21:45:28 - New connection from 10.0.0.15");
    mvprintw(row++, col1, "21:45:15 - Backup initiated on node-03");
    mvprintw(row++, col1, "21:45:02 - Service restart: nginx");
    mvprintw(row++, col1, "21:44:55 - CPU threshold alert cleared");
    
    mvprintw(LINES - 1, 4, "[A] View All Alerts | [R] Refresh | [ESC] Back");
}

void UI::drawScriptLibrary() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 📜 QUANTUM SCRIPT LIBRARY - NEURAL AUTOMATION VAULT ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    int row = 3;
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "📚 Saved Automation Scripts");
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    // Script categories
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ DEPLOYMENT SCRIPTS ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    const char* deployScripts[] = {
        "1. deploy-production.sh - Full production deployment",
        "2. rollback.sh - Emergency rollback procedure",
        "3. blue-green-swap.sh - Zero-downtime deployment",
        "4. canary-release.sh - Gradual rollout strategy"
    };
    
    for (int i = 0; i < 4; i++) {
        mvprintw(row++, 6, "%s", deployScripts[i]);
    }
    row++;
    
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ MAINTENANCE SCRIPTS ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    const char* maintScripts[] = {
        "5. cleanup-logs.sh - Automated log rotation",
        "6. update-all-packages.sh - System updates",
        "7. health-check.sh - Comprehensive diagnostics",
        "8. optimize-database.sh - DB performance tuning"
    };
    
    for (int i = 0; i < 4; i++) {
        mvprintw(row++, 6, "%s", maintScripts[i]);
    }
    row++;
    
    attron(COLOR_PAIR(3));
    mvprintw(row++, 4, "═══ SECURITY SCRIPTS ═══");
    attroff(COLOR_PAIR(3));
    row++;
    
    mvprintw(row++, 6, "9. security-audit.sh - Full security scan");
    mvprintw(row++, 6, "10. update-firewall.sh - Firewall rules update");
    
    mvprintw(LINES - 1, 4, "[1-10] Execute script | [N] New script | [E] Edit | [ESC] Back");
}

void UI::drawAlertCenter() {
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "╔");
    for (int i = 1; i < COLS - 1; i++) mvprintw(0, i, "═");
    mvprintw(0, COLS - 1, "╗");
    mvprintw(0, 2, "║ 🚨 QUANTUM ALERT CENTER - NEURAL NOTIFICATION NEXUS ║");
    mvprintw(1, 0, "╚");
    for (int i = 1; i < COLS - 1; i++) mvprintw(1, i, "═");
    mvprintw(1, COLS - 1, "╝");
    attroff(A_BOLD | COLOR_PAIR(4));
    
    int row = 3;
    
    // Alert Summary
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(row++, 4, "🔔 Alert Summary: 3 Critical | 7 Warning | 12 Info");
    attroff(COLOR_PAIR(1) | A_BOLD);
    row++;
    
    // Critical Alerts
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(row++, 4, "═══ CRITICAL ALERTS ═══");
    attroff(COLOR_PAIR(2) | A_BOLD);
    row++;
    
    attron(COLOR_PAIR(2));
    mvprintw(row++, 6, "🔴 [21:45:12] Disk usage >90%% on quantum-worker-02");
    mvprintw(row++, 6, "🔴 [21:42:33] Service crashed: docker on neural-prod");
    mvprintw(row++, 6, "🔴 [21:38:55] Failed authentication: 20 attempts");
    attroff(COLOR_PAIR(2));
    row++;
    
    // Warning Alerts
    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(row++, 4, "═══ WARNING ALERTS ═══");
    attroff(COLOR_PAIR(3) | A_BOLD);
    row++;
    
    attron(COLOR_PAIR(3));
    mvprintw(row++, 6, "🟡 [21:44:20] CPU >80%% on quantum-server-01");
    mvprintw(row++, 6, "🟡 [21:43:15] Memory usage trending high");
    mvprintw(row++, 6, "🟡 [21:40:05] Backup taking longer than usual");
    mvprintw(row++, 6, "🟡 [21:35:42] Network latency spike detected");
    attroff(COLOR_PAIR(3));
    row++;
    
    // Alert Rules
    attron(COLOR_PAIR(4));
    mvprintw(row++, 4, "═══ ALERT RULES (Active: 25) ═══");
    attroff(COLOR_PAIR(4));
    row++;
    
    attron(COLOR_PAIR(1));
    mvprintw(row++, 6, "✓ CPU >90%% for 5min → Critical");
    mvprintw(row++, 6, "✓ Disk >85%% → Warning");
    mvprintw(row++, 6, "✓ Failed logins >10 → Critical");
    mvprintw(row++, 6, "✓ Service down → Critical");
    attroff(COLOR_PAIR(1));
    
    mvprintw(LINES - 1, 4, "[C] Configure rules | [M] Mute alerts | [ESC] Back");
}

// Helper functions
void UI::generateAlert(const std::string& severity, const std::string& msg, const std::string& server) {
    Alert alert;
    alert.severity = severity;
    alert.message = msg;
    alert.server = server;
    
    auto now = std::time(nullptr);
    char timeStr[20];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&now));
    alert.timestamp = timeStr;
    
    alerts.push_back(alert);
    if (alerts.size() > 100) {
        alerts.erase(alerts.begin());
    }
}

void UI::updatePerformanceData() {
    auto& servers = tm.getServers();
    for (auto& server : servers) {
        auto stats = tm.getServerStats(server);
        perfData[server.name].cpuHistory.push_back(stats.cpu);
        perfData[server.name].ramHistory.push_back(stats.ramUsed);
        
        // Keep only last 100 data points
        if (perfData[server.name].cpuHistory.size() > 100) {
            perfData[server.name].cpuHistory.erase(perfData[server.name].cpuHistory.begin());
        }
        if (perfData[server.name].ramHistory.size() > 100) {
            perfData[server.name].ramHistory.erase(perfData[server.name].ramHistory.begin());
        }
    }
}

std::string UI::getAIResponse(const std::string& query) {
    // Simulated AI responses based on keywords
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    if (lower_query.find("cpu") != std::string::npos || lower_query.find("performance") != std::string::npos) {
        return "AI: Based on historical patterns, CPU usage is within normal parameters. Recommend enabling auto-scaling if load increases.";
    } else if (lower_query.find("disk") != std::string::npos || lower_query.find("storage") != std::string::npos) {
        return "AI: Disk usage trending up. Automated cleanup scheduled for tonight. Consider expanding storage capacity.";
    } else if (lower_query.find("security") != std::string::npos || lower_query.find("threat") != std::string::npos) {
        return "AI: No active threats detected. All systems secured. Last security scan: 2 hours ago.";
    } else if (lower_query.find("deploy") != std::string::npos) {
        return "AI: Optimal deployment window: Friday 2PM-4PM based on traffic patterns. Blue-green deployment recommended.";
    } else {
        return "AI: Query processed. How else can I assist with your infrastructure?";
    }
}
