#include <asio.hpp>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <atomic>
#include <cstdlib>
#include <csignal>
#include <nlohmann/json.hpp>
#include <chrono>
#include <condition_variable>
#include <unistd.h>
#include <filesystem>
#include <iomanip>
#include <set>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using asio::ip::tcp;
using asio::ip::udp;
using json = nlohmann::json;
namespace fs = std::filesystem;

typedef websocketpp::server<websocketpp::config::asio> websocket_server;
typedef websocket_server::message_ptr message_ptr;
using websocketpp::connection_hdl;

// Helper function for string ends_with (C++17 compatible)
bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// WebSocket connections management
std::set<connection_hdl, std::owner_less<connection_hdl>> ws_connections;
std::mutex ws_connections_mtx;
websocket_server ws_server;

// Broadcast message to all WebSocket connections
void broadcast_ws(const std::string& message) {
    std::lock_guard<std::mutex> lock(ws_connections_mtx);
    for (auto hdl : ws_connections) {
        try {
            ws_server.send(hdl, message, websocketpp::frame::opcode::text);
        } catch (...) {
            // Connection closed, will be cleaned up later
        }
    }
}

struct Task {
    std::string id;
    std::string command;
    std::string output;
    std::atomic<bool> running{true};
    std::thread thread;
    
    Task() = default;
    Task(Task&& other) noexcept 
        : id(std::move(other.id))
        , command(std::move(other.command))
        , output(std::move(other.output))
        , running(other.running.load())
        , thread(std::move(other.thread))
    {}
    
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            id = std::move(other.id);
            command = std::move(other.command);
            output = std::move(other.output);
            running.store(other.running.load());
            thread = std::move(other.thread);
        }
        return *this;
    }
    
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
};

std::map<std::string, Task> tasks;
std::mutex tasks_mtx;
int task_counter = 0;

struct Stats { double cpu; long ram_used; long ram_total; };

Stats getStats() {
    static long lastTotal = 0, lastIdle = 0;
    std::ifstream stat("/proc/stat");
    std::string cpu;
    long user, nice, system, idle;
    stat >> cpu >> user >> nice >> system >> idle;
    long total = user + nice + system + idle;
    long totald = total - lastTotal;
    long idled = idle - lastIdle;
    lastTotal = total; lastIdle = idle;
    double cpu_usage = totald ? (100.0*(totald - idled)/totald) : 0;
    std::ifstream mem("/proc/meminfo");
    long totalMem, freeMem;
    mem.ignore(256, ':'); mem >> totalMem;
    mem.ignore(256, ':'); mem >> freeMem;
    return {cpu_usage, (totalMem - freeMem)/1024, totalMem/1024};
}

void runTask(Task& t) {
    FILE* pipe = popen(t.command.c_str(), "r");
    if(!pipe){ 
        t.running=false; 
        json msg = {{"type", "task_complete"}, {"task_id", t.id}, {"exit_code", -1}};
        broadcast_ws(msg.dump());
        return; 
    }
    
    // Send task started event
    json start_msg = {{"type", "task_start"}, {"task_id", t.id}, {"command", t.command}};
    broadcast_ws(start_msg.dump());
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string output_chunk(buffer);
        t.output += output_chunk;
        
        // Stream output in real-time via WebSocket
        json output_msg = {
            {"type", "task_output"}, 
            {"task_id", t.id}, 
            {"output", output_chunk},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()}
        };
        broadcast_ws(output_msg.dump());
    }
    
    int exit_code = pclose(pipe);
    t.running = false;
    
    // Send task completed event
    json complete_msg = {
        {"type", "task_complete"}, 
        {"task_id", t.id}, 
        {"exit_code", WEXITSTATUS(exit_code)}
    };
    broadcast_ws(complete_msg.dump());
}

std::string http_response(const std::string& body){
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "Content-Type: application/json\r\n"
       << "Connection: close\r\n\r\n"
       << body;
    return ss.str();
}

std::string get_hostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return std::string(hostname);
    }
    return "unknown";
}

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

