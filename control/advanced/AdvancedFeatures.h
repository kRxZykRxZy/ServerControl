// Advanced Control Features - Database Manager Module
#pragma once
#include <string>
#include <vector>
#include <map>

namespace ServerControl {
namespace Advanced {

// Database types supported
enum class DatabaseType {
    MYSQL,
    POSTGRESQL,
    MONGODB,
    REDIS,
    SQLITE,
    MSSQL,
    ORACLE
};

// Database connection info
struct DatabaseConnection {
    std::string server_id;
    DatabaseType type;
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string database_name;
    bool connected;
};

// Database query result
struct QueryResult {
    bool success;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> columns;
    int affected_rows;
    std::string error_message;
    double execution_time_ms;
};

// Database Manager Class
class DatabaseManager {
public:
    // Connection management
    bool connect(const DatabaseConnection& conn);
    bool disconnect(const std::string& connection_id);
    std::vector<DatabaseConnection> listConnections();
    
    // Query execution
    QueryResult executeQuery(const std::string& connection_id, const std::string& query);
    QueryResult executeTransaction(const std::string& connection_id, const std::vector<std::string>& queries);
    
    // Database operations
    std::vector<std::string> listDatabases(const std::string& connection_id);
    std::vector<std::string> listTables(const std::string& connection_id, const std::string& database);
    std::string getTableSchema(const std::string& connection_id, const std::string& table);
    
    // Backup and restore
    bool backupDatabase(const std::string& connection_id, const std::string& backup_path);
    bool restoreDatabase(const std::string& connection_id, const std::string& backup_path);
    
    // Performance
    std::map<std::string, std::string> getPerformanceStats(const std::string& connection_id);
    std::vector<std::string> getSlowQueries(const std::string& connection_id);
    
private:
    std::map<std::string, DatabaseConnection> connections_;
};

// Container Manager Class
class ContainerManager {
public:
    struct Container {
        std::string id;
        std::string name;
        std::string image;
        std::string status;
        std::vector<std::string> ports;
        long memory_usage;
        double cpu_usage;
    };
    
    // Container operations
    std::vector<Container> listContainers(const std::string& server_id);
    bool startContainer(const std::string& server_id, const std::string& container_id);
    bool stopContainer(const std::string& server_id, const std::string& container_id);
    bool restartContainer(const std::string& server_id, const std::string& container_id);
    bool removeContainer(const std::string& server_id, const std::string& container_id);
    
    // Container creation
    std::string createContainer(const std::string& server_id, 
                               const std::string& image,
                               const std::map<std::string, std::string>& env_vars,
                               const std::vector<std::string>& ports);
    
    // Logs and stats
    std::string getContainerLogs(const std::string& server_id, const std::string& container_id, int lines = 100);
    Container getContainerStats(const std::string& server_id, const std::string& container_id);
    
    // Image management
    std::vector<std::string> listImages(const std::string& server_id);
    bool pullImage(const std::string& server_id, const std::string& image);
    bool removeImage(const std::string& server_id, const std::string& image);
};

} // namespace Advanced
} // namespace ServerControl
