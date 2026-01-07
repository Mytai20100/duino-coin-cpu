#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>

struct PoolInfo {
    std::string ip;
    int port;
    std::string name;
};

class NetworkManager {
private:
    PoolInfo current_pool;
    
public:
    bool initialize();
    bool fetch_pool();
    PoolInfo get_pool() const { return current_pool; }
};

class SocketClient {
private:
    int sockfd;
    bool connected;
    
public:
    SocketClient();
    ~SocketClient();
    
    bool connect(const std::string& host, int port, int timeout);
    bool send(const std::string& data);
    std::string receive(int timeout);
    void disconnect();
    bool is_connected() const { return connected; }
};

#endif