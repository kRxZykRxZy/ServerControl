#include "TaskManager.h"
#include "HttpClient.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

TaskManager::TaskManager(const std::vector<Server>& s)
    : servers(s), selected(s.size(), false) {}


// ---------------- SERVER SELECTION ----------------

void TaskManager::toggleServer(size_t index) {
    if (index < selected.size())
        selected[index] = !selected[index];
}

const std::vector<bool>& TaskManager::selectedServers() const {
    return selected;
}


// ---------------- TASK EXECUTION ----------------

void TaskManager::execOnServer(
    const Server& server,
    const std::string& cmd,
    const std::unordered_map<std::string, std::string>& env
) {
    json payload;
    payload["cmd"] = cmd;
    payload["env"] = env;

    std::string res = HttpClient::post(
        server.ip,
        server.port,
        "/exec",
        payload.dump()
    );

    auto r = json::parse(res);
    std::string taskId = r["task_id"];

    tasks.push_back({
        taskId,
        server.name,
        cmd,
        TaskState::RUNNING
    });
}

void TaskManager::runCommand(const std::string& cmd) {
    for (size_t i = 0; i < servers.size(); i++) {
        if (!selected[i]) continue;
        execOnServer(servers[i], cmd, {});
    }
}

void TaskManager::runJointCommand(const std::string& cmd) {
    int total = 0;
    for (bool s : selected)
        if (s) total++;

    int workerId = 0;
    for (size_t i = 0; i < servers.size(); i++) {
        if (!selected[i]) continue;

        execOnServer(
            servers[i],
            cmd,
            {
                {"WORKER_ID", std::to_string(workerId++)},
                {"TOTAL_WORKERS", std::to_string(total)}
            }
        );
    }
}

// ---- ACCESSORS ----

const std::vector<Server>& TaskManager::getServers() const {
    return servers;
}

std::vector<Task> TaskManager::getTasksForServer(const Server& s) const {
    std::vector<Task> out;
    for (const auto& t : tasks) {
        if (t.server == s.name)
            out.push_back(t);
    }
    return out;
}

void TaskManager::runCommandOnServer(
    const Server& s,
    const std::string& cmd
) {
    execOnServer(s, cmd, {});
}

// ---------------- TASK TRACKING ----------------

void TaskManager::refreshTasks() {
    for (auto& s : servers) {
        std::string res = HttpClient::get(
            s.ip,
            s.port,
            "/tasks"
        );

        auto j = json::parse(res);
        for (auto& jt : j) {
            std::string id = jt["id"];
            bool running = jt["running"];

            for (auto& t : tasks) {
                if (t.id == id && t.server == s.name) {
                    t.state = running
                        ? TaskState::RUNNING
                        : TaskState::FINISHED;
                }
            }
        }
    }
}

const std::vector<Task>& TaskManager::getTasks() const {
    return tasks;
}


// ---------------- LOGS & CONTROL ----------------

std::string TaskManager::getLogs(const Task& task) {
    for (auto& s : servers) {
        if (s.name == task.server) {
            return HttpClient::get(
                s.ip,
                s.port,
                "/logs?id=" + task.id
            );
        }
    }
    return {};
}

void TaskManager::killTask(const Task& task) {
    for (auto& s : servers) {
        if (s.name == task.server) {
            HttpClient::post(
                s.ip,
                s.port,
                "/kill?id=" + task.id,
                ""
            );
        }
    }
}

// ---------------- SERVER STATS ----------------

TaskManager::ServerStats TaskManager::getServerStats(const Server& s) const {
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/stats");
        auto j = json::parse(res);
        return {
            j["cpu"].get<double>(),
            j["ram_used"].get<long>(),
            j["ram_total"].get<long>()
        };
    } catch (...) {
        return {0.0, 0, 0};
    }
}

// ---------------- FILE MANAGEMENT ----------------

std::string base64_encode(const std::string& input) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    
    for (size_t n = 0; n < input.size(); n++) {
        char_array_3[i++] = input[n];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while(i++ < 3)
            ret += '=';
    }
    
    return ret;
}

std::string base64_decode(const std::string& input) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string ret;
    int i = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    for (size_t n = 0; n < input.size() && input[n] != '='; n++) {
        if (!isalnum(input[n]) && input[n] != '+' && input[n] != '/') continue;
        
        char_array_4[i++] = input[n];
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;
        
        for (int j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (int j = 0; j < i - 1; j++)
            ret += char_array_3[j];
    }
    
    return ret;
}

std::vector<TaskManager::FileInfo> TaskManager::listFiles(const Server& s) {
    std::vector<FileInfo> files;
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/files/list");
        auto j = json::parse(res);
        
        if (j.contains("files") && j["files"].is_array()) {
            for (const auto& file : j["files"]) {
                FileInfo info;
                info.name = file.value("name", "");
                info.is_dir = file.value("is_dir", false);
                info.size = file.value("size", 0);
                info.modified = file.value("modified", "");
                files.push_back(info);
            }
        }
    } catch (...) {}
    return files;
}

