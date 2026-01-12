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

using asio::ip::tcp;
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
        else{
            asio::write(socket, asio::buffer(http_response("{}")));
        }
    }catch(...){}
}

int main(){
    try{
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
        for(;;){
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::thread(handle_client,std::move(socket)).detach();
        }
    }catch(std::exception& e){ std::cerr<<e.what(); }
}
