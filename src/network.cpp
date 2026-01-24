#include "../include/network.h"
#include "../include/logger.h"
#include "../include/http_client.h"
#include "../include/json.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

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
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;
    
    std::string port_str = std::to_string(port);
    
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &servinfo) != 0) {
        return false;
    }
    
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype | SOCK_CLOEXEC, p->ai_protocol);
        if (sockfd == -1) continue;
        
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        
        int flag = 1;
        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        
        flag = 1;
        setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));
        
        int keepalive = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
        
        int keepidle = 60;
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
        
        int keepintvl = 10;
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
        
        int keepcnt = 3;
        setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
        
        int bufsize = 262144;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
        
        int tos = 0x10;
        setsockopt(sockfd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
        
        int user_timeout = timeout * 1000;
        setsockopt(sockfd, IPPROTO_TCP, TCP_USER_TIMEOUT, &user_timeout, sizeof(user_timeout));
        
        if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            if (errno == EINPROGRESS) {
                struct pollfd pfd;
                pfd.fd = sockfd;
                pfd.events = POLLOUT;
                
                int ret = poll(&pfd, 1, timeout * 1000);
                if (ret > 0) {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
                    
                    if (error == 0) {
                        fcntl(sockfd, F_SETFL, flags);
                        connected = true;
                        break;
                    }
                }
            }
            
            close(sockfd);
            sockfd = -1;
            continue;
        }
        
        fcntl(sockfd, F_SETFL, flags);
        connected = true;
        break;
    }
    
    freeaddrinfo(servinfo);
    return connected;
}

bool SocketClient::send(const std::string& data) {
    if (!connected) return false;
    
    std::string msg = data + "\n";
    size_t total_sent = 0;
    size_t remaining = msg.length();
    
    while (total_sent < msg.length()) {
        ssize_t sent = ::send(sockfd, msg.c_str() + total_sent, remaining, MSG_NOSIGNAL);
        
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = sockfd;
                pfd.events = POLLOUT;
                
                if (poll(&pfd, 1, 5000) <= 0) {
                    return false;
                }
                continue;
            }
            return false;
        }
        
        total_sent += sent;
        remaining -= sent;
    }
    
    return true;
}

std::string SocketClient::receive(int timeout) {
    if (!connected) return "";
    
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, timeout * 1000);
    if (ret <= 0) return "";
    
    char buffer[4096];
    ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    
    if (n <= 0) {
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return "";
        }
        connected = false;
        return "";
    }
    
    buffer[n] = '\0';
    std::string result(buffer);
    
    size_t newline = result.find('\n');
    if (newline != std::string::npos) {
        result = result.substr(0, newline);
    }
    
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));
    
    return result;
}

void SocketClient::disconnect() {
    if (sockfd != -1) {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        sockfd = -1;
    }
    connected = false;
}