#ifndef XJADEO_CONFIGURATIONMANAGER_H
#define XJADEO_CONFIGURATIONMANAGER_H

#include <string>
#include <map>
#include <vector>

namespace xjadeo {

/**
 * ConfigurationManager - Handles configuration file reading/writing and command-line parsing
 * 
 * Manages application configuration, command-line options, and config file persistence.
 */
class ConfigurationManager {
public:
    ConfigurationManager();
    ~ConfigurationManager();

    // Load configuration from file
    bool loadFromFile(const std::string& filename);
    
    // Save configuration to file
    bool saveToFile(const std::string& filename) const;

    // Parse command-line arguments
    int parseCommandLine(int argc, char** argv);

    // Get configuration values
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;

    // Set configuration values
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);

    // Get command-line arguments (for compatibility)
    std::vector<std::string> getArguments() const { return arguments_; }
    std::string getMovieFile() const { return movieFile_; }
    std::string getConfigFilePath() const;

private:
    std::map<std::string, std::string> config_;
    std::vector<std::string> arguments_;
    std::string movieFile_;
    
    void loadDefaults();
};

} // namespace xjadeo

#endif // XJADEO_CONFIGURATIONMANAGER_H

