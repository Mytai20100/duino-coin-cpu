#include "../include/http_client.h"
#include "../include/config.h"
#include "../include/logger.h"
#include <curl/curl.h>
#include <sstream>

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string HttpClient::get(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string response;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (!curl) {
        Logger::error("Failed to initialize CURL");
        return "";
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "duino-cpu/" VERSION);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        Logger::error("CURL error: " + std::string(curl_easy_strerror(res)));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return "";
    }
    
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    
    return response;
}

bool HttpClient::parse_url(const std::string& url, std::string& host, 
                           std::string& path, int& port, bool& use_ssl) {
    return true;
}

std::string HttpClient::https_get(const std::string& host, const std::string& path, int port) {
    return "";
}