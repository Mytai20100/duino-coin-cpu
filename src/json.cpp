#include "../include/json.h"
#include <sstream>
#include <algorithm>

std::string Json::get_value(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    
    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return "";
    
    if (json[pos] == '"') {
        size_t start = pos + 1;
        size_t end = json.find('"', start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    } else {
        size_t start = pos;
        size_t end = json.find_first_of(",}\n", start);
        if (end == std::string::npos) end = json.length();
        std::string val = json.substr(start, end - start);
        val.erase(std::remove_if(val.begin(), val.end(), ::isspace), val.end());
        return val;
    }
}

bool Json::get_bool(const std::string& json, const std::string& key) {
    std::string val = get_value(json, key);
    return val == "true";
}

int Json::get_int(const std::string& json, const std::string& key) {
    std::string val = get_value(json, key);
    try {
        return std::stoi(val);
    } catch (...) {
        return 0;
    }
}

std::map<std::string, std::string> Json::parse(const std::string& json) {
    std::map<std::string, std::string> result;
    return result;
}