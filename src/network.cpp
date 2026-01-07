#include "../include/network.h"
#include "../include/logger.h"
#include <sys/socket.h>
#include "../include/http_client.h"
#include "../include/json.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>

bool NetworkManager::initialize() {
    return true;
}

bool NetworkManager::fetch_pool() {
    std::string response = HttpClient::get("https://server.duinocoin.com/getPool");
    
    if (response.empty()) {
        Logger::error("Failed to fetch pool information");
        return false;
    }
    
    if (!Json::get_bool(response, "success")) {
        Logger::error("Pool picker returned error");
        return false;
    }
    
    current_pool.ip = Json::get_value(response, "ip");
    current_pool.port = Json::get_int(response, "port");
    current_pool.name = Json::get_value(response, "name");
    
    if (current_pool.ip.empty() || current_pool.port == 0) {
        Logger::error("Invalid pool data received");
        return false;
    }
    
    Logger::info("Selected pool: " + current_pool.name + " (" + 
                 current_pool.ip + ":" + std::to_string(current_pool.port) + ")");
    
    return true;
}

SocketClient::SocketClient() : sockfd(-1), connected(false) {}

SocketClient::~SocketClient() {
    disconnect();
}

bool SocketClient::connect(const std::string& host, int port, int timeout) {
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string port_str = std::to_string(port);
    
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &servinfo) != 0) {
        return false;
    }
    
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        
        if (::connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) {
            connected = true;
            break;
        }
        
        close(sockfd);
        sockfd = -1;
    }
    
    freeaddrinfo(servinfo);
    return connected;
}

bool SocketClient::send(const std::string& data) {
    if (!connected) return false;
    
    std::string msg = data + "\n";
    ssize_t sent = ::send(sockfd, msg.c_str(), msg.length(), 0);
    return sent == static_cast<ssize_t>(msg.length());
}

std::string SocketClient::receive(int timeout) {
    if (!connected) return "";
    
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, timeout * 1000);
    if (ret <= 0) return "";
    
    char buffer[1024];
    ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    
    if (n <= 0) {
        connected = false;
        return "";
    }
    
    buffer[n] = '\0';
    std::string result(buffer);
    
    size_t newline = result.find('\n');
    if (newline != std::string::npos) {
        result = result.substr(0, newline);
    }
    
    return result;
}

void SocketClient::disconnect() {
    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }
    connected = false;
}