bool TaskManager::uploadFile(const Server& s, const std::string& localPath, const std::string& remoteName) {
    try {
        std::ifstream file(localPath, std::ios::binary);
        if (!file.is_open()) return false;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        std::string encoded = base64_encode(content);
        
        json payload;
        payload["filename"] = remoteName;
        payload["content"] = encoded;
        
        std::string res = HttpClient::post(s.ip, s.port, "/files/upload", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

bool TaskManager::downloadFile(const Server& s, const std::string& remoteName, const std::string& localPath) {
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/files/download?name=" + remoteName);
        auto j = json::parse(res);
        
        if (!j.contains("content")) return false;
        
        std::string decoded = base64_decode(j["content"]);
        
        std::ofstream file(localPath, std::ios::binary);
        if (!file.is_open()) return false;
        
        file.write(decoded.c_str(), decoded.size());
        file.close();
        
        return true;
    } catch (...) {
        return false;
    }
}

bool TaskManager::deleteFile(const Server& s, const std::string& filename) {
    try {
        json payload;
        payload["filename"] = filename;
        
        std::string res = HttpClient::post(s.ip, s.port, "/files/delete", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

bool TaskManager::renameFile(const Server& s, const std::string& oldName, const std::string& newName) {
    try {
        json payload;
        payload["oldname"] = oldName;
        payload["newname"] = newName;
        
        std::string res = HttpClient::post(s.ip, s.port, "/files/rename", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

bool TaskManager::createFile(const Server& s, const std::string& filename, const std::string& content) {
    return writeFile(s, filename, content);
}

std::string TaskManager::readFile(const Server& s, const std::string& filename) {
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/files/read?name=" + filename);
        auto j = json::parse(res);
        
        if (j.contains("content")) {
            return j["content"];
        }
        return "";
    } catch (...) {
        return "";
    }
}

bool TaskManager::writeFile(const Server& s, const std::string& filename, const std::string& content) {
    try {
        json payload;
        payload["filename"] = filename;
        payload["content"] = content;
        
        std::string res = HttpClient::post(s.ip, s.port, "/files/write", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

// ---------------- SYSTEM CONTROL ----------------

bool TaskManager::shutdownServer(const Server& s) {
    try {
        std::string res = HttpClient::post(s.ip, s.port, "/system/shutdown", "{}");
        auto j = json::parse(res);
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

bool TaskManager::rebootServer(const Server& s) {
    try {
        std::string res = HttpClient::post(s.ip, s.port, "/system/reboot", "{}");
        auto j = json::parse(res);
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

TaskManager::SystemInfo TaskManager::getSystemInfo(const Server& s) {
    SystemInfo info;
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/system/info");
        auto j = json::parse(res);
        
        info.os = j.value("os", "Unknown");
        info.kernel = j.value("kernel", "Unknown");
        info.uptime = j.value("uptime", "Unknown");
        info.disk = j.value("disk", "Unknown");
        
        if (j.contains("network") && j["network"].is_array()) {
            for (const auto& net : j["network"]) {
                info.network.push_back(net);
            }
        }
    } catch (...) {}
    return info;
}

std::vector<TaskManager::ProcessInfo> TaskManager::listProcesses(const Server& s) {
    std::vector<ProcessInfo> processes;
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/system/processes");
        auto j = json::parse(res);
        
        if (j.contains("processes") && j["processes"].is_array()) {
            for (const auto& proc : j["processes"]) {
                ProcessInfo info;
                info.pid = proc.value("pid", "");
                info.user = proc.value("user", "");
                info.cpu = proc.value("cpu", "");
                info.mem = proc.value("mem", "");
                info.command = proc.value("command", "");
                processes.push_back(info);
            }
        }
    } catch (...) {}
    return processes;
}

bool TaskManager::killProcess(const Server& s, const std::string& pid, const std::string& signal) {
    try {
        json payload;
        payload["pid"] = pid;
        payload["signal"] = signal;
        
        std::string res = HttpClient::post(s.ip, s.port, "/system/kill-process", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

std::vector<TaskManager::ServiceInfo> TaskManager::listServices(const Server& s) {
    std::vector<ServiceInfo> services;
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/system/services");
        auto j = json::parse(res);
        
        if (j.contains("services") && j["services"].is_array()) {
            for (const auto& svc : j["services"]) {
                ServiceInfo info;
                info.name = svc.value("name", "");
                info.load = svc.value("load", "");
                info.active = svc.value("active", "");
                info.sub = svc.value("sub", "");
                services.push_back(info);
            }
        }
    } catch (...) {}
    return services;
}

bool TaskManager::controlService(const Server& s, const std::string& service, const std::string& action) {
    try {
        json payload;
        payload["service"] = service;
        payload["action"] = action;
        
        std::string res = HttpClient::post(s.ip, s.port, "/system/service-control", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

std::vector<TaskManager::DockerInfo> TaskManager::listDockerContainers(const Server& s) {
    std::vector<DockerInfo> containers;
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/system/docker");
        auto j = json::parse(res);
        
        if (j.contains("containers") && j["containers"].is_array()) {
            for (const auto& cont : j["containers"]) {
                DockerInfo info;
                info.id = cont.value("id", "");
                info.name = cont.value("name", "");
                info.status = cont.value("status", "");
                info.image = cont.value("image", "");
                containers.push_back(info);
            }
        }
    } catch (...) {}
    return containers;
}

bool TaskManager::controlDocker(const Server& s, const std::string& container, const std::string& action) {
    try {
        json payload;
        payload["container"] = container;
        payload["action"] = action;
        
        std::string res = HttpClient::post(s.ip, s.port, "/system/docker-control", payload.dump());
        auto j = json::parse(res);
        
        return j.value("success", false);
    } catch (...) {
        return false;
    }
}

std::string TaskManager::getSystemLogs(const Server& s, int lines) {
    try {
        std::string res = HttpClient::get(s.ip, s.port, "/system/logs?lines=" + std::to_string(lines));
        auto j = json::parse(res);
        
        if (j.contains("logs")) {
            return j["logs"];
        }
        return "";
    } catch (...) {
        return "";
    }
}
