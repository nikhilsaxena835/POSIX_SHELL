#ifndef SHELLCONTEXT_H
#define SHELLCONTEXT_H

#include <dirent.h>
#include <string>
#include <vector>
#include "JobManager.h"

// ShellContext acts as the single façade for all shell subsystems:
// environment info, working directory, history, and job control.

struct ShellContext {
    // --- Environment ---
    std::string systemName;
    std::string username;
    std::string homeDir;

    // --- Working directory ---
    std::string currDir;
    std::string prevDir;
    DIR* currDirHandle  = nullptr;
    DIR* prevDirHandle  = nullptr;

    // --- Subsystems ---
    JobManager jobManager;
    std::vector<std::string> historyStore;

    // --- Facade helpers ---

    // Build the display prompt path (replaces homeDir prefix with ~)
    std::string promptPath() const {
        if (currDir == homeDir) {
            return "~";
        }
        if (currDir.find(homeDir) == 0 && currDir.length() != homeDir.length()) {
            return "~" + currDir.substr(homeDir.length());
        }
        return currDir;
    }

    // Full prompt string: user@host:path>
    std::string prompt() const {
        return username + "@" + systemName + ":" + promptPath() + "> ";
    }
};

#endif // SHELLCONTEXT_H
