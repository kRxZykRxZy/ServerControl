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
        std::string body;
        if(buf.size()) std::getline(request, body);

        if(method=="POST" && path=="/exec"){
            json j=json::parse(body);
            std::string cmd=j["cmd"];
            Task t; t.command=cmd; t.id=std::to_string(++task_counter);
            {
                std::lock_guard<std::mutex> lk(tasks_mtx);
                tasks[t.id]=std::move(t);
                tasks[t.id].thread = std::thread(runTask,std::ref(tasks[t.id]));
                tasks[t.id].thread.detach();
            }
            asio::write(socket, asio::buffer(http_response(json{{"task_id", t.id}}.dump())));
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
