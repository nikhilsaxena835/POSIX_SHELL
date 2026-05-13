#ifndef EXECCONTEXT_H
#define EXECCONTEXT_H

#include <dirent.h>
#include <string>
#include <vector>

struct ExecContext {
    std::vector<std::string> args;
    DIR** curr;
    DIR** prev;
    std::string* currDir;
    std::string* prevDir;

    int argc() const { return static_cast<int>(args.size()); }
};

#endif // EXECCONTEXT_H
