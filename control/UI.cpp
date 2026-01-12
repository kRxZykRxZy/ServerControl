#include "UI.h"
#include "TaskManager.h"
#include <ncurses.h>

static TaskManager* tm = nullptr;

UI::UI(const std::vector<Server>& s)
    : servers(s), selected(s.size(), false) {}

void UI::draw() {
    clear();

    mvprintw(0, 2, "CLUSTER CONTROL PANEL");
    mvhline(1, 0, '-', COLS);

    mvprintw(2, 2, "Servers (SPACE to select):");
    for (int i = 0; i < servers.size(); i++) {
        mvprintw(3 + i, 4, "[%c] %d: %s",
                 selected[i] ? 'X' : ' ',
                 i,
                 servers[i].name.c_str());
    }

    int row = 4 + servers.size();
    mvhline(row++, 0, '-', COLS);

    mvprintw(row++, 2, "Running Tasks:");
    auto tasks = tm->fetchTasks();
    for (auto& t : tasks) {
        mvprintw(row++, 4, "%s | %s | %s",
                 t.id.c_str(),
                 t.server.c_str(),
                 t.command.c_str());
    }

    mvhline(row++, 0, '-', COLS);
    mvprintw(row++, 2, "Command:");
    mvprintw(row++, 4, "> %s", command.c_str());

    refresh();
}

void UI::handleInput(int ch) {
    if (ch == '\n') {
        tm->runOnSelected(command, selected);
        command.clear();
    }
    else if (ch == KEY_BACKSPACE || ch == 127) {
        if (!command.empty()) command.pop_back();
    }
    else if (ch == ' ') {
        int y, x;
        getyx(stdscr, y, x);
        int idx = y - 3;
        if (idx >= 0 && idx < selected.size())
            selected[idx] = !selected[idx];
    }
    else if (isprint(ch)) {
        command.push_back(ch);
    }
}

void UI::run() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    TaskManager manager(servers);
    tm = &manager;

    while (true) {
        draw();
        int ch = getch();
        if (ch == 27) break; // ESC exits
        handleInput(ch);
    }

    endwin();
}

