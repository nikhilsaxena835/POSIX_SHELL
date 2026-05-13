#ifndef EXECCONTEXT_H
#define EXECCONTEXT_H

#include <string>
#include <vector>

// Per-command execution context: just the command arguments.
// Directory/environment state is accessed via ShellContext.
struct ExecContext {
    std::vector<std::string> args;

    int argc() const { return static_cast<int>(args.size()); }
};

#endif // EXECCONTEXT_H