void handle_client(tcp::socket socket) {
    try{
        asio::streambuf buf;
        asio::read_until(socket, buf, "\r\n\r\n");
        std::istream request(&buf);
        std::string method, path, httpver;
        request >> method >> path >> httpver;
        
        // Parse headers to get Content-Length
        std::string line;
        std::getline(request, line); // consume newline after request line
        size_t content_length = 0;
        while (std::getline(request, line) && line != "\r") {
            // Convert to lowercase for case-insensitive comparison
            std::string lower_line = line;
            for (auto& c : lower_line) c = std::tolower(c);
            
            if (lower_line.find("content-length:") == 0) {
                // Find the colon and skip whitespace
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string value = line.substr(colon_pos + 1);
                    // Skip leading whitespace
                    size_t start = value.find_first_not_of(" \t\r\n");
                    if (start != std::string::npos) {
                        content_length = std::stoul(value.substr(start));
                    }
                }
            }
        }
        
        // Read body if present
        std::string body;
        if (content_length > 0) {
            // Check if body is already in buffer
            size_t available = buf.size();
            if (available < content_length) {
                // Read remaining body
                asio::read(socket, buf, asio::transfer_exactly(content_length - available));
            }
            std::ostringstream ss;
            ss << &buf;
            body = ss.str();
            if (body.size() > content_length) {
                body = body.substr(0, content_length);
            }
        }

        if(method=="POST" && path=="/exec"){
            json j=json::parse(body);
            std::string cmd=j["cmd"];
            std::string task_id = std::to_string(++task_counter);
            Task t; 
            t.command=cmd; 
            t.id=task_id;
            {
                std::lock_guard<std::mutex> lk(tasks_mtx);
                tasks[task_id]=std::move(t);
                tasks[task_id].thread = std::thread(runTask,std::ref(tasks[task_id]));
                tasks[task_id].thread.detach();
            }
            asio::write(socket, asio::buffer(http_response(json{{"task_id", task_id}}.dump())));
        }
        else if(method=="GET" && path=="/tasks"){
            json j = json::array();
            std::lock_guard<std::mutex> lk(tasks_mtx);
            for(auto& [id,t]:tasks)
                j.push_back({{"id",id},{"running",t.running.load()},{"command",t.command}});
            asio::write(socket, asio::buffer(http_response(j.dump())));
        }
        else if(method=="GET" && path.find("/logs")==0){
            std::string id;
            auto pos = path.find("id=");
            if(pos!=std::string::npos) id=path.substr(pos+3);
            std::string out;
            std::lock_guard<std::mutex> lk(tasks_mtx);
            if(tasks.count(id)) out=tasks[id].output;
            asio::write(socket, asio::buffer(http_response(json{{"logs", out}}.dump())));
        }
        else if(method=="POST" && path.find("/kill")==0){
            std::string id;
            auto pos = path.find("id=");
            if(pos!=std::string::npos) id=path.substr(pos+3);
            std::lock_guard<std::mutex> lk(tasks_mtx);
            if(tasks.count(id)){
                std::thread([id](){
                    std::string cmd="pkill -P "+id;
                    system(cmd.c_str());
                }).detach();
            }
            asio::write(socket, asio::buffer(http_response(json{{"killed",id}}.dump())));
        }
        else if(method=="GET" && path=="/stats"){
            auto s=getStats();
            asio::write(socket, asio::buffer(http_response(json{
                {"cpu",s.cpu},{"ram_used",s.ram_used},{"ram_total",s.ram_total}
            }.dump())));
        }
        else if(method=="GET" && path=="/hostname"){
            asio::write(socket, asio::buffer(http_response(json{
                {"hostname", get_hostname()}
            }.dump())));
        }
        else if(method=="GET" && path=="/files/list"){
            // List files in storage directory
            std::string storage_path = "./storage";
            
            // Create storage directory if it doesn't exist
            if (!fs::exists(storage_path)) {
                fs::create_directories(storage_path);
            }
            
            json files = json::array();
            try {
                for (const auto& entry : fs::directory_iterator(storage_path)) {
                    json file_info;
                    file_info["name"] = entry.path().filename().string();
                    file_info["is_dir"] = entry.is_directory();
                    file_info["size"] = entry.is_regular_file() ? fs::file_size(entry.path()) : 0;
                    
                    auto ftime = fs::last_write_time(entry.path());
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                    );
                    auto time = std::chrono::system_clock::to_time_t(sctp);
                    
                    std::ostringstream oss;
                    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
                    file_info["modified"] = oss.str();
                    
                    files.push_back(file_info);
                }
            } catch (...) {}
            
            asio::write(socket, asio::buffer(http_response(json{{"files", files}}.dump())));
        }
        else if(method=="GET" && path.find("/files/download")==0){
            // Download a file
            auto pos = path.find("name=");
            if(pos != std::string::npos) {
                std::string filename = path.substr(pos + 5);
                std::string filepath = "./storage/" + filename;
                
                if (fs::exists(filepath) && fs::is_regular_file(filepath)) {
                    std::ifstream file(filepath, std::ios::binary);
                    std::string content((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                    file.close();
                    
                    std::string encoded = base64_encode(content);
                    asio::write(socket, asio::buffer(http_response(json{
                        {"content", encoded},
                        {"filename", filename}
                    }.dump())));
                } else {
                    asio::write(socket, asio::buffer(http_response(json{{"error", "File not found"}}.dump())));
                }
            } else {
                asio::write(socket, asio::buffer(http_response(json{{"error", "No filename"}}.dump())));
            }
        }
        else if(method=="POST" && path=="/files/upload"){
            // Upload a file with intelligent auto-install detection
            try {
                json j = json::parse(body);
                std::string filename = j["filename"];
                std::string content_b64 = j["content"];
                bool auto_install = j.value("auto_install", false);
                
                std::string storage_path = "./storage";
                if (!fs::exists(storage_path)) {
                    fs::create_directories(storage_path);
                }
                
                std::string filepath = storage_path + "/" + filename;
                std::string content = base64_decode(content_b64);
                
                std::ofstream file(filepath, std::ios::binary);
                file.write(content.c_str(), content.size());
                file.close();
                
                json response = {
                    {"success", true},
                    {"message", "File uploaded successfully"},
                    {"filename", filename}
                };
                
                // Auto-install detection based on file extension
                if (auto_install) {
                    std::string install_cmd;
                    std::string file_type;
                    
                    // Detect file type and determine installation command
                    if (ends_with(filename, ".deb")) {
                        install_cmd = "sudo dpkg -i " + filepath;
                        file_type = "Debian Package";
                    } else if (ends_with(filename, ".rpm")) {
                        install_cmd = "sudo rpm -i " + filepath;
                        file_type = "RPM Package";
                    } else if (ends_with(filename, ".AppImage")) {
                        install_cmd = "chmod +x " + filepath;
                        file_type = "AppImage";
                    } else if (ends_with(filename, ".sh")) {
                        install_cmd = "chmod +x " + filepath + " && " + filepath;
                        file_type = "Shell Script";
                    } else if (ends_with(filename, ".tar.gz") || ends_with(filename, ".tgz")) {
                        install_cmd = "tar -xzf " + filepath + " -C " + storage_path;
                        file_type = "Tarball";
                    } else if (ends_with(filename, ".zip")) {
                        install_cmd = "unzip " + filepath + " -d " + storage_path;
                        file_type = "ZIP Archive";
                    } else if (ends_with(filename, ".py")) {
                        // Check if it's a Python package setup file
                        install_cmd = "pip3 install " + filepath;
                        file_type = "Python Package";
                    }
                    
                    if (!install_cmd.empty()) {
                        // Execute installation in background
                        std::string task_id = std::to_string(++task_counter);
                        Task t;
                        t.command = install_cmd;
                        t.id = task_id;
                        
                        {
                            std::lock_guard<std::mutex> lk(tasks_mtx);
                            tasks[task_id] = std::move(t);
                            tasks[task_id].thread = std::thread(runTask, std::ref(tasks[task_id]));
                            tasks[task_id].thread.detach();
                        }
                        
                        response["auto_install"] = true;
                        response["file_type"] = file_type;
                        response["install_task_id"] = task_id;
                        response["install_command"] = install_cmd;
                    } else {
                        response["auto_install"] = false;
                        response["message"] = "File uploaded but auto-install not supported for this file type";
                    }
                }
                
                asio::write(socket, asio::buffer(http_response(response.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="POST" && path=="/files/delete"){
            // Delete a file
            try {
                json j = json::parse(body);
                std::string filename = j["filename"];
                std::string filepath = "./storage/" + filename;
                
                if (fs::exists(filepath)) {
                    fs::remove(filepath);
                    asio::write(socket, asio::buffer(http_response(json{
                        {"success", true},
                        {"message", "File deleted successfully"}
                    }.dump())));
                } else {
                    asio::write(socket, asio::buffer(http_response(json{
                        {"success", false},
                        {"error", "File not found"}
                    }.dump())));
                }
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="POST" && path=="/files/rename"){
            // Rename a file
            try {
                json j = json::parse(body);
                std::string oldname = j["oldname"];
                std::string newname = j["newname"];
                std::string oldpath = "./storage/" + oldname;
                std::string newpath = "./storage/" + newname;
                
                if (fs::exists(oldpath)) {
                    fs::rename(oldpath, newpath);
                    asio::write(socket, asio::buffer(http_response(json{
                        {"success", true},
                        {"message", "File renamed successfully"}
                    }.dump())));
                } else {
                    asio::write(socket, asio::buffer(http_response(json{
                        {"success", false},
                        {"error", "File not found"}
                    }.dump())));
                }
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="GET" && path.find("/files/read")==0){
            // Read file content
            auto pos = path.find("name=");
            if(pos != std::string::npos) {
                std::string filename = path.substr(pos + 5);
                std::string filepath = "./storage/" + filename;
                
                if (fs::exists(filepath) && fs::is_regular_file(filepath)) {
                    std::ifstream file(filepath);
                    std::string content((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
                    file.close();
                    
                    asio::write(socket, asio::buffer(http_response(json{
                        {"content", content},
                        {"filename", filename}
                    }.dump())));
                } else {
                    asio::write(socket, asio::buffer(http_response(json{{"error", "File not found"}}.dump())));
                }
            } else {
                asio::write(socket, asio::buffer(http_response(json{{"error", "No filename"}}.dump())));
            }
        }
        else if(method=="POST" && path=="/files/write"){
            // Write/update file content
            try {
                json j = json::parse(body);
                std::string filename = j["filename"];
                std::string content = j["content"];
                
                std::string storage_path = "./storage";
                if (!fs::exists(storage_path)) {
                    fs::create_directories(storage_path);
                }
                
                std::string filepath = storage_path + "/" + filename;
                
                std::ofstream file(filepath);
                file.write(content.c_str(), content.size());
                file.close();
                
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", true},
                    {"message", "File saved successfully"}
                }.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="POST" && path=="/system/shutdown"){
            // Shutdown the server
            asio::write(socket, asio::buffer(http_response(json{
                {"success", true},
                {"message", "Shutdown initiated"}
            }.dump())));
            
            std::thread([](){
                sleep(2);
                system("sudo shutdown -h now");
            }).detach();
        }
        else if(method=="POST" && path=="/system/reboot"){
            // Reboot the server
            asio::write(socket, asio::buffer(http_response(json{
                {"success", true},
                {"message", "Reboot initiated"}
            }.dump())));
            
            std::thread([](){
                sleep(2);
                system("sudo reboot");
            }).detach();
        }
        else if(method=="GET" && path=="/system/info"){
            // Get comprehensive system information
            json sysinfo;
            
            // OS info
            std::ifstream os_release("/etc/os-release");
            std::string line;
            while (std::getline(os_release, line)) {
                if (line.find("PRETTY_NAME=") == 0) {
                    sysinfo["os"] = line.substr(13, line.length() - 14);
                    break;
                }
            }
            
            // Kernel version
            FILE* pipe = popen("uname -r", "r");
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                sysinfo["kernel"] = std::string(buffer).substr(0, strlen(buffer)-1);
            }
            pclose(pipe);
            
            // Uptime
            pipe = popen("uptime -p", "r");
            if (fgets(buffer, sizeof(buffer), pipe)) {
                sysinfo["uptime"] = std::string(buffer).substr(0, strlen(buffer)-1);
            }
            pclose(pipe);
            
            // Load average
            std::ifstream loadavg("/proc/loadavg");
            double load1, load5, load15;
            loadavg >> load1 >> load5 >> load15;
            sysinfo["load_avg"] = {load1, load5, load15};
            
            // Disk usage
            pipe = popen("df -h / | tail -1 | awk '{print $2,$3,$4,$5}'", "r");
            if (fgets(buffer, sizeof(buffer), pipe)) {
                sysinfo["disk"] = std::string(buffer).substr(0, strlen(buffer)-1);
            }
            pclose(pipe);
            
            // Network interfaces
            pipe = popen("ip -br addr | awk '{print $1,$3}'", "r");
            json interfaces = json::array();
            while (fgets(buffer, sizeof(buffer), pipe)) {
                interfaces.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            sysinfo["network"] = interfaces;
            
            asio::write(socket, asio::buffer(http_response(sysinfo.dump())));
        }
        else if(method=="GET" && path=="/system/processes"){
            // List all processes
            json processes = json::array();
            
            FILE* pipe = popen("ps aux | tail -n +2", "r");
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string line(buffer);
                std::istringstream iss(line);
                std::string user, pid, cpu, mem, vsz, rss, tty, stat, start, time, command;
                iss >> user >> pid >> cpu >> mem >> vsz >> rss >> tty >> stat >> start >> time;
                std::getline(iss, command);
                
                json proc;
                proc["pid"] = pid;
                proc["user"] = user;
                proc["cpu"] = cpu;
                proc["mem"] = mem;
                proc["command"] = command;
                processes.push_back(proc);
            }
            pclose(pipe);
            
            asio::write(socket, asio::buffer(http_response(json{{"processes", processes}}.dump())));
        }
        else if(method=="POST" && path=="/system/kill-process"){
            // Kill a specific process
            try {
                json j = json::parse(body);
                std::string pid = j["pid"];
                std::string signal = j.value("signal", "15");
                
                std::string cmd = "kill -" + signal + " " + pid;
                int result = system(cmd.c_str());
                
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", result == 0},
                    {"message", result == 0 ? "Process killed" : "Failed to kill process"}
                }.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="GET" && path=="/system/services"){
            // List systemd services
            json services = json::array();
            
            FILE* pipe = popen("systemctl list-units --type=service --all --no-pager", "r");
            char buffer[512];
            bool header_passed = false;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string line(buffer);
                if (!header_passed) {
                    if (line.find("UNIT") != std::string::npos) header_passed = true;
                    continue;
                }
                if (line.empty() || line[0] == '\n') continue;
                
                std::istringstream iss(line);
                std::string unit, load, active, sub;
                iss >> unit >> load >> active >> sub;
                
                json svc;
                svc["name"] = unit;
                svc["load"] = load;
                svc["active"] = active;
                svc["sub"] = sub;
                services.push_back(svc);
            }
            pclose(pipe);
            
            asio::write(socket, asio::buffer(http_response(json{{"services", services}}.dump())));
        }
        else if(method=="POST" && path=="/system/service-control"){
            // Control systemd service (start/stop/restart)
            try {
                json j = json::parse(body);
                std::string service = j["service"];
                std::string action = j["action"]; // start, stop, restart, enable, disable
                
                std::string cmd = "sudo systemctl " + action + " " + service;
                int result = system(cmd.c_str());
                
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", result == 0},
                    {"message", result == 0 ? "Service " + action + " successful" : "Failed to " + action + " service"}
                }.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="GET" && path=="/system/docker"){
            // List Docker containers if docker is available
            json containers = json::array();
            
            FILE* pipe = popen("docker ps -a --format '{{.ID}}|{{.Names}}|{{.Status}}|{{.Image}}' 2>/dev/null", "r");
            if (pipe) {
                char buffer[512];
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    std::string line(buffer);
                    line = line.substr(0, line.length() - 1);
                    
                    size_t pos1 = line.find('|');
                    size_t pos2 = line.find('|', pos1 + 1);
                    size_t pos3 = line.find('|', pos2 + 1);
                    
                    if (pos1 != std::string::npos && pos2 != std::string::npos) {
                        json cont;
                        cont["id"] = line.substr(0, pos1);
                        cont["name"] = line.substr(pos1 + 1, pos2 - pos1 - 1);
                        cont["status"] = line.substr(pos2 + 1, pos3 - pos2 - 1);
                        cont["image"] = pos3 != std::string::npos ? line.substr(pos3 + 1) : "";
                        containers.push_back(cont);
                    }
                }
                pclose(pipe);
            }
            
            asio::write(socket, asio::buffer(http_response(json{{"containers", containers}}.dump())));
        }
        else if(method=="POST" && path=="/system/docker-control"){
            // Control Docker container
            try {
                json j = json::parse(body);
                std::string container = j["container"];
                std::string action = j["action"]; // start, stop, restart, remove
                
                std::string cmd = "docker " + action + " " + container;
                int result = system(cmd.c_str());
                
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", result == 0},
                    {"message", result == 0 ? "Container " + action + " successful" : "Failed to " + action + " container"}
                }.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="GET" && path=="/system/logs"){
            // Get system logs
            auto pos = path.find("lines=");
            std::string lines = "50";
            if (pos != std::string::npos) {
                lines = path.substr(pos + 6);
                // Extract just the number before any other parameters
                size_t end = lines.find('&');
                if (end != std::string::npos) lines = lines.substr(0, end);
            }
            
            std::string cmd = "journalctl -n " + lines + " --no-pager";
            FILE* pipe = popen(cmd.c_str(), "r");
            std::string logs;
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                logs += buffer;
            }
            pclose(pipe);
            
            asio::write(socket, asio::buffer(http_response(json{{"logs", logs}}.dump())));
        }
        else if(method=="GET" && path=="/system/metrics"){
            // Advanced metrics collection
            json metrics;
            
            // CPU detailed info
            FILE* pipe = popen("lscpu | grep -E 'Model name|CPU\\(s\\)|Thread|Core'", "r");
            char buffer[256];
            std::vector<std::string> cpu_info;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                cpu_info.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            metrics["cpu_info"] = cpu_info;
            
            // Network statistics
            pipe = popen("ss -s 2>/dev/null || netstat -s 2>/dev/null | head -20", "r");
            std::vector<std::string> net_stats;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                net_stats.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            metrics["network_stats"] = net_stats;
            
            // Disk I/O
            pipe = popen("iostat -d 2>/dev/null | tail -n +4 | head -5", "r");
            std::vector<std::string> disk_io;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                disk_io.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            metrics["disk_io"] = disk_io;
            
            // Top processes by CPU
            pipe = popen("ps aux --sort=-%cpu | head -6 | tail -5", "r");
            std::vector<std::string> top_cpu;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                top_cpu.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            metrics["top_cpu"] = top_cpu;
            
            // Top processes by memory
            pipe = popen("ps aux --sort=-%mem | head -6 | tail -5", "r");
            std::vector<std::string> top_mem;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                top_mem.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            metrics["top_mem"] = top_mem;
            
            asio::write(socket, asio::buffer(http_response(metrics.dump())));
        }
        else if(method=="GET" && path=="/system/security-scan"){
            // Security vulnerability scan
            json security;
            
            // Check open ports
            FILE* pipe = popen("ss -tuln 2>/dev/null || netstat -tuln 2>/dev/null | grep LISTEN", "r");
            char buffer[256];
            std::vector<std::string> open_ports;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                open_ports.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            security["open_ports"] = open_ports;
            
            // Check for failed login attempts
            pipe = popen("grep 'Failed password' /var/log/auth.log 2>/dev/null | tail -10", "r");
            std::vector<std::string> failed_logins;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                failed_logins.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            security["failed_logins"] = failed_logins;
            
            // Check SSH configuration
            pipe = popen("grep -E 'PermitRootLogin|PasswordAuthentication' /etc/ssh/sshd_config 2>/dev/null", "r");
            std::vector<std::string> ssh_config;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                ssh_config.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            security["ssh_config"] = ssh_config;
            
            // Check firewall status
            pipe = popen("ufw status 2>/dev/null || iptables -L -n 2>/dev/null | head -20", "r");
            std::vector<std::string> firewall;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                firewall.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            security["firewall"] = firewall;
            
            asio::write(socket, asio::buffer(http_response(security.dump())));
        }
        else if(method=="GET" && path=="/system/network-info"){
            // Detailed network information
            json network;
            
            // Active connections
            FILE* pipe = popen("ss -tunap 2>/dev/null | head -20", "r");
            char buffer[256];
            std::vector<std::string> connections;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                connections.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            network["active_connections"] = connections;
            
            // Routing table
            pipe = popen("ip route", "r");
            std::vector<std::string> routes;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                routes.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            network["routes"] = routes;
            
            // Interface statistics
            pipe = popen("ip -s link", "r");
            std::vector<std::string> if_stats;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                if_stats.push_back(std::string(buffer).substr(0, strlen(buffer)-1));
            }
            pclose(pipe);
            network["interface_stats"] = if_stats;
            
            asio::write(socket, asio::buffer(http_response(network.dump())));
        }
        else if(method=="POST" && path=="/system/backup"){
            // Create system backup
            try {
                json j = json::parse(body);
                std::string path = j.value("path", "./storage");
                std::string backup_name = "backup_" + std::to_string(std::time(nullptr)) + ".tar.gz";
                
                std::string cmd = "tar -czf /tmp/" + backup_name + " " + path + " 2>&1";
                FILE* pipe = popen(cmd.c_str(), "r");
                char buffer[256];
                std::string output;
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    output += buffer;
                }
                int result = pclose(pipe);
                
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", result == 0},
                    {"backup_file", "/tmp/" + backup_name},
                    {"output", output}
                }.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="POST" && path=="/system/execute-script"){
            // Execute saved script
            try {
                json j = json::parse(body);
                std::string script_content = j["script"];
                std::string script_file = "/tmp/script_" + std::to_string(std::time(nullptr)) + ".sh";
                
                // Write script to file
                std::ofstream script(script_file);
                script << script_content;
                script.close();
                
                // Make executable
                chmod(script_file.c_str(), 0755);
                
                // Execute
                std::string output;
                FILE* pipe = popen(script_file.c_str(), "r");
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    output += buffer;
                }
                int result = pclose(pipe);
                
                // Cleanup
                std::remove(script_file.c_str());
                
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", result == 0},
                    {"output", output},
                    {"exit_code", result}
                }.dump())));
            } catch (const std::exception& e) {
                asio::write(socket, asio::buffer(http_response(json{
                    {"success", false},
                    {"error", e.what()}
                }.dump())));
            }
        }
        else if(method=="GET" && path=="/system/health-check"){
            // Comprehensive health check
            json health;
            health["timestamp"] = std::time(nullptr);
            
            // CPU health
            auto stats = getStats();
            health["cpu_usage"] = stats.cpu;
            health["cpu_healthy"] = stats.cpu < 90;
            
            // RAM health
            long ram_percent = stats.ram_total > 0 ? (stats.ram_used * 100 / stats.ram_total) : 0;
            health["ram_usage_percent"] = ram_percent;
            health["ram_healthy"] = ram_percent < 90;
            
            // Disk health
            FILE* pipe = popen("df -h / | tail -1 | awk '{print $5}' | sed 's/%//'", "r");
            char buffer[32];
            int disk_percent = 0;
            if (fgets(buffer, sizeof(buffer), pipe)) {
                disk_percent = std::atoi(buffer);
            }
            pclose(pipe);
            health["disk_usage_percent"] = disk_percent;
            health["disk_healthy"] = disk_percent < 90;
            
            // System load
            std::ifstream loadavg("/proc/loadavg");
            double load1;
            loadavg >> load1;
            health["load_average"] = load1;
            health["load_healthy"] = load1 < 4.0;
            
            // Overall health
            health["overall_healthy"] = 
                health["cpu_healthy"].get<bool>() && 
                health["ram_healthy"].get<bool>() && 
                health["disk_healthy"].get<bool>() && 
                health["load_healthy"].get<bool>();
            
            asio::write(socket, asio::buffer(http_response(health.dump())));
        }
        else{
            asio::write(socket, asio::buffer(http_response("{}")));
        }
    }catch(...){}
}

