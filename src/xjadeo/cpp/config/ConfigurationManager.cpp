#include "ConfigurationManager.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>

namespace xjadeo {

ConfigurationManager::ConfigurationManager() {
    loadDefaults();
}

ConfigurationManager::~ConfigurationManager() {
}

void ConfigurationManager::loadDefaults() {
    // Set default values
    setInt("osc_port", 0);
    setBool("remote_en", false);
    setBool("mq_en", false);
    setBool("want_quiet", false);
    setBool("want_verbose", false);
    setDouble("fps", 0.0); // 0 = use file framerate
    setInt("offset", 0);
    setString("midi_port", "-1"); // -1 = autodetect
    setBool("midi_clkadj", false); // MIDI clock adjustment
    setDouble("delay", -1.0); // Frame delay (1.0/fps, or -1 to use file framerate)
    setBool("want_letterbox", true);
    setBool("start_fullscreen", false);
    setBool("start_ontop", false);
}

bool ConfigurationManager::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            config_[key] = value;
        }
    }

    return true;
}

bool ConfigurationManager::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << "# xjadeo configuration file\n";
    file << "# Generated automatically\n\n";

    for (const auto& pair : config_) {
        file << pair.first << "=" << pair.second << "\n";
    }

    return true;
}

int ConfigurationManager::parseCommandLine(int argc, char** argv) {
    arguments_.clear();
    movieFile_.clear();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        arguments_.push_back(arg);

        if (arg == "--osc" || arg == "-O") {
            if (i + 1 < argc) {
                setInt("osc_port", std::atoi(argv[++i]));
            } else {
                setInt("osc_port", 7000); // Default port
            }
        } else if (arg == "--remote" || arg == "-R") {
            setBool("remote_en", true);
        } else if (arg == "--mq" || arg == "-Q") {
            setBool("mq_en", true);
        } else if (arg == "--quiet" || arg == "-q") {
            setBool("want_quiet", true);
        } else if (arg == "--verbose" || arg == "-v") {
            setBool("want_verbose", true);
        } else if (arg == "--fps" || arg == "-f") {
            if (i + 1 < argc) {
                setDouble("fps", std::atof(argv[++i]));
            }
        } else if (arg == "--offset" || arg == "-o") {
            if (i + 1 < argc) {
                setInt("offset", std::atoi(argv[++i]));
            }
        } else if (arg == "--midi" || arg == "-m") {
            if (i + 1 < argc) {
                setString("midi_port", argv[++i]);
            }
        } else if (arg == "--midi-clkadj" || arg == "--midi-clk") {
            setBool("midi_clkadj", true);
        } else if (arg == "--fullscreen" || arg == "-s") {
            setBool("start_fullscreen", true);
        } else if (arg == "--ontop" || arg == "-a") {
            setBool("start_ontop", true);
        } else if (arg[0] != '-') {
            // Assume it's a movie file
            if (movieFile_.empty()) {
                movieFile_ = arg;
            }
        }
    }

    return 0;
}

std::string ConfigurationManager::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return defaultValue;
}

int ConfigurationManager::getInt(const std::string& key, int defaultValue) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return std::atoi(it->second.c_str());
    }
    return defaultValue;
}

double ConfigurationManager::getDouble(const std::string& key, double defaultValue) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return std::atof(it->second.c_str());
    }
    return defaultValue;
}

bool ConfigurationManager::getBool(const std::string& key, bool defaultValue) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        const std::string& value = it->second;
        return (value == "1" || value == "true" || value == "yes" || value == "on");
    }
    return defaultValue;
}

void ConfigurationManager::setString(const std::string& key, const std::string& value) {
    config_[key] = value;
}

void ConfigurationManager::setInt(const std::string& key, int value) {
    config_[key] = std::to_string(value);
}

void ConfigurationManager::setDouble(const std::string& key, double value) {
    config_[key] = std::to_string(value);
}

void ConfigurationManager::setBool(const std::string& key, bool value) {
    config_[key] = value ? "1" : "0";
}

std::string ConfigurationManager::getConfigFilePath() const {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.xjadeorc";
    }
    return ".xjadeorc";
}

} // namespace xjadeo

