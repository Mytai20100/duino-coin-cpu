#ifndef JSON_H
#define JSON_H

#include <string>
#include <map>

class Json {
public:
    static std::map<std::string, std::string> parse(const std::string& json);
    static std::string get_value(const std::string& json, const std::string& key);
    static bool get_bool(const std::string& json, const std::string& key);
    static int get_int(const std::string& json, const std::string& key);
};

#endif