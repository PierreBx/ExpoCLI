#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include <string>
#include <optional>

namespace expocli {

// Stores application context/settings during a session
class AppContext {
public:
    AppContext() = default;

    // XSD file path management
    void setXsdPath(const std::string& path);
    std::optional<std::string> getXsdPath() const;
    bool hasXsdPath() const;

    // Destination directory management
    void setDestPath(const std::string& path);
    std::optional<std::string> getDestPath() const;
    bool hasDestPath() const;

    // Verbose mode management
    void setVerbose(bool verbose);
    bool isVerbose() const;

private:
    std::optional<std::string> xsd_path_;
    std::optional<std::string> dest_path_;
    bool verbose_ = false;
};

} // namespace expocli

#endif // APP_CONTEXT_H
