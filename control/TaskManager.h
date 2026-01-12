#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "Server.h"
#include "Task.h"

class TaskManager {
public:
    TaskManager(const std::vector<Server>& servers);

    // ---- server selection ----
    void toggleServer(size_t index);
    const std::vector<bool>& selectedServers() const;

    // ---- task execution ----
    void runCommand(const std::string& cmd);
    void runJointCommand(const std::string& cmd);

    // ---- task management ----
    void refreshTasks();
    const std::vector<Task>& getTasks() const;

    std::string getLogs(const Task& task);
    void killTask(const Task& task);

    // ---- server access ----
    const std::vector<Server>& getServers() const;
    std::vector<Task> getTasksForServer(const Server& s) const;
    void runCommandOnServer(const Server& s, const std::string& cmd);
    
    // ---- server stats ----
    struct ServerStats {
        double cpu;
        long ramUsed;
        long ramTotal;
    };
    ServerStats getServerStats(const Server& s) const;
    
    // ---- file management ----
    struct FileInfo {
        std::string name;
        bool is_dir;
        size_t size;
        std::string modified;
    };
    std::vector<FileInfo> listFiles(const Server& s);
    bool uploadFile(const Server& s, const std::string& localPath, const std::string& remoteName);
    bool downloadFile(const Server& s, const std::string& remoteName, const std::string& localPath);
    bool deleteFile(const Server& s, const std::string& filename);
    bool renameFile(const Server& s, const std::string& oldName, const std::string& newName);
    bool createFile(const Server& s, const std::string& filename, const std::string& content);
    std::string readFile(const Server& s, const std::string& filename);
    bool writeFile(const Server& s, const std::string& filename, const std::string& content);
    
    // ---- system control ----
    struct ProcessInfo {
        std::string pid;
        std::string user;
        std::string cpu;
        std::string mem;
        std::string command;
    };
    
    struct ServiceInfo {
        std::string name;
        std::string load;
        std::string active;
        std::string sub;
    };
    
    struct DockerInfo {
        std::string id;
        std::string name;
        std::string status;
        std::string image;
    };
    
    struct SystemInfo {
        std::string os;
        std::string kernel;
        std::string uptime;
        std::string disk;
        std::vector<std::string> network;
    };
    
    bool shutdownServer(const Server& s);
    bool rebootServer(const Server& s);
    SystemInfo getSystemInfo(const Server& s);
    std::vector<ProcessInfo> listProcesses(const Server& s);
    bool killProcess(const Server& s, const std::string& pid, const std::string& signal = "15");
    std::vector<ServiceInfo> listServices(const Server& s);
    bool controlService(const Server& s, const std::string& service, const std::string& action);
    std::vector<DockerInfo> listDockerContainers(const Server& s);
    bool controlDocker(const Server& s, const std::string& container, const std::string& action);
    std::string getSystemLogs(const Server& s, int lines = 50);

private:
    std::vector<Server> servers;
    std::vector<bool> selected;
    std::vector<Task> tasks;

    void execOnServer(
        const Server& server,
        const std::string& cmd,
        const std::unordered_map<std::string, std::string>& env
    );
};
