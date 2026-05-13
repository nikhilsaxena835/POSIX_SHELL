#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include <memory>
#include <string>
#include <unordered_map>
#include "ICommand.h"

class CommandRegistry {
public:
    void registerCommand(const std::string& name, std::unique_ptr<ICommand> cmd);
    ICommand* lookup(const std::string& name) const;
    void registerDefaults();
private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands_;
};

#endif // COMMANDREGISTRY_H
