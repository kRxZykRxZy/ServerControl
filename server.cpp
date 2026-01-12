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

using asio::ip::tcp;
using asio::ip::udp;
using json = nlohmann::json;

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
    if(!pipe){ t.running=false; return; }
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        t.output += buffer;
    }
    pclose(pipe);
    t.running = false;
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
                    {"port", 8080}
                };
                std::string resp_str = response.dump();
                socket.send_to(asio::buffer(resp_str), remote_endpoint);
            }
        }
    } catch(std::exception& e) {
        std::cerr << "Discovery error: " << e.what() << "\n";
    }
}

int main(){
    try{
        // Start discovery responder in background
        std::thread(discovery_responder).detach();
        
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
        std::cout << "Server started on port 8080 (HTTP) and 8081 (Discovery)\n";
        for(;;){
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::thread(handle_client,std::move(socket)).detach();
        }
    }catch(std::exception& e){ std::cerr<<e.what(); }
}
