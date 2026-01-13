#include "Task.h"
#include "../network/WebSocket.h"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <chrono>
#include <sys/wait.h>

using json = nlohmann::json;

extern void broadcast_ws(const std::string& message);

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
