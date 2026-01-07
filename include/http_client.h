#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <string>

class HttpClient {
public:
    static std::string get(const std::string& url);
    
private:
    static bool parse_url(const std::string& url, std::string& host, 
                         std::string& path, int& port, bool& use_ssl);
    static std::string https_get(const std::string& host, const std::string& path, int port);
};

#endif