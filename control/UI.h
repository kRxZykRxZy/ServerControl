#pragma once
#include <vector>
#include <string>
#include <map>
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
        MULTI_SERVER_EXEC,
        AI_ASSISTANT,
        NETWORK_TOPOLOGY,
        SECURITY_SCANNER,
        PERFORMANCE_ANALYTICS,
        CLUSTER_MANAGER,
        BACKUP_RESTORE,
        MONITORING_DASHBOARD,
        SCRIPT_LIBRARY,
        ALERT_CENTER
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
    
    // AI Assistant
    std::vector<std::string> aiChatHistory;
    std::string aiInputBuffer;
    
    // Network topology
    int selectedNode = 0;
    
    // Performance analytics
    struct PerformanceData {
        std::vector<double> cpuHistory;
        std::vector<long> ramHistory;
        std::vector<int> taskHistory;
    };
    std::map<std::string, PerformanceData> perfData;
    
    // Alert system
    struct Alert {
        std::string severity; // critical, warning, info
        std::string message;
        std::string timestamp;
        std::string server;
    };
    std::vector<Alert> alerts;
    
    // Script library
    std::vector<std::string> savedScripts;
    int selectedScript = 0;

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
    void drawAIAssistant();
    void drawNetworkTopology();
    void drawSecurityScanner();
    void drawPerformanceAnalytics();
    void drawClusterManager();
    void drawBackupRestore();
    void drawMonitoringDashboard();
    void drawScriptLibrary();
    void drawAlertCenter();
    
    // Helper functions
    void generateAlert(const std::string& severity, const std::string& msg, const std::string& server);
    void updatePerformanceData();
    std::string getAIResponse(const std::string& query);
};