void discovery_responder() {
    try {
        asio::io_context io;
        udp::socket socket(io, udp::endpoint(udp::v4(), 8081));
        
        for(;;) {
            char recv_buf[128];
            udp::endpoint remote_endpoint;
            size_t len = socket.receive_from(asio::buffer(recv_buf), remote_endpoint);
            
            std::string msg(recv_buf, len);
            if (msg == "DISCOVER_SERVER") {
                json response = {
                    {"type", "SERVER_RESPONSE"},
                    {"hostname", get_hostname()},
                    {"port", 8080},
                    {"ws_port", 8082}
                };
                std::string resp_str = response.dump();
                socket.send_to(asio::buffer(resp_str), remote_endpoint);
            }
        }
    } catch(std::exception& e) {
        std::cerr << "Discovery error: " << e.what() << "\n";
    }
}

// CPU monitoring thread - broadcasts alerts when CPU > 90%
void cpu_monitor() {
    double last_alert_time = 0;
    const double alert_cooldown = 60.0; // Alert at most once per minute
    
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        auto stats = getStats();
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        // Broadcast stats update
        json stats_msg = {
            {"type", "stats_update"},
            {"cpu", stats.cpu},
            {"ram_used", stats.ram_used},
            {"ram_total", stats.ram_total},
            {"timestamp", timestamp}
        };
        broadcast_ws(stats_msg.dump());
        
        // Check for CPU alert (>90%)
        if (stats.cpu > 90.0) {
            double current_time = timestamp / 1000.0;
            if (current_time - last_alert_time > alert_cooldown) {
                json alert_msg = {
                    {"type", "cpu_alert"},
                    {"cpu", stats.cpu},
                    {"hostname", get_hostname()},
                    {"message", "CPU usage exceeded 90%!"},
                    {"timestamp", timestamp}
                };
                broadcast_ws(alert_msg.dump());
                last_alert_time = current_time;
                
                // Also log to console
                std::cout << "⚠️  CPU ALERT: " << stats.cpu << "% on " << get_hostname() << std::endl;
            }
        }
    }
}

