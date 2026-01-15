#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace Files {

struct FileInfo {
    std::string name;
    bool is_directory;
    size_t size;
    std::string modified;
};

class Manager {
public:
    static Manager& Instance();
    
    // File listing
    std::vector<FileInfo> List(const std::string& path = "./storage");
    
    // File operations
    bool Upload(const std::string& filename, const std::string& base64Content, bool autoInstall = false);
    bool Download(const std::string& filename, std::string& base64Content);
    bool Delete(const std::string& filename);
    bool Rename(const std::string& oldName, const std::string& newName);
    
    // File content operations
    bool Read(const std::string& filename, std::string& content);
    bool Write(const std::string& filename, const std::string& content);
    
    // Utility
    std::string SanitizeFilename(const std::string& filename);
    std::string Base64Encode(const std::string& input);
    std::string Base64Decode(const std::string& input);
    
private:
    Manager() = default;
    std::string storage_path_ = "./storage";
    bool EnsureStorageExists();
};

} // namespace Files
