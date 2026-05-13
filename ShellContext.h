#ifndef SHELLCONTEXT_H
#define SHELLCONTEXT_H

#include <string>
#include <vector>

struct ShellContext {
    int foregroundPID = -1;
    bool foreground = false;
    std::vector<std::string> historyStore;
    std::string homeDir;
};

#endif // SHELLCONTEXT_H
