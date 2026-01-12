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
        SERVER_STATS,
        SERVER_FILES,
        FILE_EDITOR,
        FILE_OPERATIONS,
        MULTI_SERVER_EXEC
    } mode = Mode::MAIN;

    std::string inputBuffer;
    std::vector<std::string> commandHistory;
    int selectedTask = 0;
    int selectedFile = 0;
    std::string uploadFilePath;
    std::vector<std::string> fileEditorLines;
    std::string currentEditFile;
    int editorCursorRow = 0;
    int editorCursorCol = 0;
    int editorScrollOffset = 0;
    
    // Clipboard for copy/cut/paste
    std::string clipboardFile;
    bool clipboardIsCut = false;

    void draw();
    void handleInput(int ch);

    void drawMain();
    void drawServerMenu();
    void drawServerTerminal();
    void drawServerLogs();
    void drawServerKill();
    void drawServerStats();
    void drawServerFiles();
    void drawFileEditor();
    void drawMultiServerExec();
};