// WebSocket connection handlers
void on_ws_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(ws_connections_mtx);
    ws_connections.insert(hdl);
    std::cout << "WebSocket client connected. Total clients: " << ws_connections.size() << std::endl;
}

void on_ws_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(ws_connections_mtx);
    ws_connections.erase(hdl);
    std::cout << "WebSocket client disconnected. Total clients: " << ws_connections.size() << std::endl;
}

void on_ws_message(websocket_server* server, connection_hdl hdl, message_ptr msg) {
    // Handle incoming WebSocket messages (if needed for bidirectional communication)
    try {
        json j = json::parse(msg->get_payload());
        std::string type = j["type"];
        
        if (type == "ping") {
            json pong = {{"type", "pong"}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()}};
            server->send(hdl, pong.dump(), websocketpp::frame::opcode::text);
        }
    } catch (...) {
        // Invalid message, ignore
    }
}

void run_websocket_server() {
    try {
        ws_server.set_access_channels(websocketpp::log::alevel::none);
        ws_server.set_error_channels(websocketpp::log::elevel::none);
        
        ws_server.init_asio();
        ws_server.set_reuse_addr(true);
        
        ws_server.set_open_handler(&on_ws_open);
        ws_server.set_close_handler(&on_ws_close);
        ws_server.set_message_handler(std::bind(&on_ws_message, &ws_server, std::placeholders::_1, std::placeholders::_2));
        
        ws_server.listen(8082);
        ws_server.start_accept();
        
        std::cout << "WebSocket server started on port 8082\n";
        
        ws_server.run();
    } catch (std::exception& e) {
        std::cerr << "WebSocket server error: " << e.what() << std::endl;
    }
}

int main(){
    try{
        // Start discovery responder in background
        std::thread(discovery_responder).detach();
        
        // Start CPU monitoring thread
        std::thread(cpu_monitor).detach();
        
        // Start WebSocket server in background
        std::thread(run_websocket_server).detach();
        
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "Server started on port 8080 (HTTP), 8081 (Discovery), and 8082 (WebSocket)\n";
        std::cout << "CPU monitoring active - will alert when CPU > 90%\n";
        for(;;){
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::thread(handle_client,std::move(socket)).detach();
        }
    }catch(std::exception& e){ std::cerr<<e.what(); }
}
