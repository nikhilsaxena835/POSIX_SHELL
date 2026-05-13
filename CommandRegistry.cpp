#include "CommandRegistry.h"
#include "cd.h"
#include "echos.h"
#include "ls.h"
#include "pinfo.h"
#include "search.h"
#include "history.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

using namespace std;

// --- Helper: build a null-terminated char* argv from ExecContext ---
static vector<char*> buildArgvFromExec(ExecContext& exec) {
    vector<char*> argv;
    argv.reserve(exec.args.size() + 1);
    for (auto& s : exec.args) {
        argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

// ================================================================
// Concrete command classes
// ================================================================

class CdCommand : public ICommand {
public:
    int execute(ShellContext& ctx, ExecContext& exec) override {
        int count = exec.argc();
        if (count > 2) {
            cout << "Error: CD got more than one arguments" << endl;
            return 1;
        }
        int x;
        if (count == 2) {
            x = changeDirectory(exec.args[1], ctx.currDirHandle, ctx.prevDirHandle,
                                ctx.currDir, ctx.prevDir, ctx.homeDir);
        } else {
            x = 0;
        }
        if (x == 0) {
            string temp = ctx.currDir;
            ctx.currDir = ctx.homeDir;
            ctx.prevDir = temp;
            chdir(ctx.homeDir.c_str());
        }
        if (x == 1 || x == -1) {
            return (x == -1) ? 1 : 0;
        }
        return 0;
    }
};

class EchoCommand : public ICommand {
public:
    int execute(ShellContext& /*ctx*/, ExecContext& exec) override {
        if (exec.argc() >= 2) {
            string temp = exec.args[1];
            echo(temp);
            cout << "\n";
        }
        return 0;
    }
};

class PwdCommand : public ICommand {
public:
    int execute(ShellContext& /*ctx*/, ExecContext& /*exec*/) override {
        pwd();
        return 0;
    }
};

class LsCommand : public ICommand {
public:
    int execute(ShellContext& ctx, ExecContext& exec) override {
        auto argv = buildArgvFromExec(exec);
        lsMain(argv.data(), ctx.homeDir);
        return 0;
    }
};

class PinfoCommand : public ICommand {
public:
    int execute(ShellContext& /*ctx*/, ExecContext& exec) override {
        int pid = 0;
        if (exec.argc() >= 2) {
            pid = atoi(exec.args[1].c_str());
        }
        getPInfor(pid);
        return 0;
    }
};

class SearchCommand : public ICommand {
public:
    int execute(ShellContext& /*ctx*/, ExecContext& exec) override {
        if (exec.argc() >= 2) {
            recursive_search(const_cast<char*>(exec.args[1].c_str()));
        } else {
            recursive_search(nullptr);
        }
        return 0;
    }
};

class HistoryCommand : public ICommand {
public:
    int execute(ShellContext& ctx, ExecContext& exec) override {
        int n = 0;
        if (exec.argc() >= 2) {
            n = atoi(exec.args[1].c_str());
        }
        print_history(ctx.historyStore, n);
        return 0;
    }
};

// ================================================================
// Registry implementation
// ================================================================

void CommandRegistry::registerCommand(const string& name, unique_ptr<ICommand> cmd) {
    commands_[name] = move(cmd);
}

ICommand* CommandRegistry::lookup(const string& name) const {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void CommandRegistry::registerDefaults() {
    registerCommand("cd",      make_unique<CdCommand>());
    registerCommand("echo",    make_unique<EchoCommand>());
    registerCommand("pwd",     make_unique<PwdCommand>());
    registerCommand("ls",      make_unique<LsCommand>());
    registerCommand("pinfo",   make_unique<PinfoCommand>());
    registerCommand("search",  make_unique<SearchCommand>());
    registerCommand("history", make_unique<HistoryCommand>());
}
