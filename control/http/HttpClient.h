#pragma once
#include <string>

class HttpClient {
public:
    static std::string post(const std::string& ip, int port,
                            const std::string& path,
                            const std::string& body);

    static std::string get(const std::string& ip, int port,
                           const std::string& path);
};

