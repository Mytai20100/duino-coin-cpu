#ifndef CONFIG_YAML_H
#define CONFIG_YAML_H

#include "config.h"
#include <string>

class ConfigYAML {
public:
    static bool load(const std::string& filename, Config& config);
    static bool save(const std::string& filename, const Config& config);
    static bool create_default(const std::string& filename);
};

#endif