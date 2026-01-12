// Compile: g++ server.cpp -O2 -std=c++20 -pthread -o agent

#include <asio.hpp>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <sstream>

using asio::ip::tcp;

struct Task {
    pid_t pid;
    std::string output;
    bool running;
};

std::unordered_map<std::string, Task> tasks;
std::mutex task_mutex;
int task_counter = 0;

std::string exec_command(const std::string& cmd, pid_t& out_pid) {
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(1);
    }

    close(pipefd[1]);
    out_pid = pid;

    std::stringstream ss;
    char buffer[1024];
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        ss.write(buffer, n);
    }
    close(pipefd[0]);

    waitpid(pid, nullptr, 0);
    return ss.str();
}

std::string http_response(const std::string& body) {
    return "HTTP/1.1 200 OK\r\nContent-Length: " +
           std::to_string(body.size()) +
           "\r\n\r\n" + body;
}

void handle_request(tcp::socket socket) {
    asio::streambuf buf;
    asio::read_until(socket, buf, "\r\n\r\n");

    std::istream request(&buf);
    std::string method, path;
    request >> method >> path;

    if (method == "POST" && path == "/exec") {
        std::string cmd((std::istreambuf_iterator<char>(request)), {});
        pid_t pid;

        std::thread([cmd, &pid]() {
            std::string output = exec_command(cmd, pid);
            std::lock_guard lock(task_mutex);
            for (auto& [id, task] : tasks) {
                if (task.pid == pid) {
                    task.output = output;
                    task.running = false;
                }
            }
        }).detach();

        std::lock_guard lock(task_mutex);
        std::string id = std::to_string(++task_counter);
        tasks[id] = {pid, "", true};

        asio::write(socket, asio::buffer(http_response("{\"task_id\":\"" + id + "\"}")));
    }

    else if (method == "GET" && path == "/tasks") {
        std::stringstream ss;
        ss << "[";
        std::lock_guard lock(task_mutex);
        for (auto& [id, t] : tasks) {
            ss << "{\"id\":\"" << id << "\",\"pid\":" << t.pid
               << ",\"running\":" << (t.running ? "true" : "false") << "},";
        }
        ss << "]";
        asio::write(socket, asio::buffer(http_response(ss.str())));
    }

    else if (method == "GET" && path.rfind("/logs?id=", 0) == 0) {
        std::string id = path.substr(9);
        std::lock_guard lock(task_mutex);
        asio::write(socket, asio::buffer(http_response(tasks[id].output)));
    }

    else if (method == "POST" && path.rfind("/kill?id=", 0) == 0) {
        std::string id = path.substr(9);
        std::lock_guard lock(task_mutex);
        kill(tasks[id].pid, SIGKILL);
        tasks[id].running = false;
        asio::write(socket, asio::buffer(http_response("killed")));
    }

    socket.close();
}

void daemonize() {
    if (fork() > 0) exit(0);
    setsid();
    if (fork() > 0) exit(0);

    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main() {
    daemonize();

    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 9001));

    while (true) {
        tcp::socket socket(io);
        acceptor.accept(socket);
        std::thread(handle_request, std::move(socket)).detach();
    }
}
