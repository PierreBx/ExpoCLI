#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "app_context.h"
#include <string>

namespace expocli {

class CommandHandler {
public:
    explicit CommandHandler(AppContext& context);

    // Returns true if the command was handled, false if it should be treated as a query
    bool handleCommand(const std::string& input);

private:
    AppContext& context_;

    bool handleSetCommand(const std::string& input);
    bool handleShowCommand(const std::string& input);
    bool handleGenerateCommand(const std::string& input);
    bool handleCheckCommand(const std::string& input);

    void setXsdPath(const std::string& path);
    void setDestPath(const std::string& path);

    void showXsdPath();
    void showDestPath();

    bool validateXsdFile(const std::string& path);
    bool validateAndCreateDestDirectory(const std::string& path);
};

} // namespace expocli

#endif // COMMAND_HANDLER_H
