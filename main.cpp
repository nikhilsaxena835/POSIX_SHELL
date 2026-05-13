#include <cstring>
#include <dirent.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <sys/stat.h>
#include <glob.h>
#include "getInfo.h"
#include <sys/wait.h>
#include "CommandRegistry.h"
#include "ParseChain.h"
#include "cd.h"
#include "echos.h"
#include "history.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <cctype>
#include "pinfo.h"
#include "ShellContext.h"

using namespace std;

string zero = " \t\r\n\a";
char *delim = (char *)zero.c_str();
ShellContext shellContext;
CommandRegistry commandRegistry;
static unique_ptr<IParseStage> parseChain;
volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigtstp_received = 0;
volatile sig_atomic_t sigchld_received = 0;


void CSigHandler(int signo){
    (void)signo;
    sigint_received = 1;
}

void ZSigHandler(int signo){
    (void)signo;
    sigtstp_received = 1;
}

void SIGCHLDHandler(int signo) {
    (void)signo;
    sigchld_received = 1;
}


void send_signal(){
    // Set up signal handlers
    struct sigaction sa_int, sa_tstp, sa_chld;

    sa_int.sa_handler = CSigHandler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, nullptr);

    sa_tstp.sa_handler = ZSigHandler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, nullptr);
    sa_chld.sa_handler = SIGCHLDHandler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa_chld, nullptr);
}

void handlePendingSignals() {
    if (sigint_received) {
        sigint_received = 0;
        shellContext.jobManager.interruptForeground();
    }
    if (sigtstp_received) {
        sigtstp_received = 0;
        shellContext.jobManager.suspendForeground();
    }
    if (sigchld_received) {
        sigchld_received = 0;
        shellContext.jobManager.reapChildren();
    }
}

string readInputLine() {
    string line;
    if (!getline(cin, line)) {
        exit(1);
    }
    return line;
}

bool isAllWhitespace(const string &input) {
    for (char c : input) {
        if (!isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

vector<string> splitByDelimiter(const string &input, char delimiter) {
    vector<string> parts;
    string current;
    bool in_single = false;
    bool in_double = false;
    for (char c : input) {
        if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        }
        if (!in_single && !in_double && c == delimiter) {
            if (!isAllWhitespace(current)) {
                parts.push_back(current);
            }
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!isAllWhitespace(current)) {
        parts.push_back(current);
    }
    return parts;
}

vector<string> splitSimple(const string &input) {
    vector<string> tokens;
    string current;
    for (char c : input) {
        if (isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

vector<string> splitWithQuotes(const string &input) {
    vector<string> tokens;
    string current;
    bool in_single = false;
    bool in_double = false;
    for (char c : input) {
        if (c == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (c == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (!in_single && !in_double && isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

vector<string> tokenizeLine(const string &input) {
    size_t start = input.find_first_not_of(" \t\r\n\a");
    if (start == string::npos) {
        return {};
    }
    bool is_echo = input.compare(start, 4, "echo") == 0 &&
                   (start + 4 == input.size() || isspace(static_cast<unsigned char>(input[start + 4])));
    if (!is_echo) {
        return splitWithQuotes(input);
    }

    size_t rest_start = start + 4;
    if (rest_start < input.size() && isspace(static_cast<unsigned char>(input[rest_start]))) {
        rest_start++;
    }
    string rest = input.substr(rest_start);
    string message = rest;
    string redir;
    bool in_single = false;
    bool in_double = false;
    for (size_t i = 0; i < rest.size(); ++i) {
        if (rest[i] == '\'' && !in_double) {
            in_single = !in_single;
        } else if (rest[i] == '"' && !in_single) {
            in_double = !in_double;
        }
        if (!in_single && !in_double && rest[i] == '>') {
            message = rest.substr(0, i);
            redir = rest.substr(i);
            break;
        }
    }

    if (!isEchoQuote(message)) {
        message = removeWhiteSpace(message);
    }

    vector<string> tokens;
    tokens.push_back("echo");
    tokens.push_back(message);

    if (!redir.empty()) {
        vector<string> redir_tokens = splitSimple(redir);
        tokens.insert(tokens.end(), redir_tokens.begin(), redir_tokens.end());
    }
    return tokens;
}

vector<char *> buildArgv(vector<string> &tokens) {
    vector<char *> argv;
    argv.reserve(tokens.size() + 1);
    for (string &token : tokens) {
        argv.push_back(const_cast<char *>(token.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

bool hasPipes(const string &input) {
    bool in_single = false;
    bool in_double = false;
    for (char c : input) {
        if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else if (!in_single && !in_double && c == '|') {
            return true;
        }
    }
    return false;
}

bool stripBackgroundToken(vector<string> &tokens) {
    if (tokens.empty()) {
        return false;
    }
    string &last = tokens.back();
    if (last == "&") {
        tokens.pop_back();
        return true;
    }
    if (!last.empty() && last.back() == '&') {
        last.pop_back();
        if (last.empty()) {
            tokens.pop_back();
        }
        return true;
    }
    return false;
}

// ================================================================
// AST-driven execution helpers
// ================================================================

// Apply pre-parsed redirections in a child process
static void applyRedirections(const vector<Redirection>& redirs) {
    for (const auto& r : redirs) {
        int fd = -1;
        switch (r.type) {
            case RedirType::Input:
                fd = open(r.filename.c_str(), O_RDONLY);
                if (fd == -1) { perror(r.filename.c_str()); _exit(EXIT_FAILURE); }
                if (dup2(fd, STDIN_FILENO) == -1) perror("dup2");
                break;
            case RedirType::Output:
                fd = open(r.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) { perror(r.filename.c_str()); _exit(EXIT_FAILURE); }
                if (dup2(fd, STDOUT_FILENO) == -1) perror("dup2");
                break;
            case RedirType::Append:
                fd = open(r.filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd == -1) { perror(r.filename.c_str()); _exit(EXIT_FAILURE); }
                if (dup2(fd, STDOUT_FILENO) == -1) perror("dup2");
                break;
        }
        if (fd != -1) close(fd);
    }
}

// Execute a CommandNode in a child process (apply redirections, then dispatch)
static void executeInChild(const CommandNode& cmdNode, ShellContext& context) {
    applyRedirections(cmdNode.redirections);

    if (cmdNode.args.empty()) _exit(EXIT_FAILURE);
    if (cmdNode.args[0] == "exit") _exit(EXIT_SUCCESS);

    ICommand* icmd = commandRegistry.lookup(cmdNode.args[0]);
    if (icmd) {
        vector<string> argsCopy = cmdNode.args;
        ExecContext exec{argsCopy};
        icmd->execute(context, exec);
        cout.flush(); cerr.flush(); fflush(nullptr);
        _exit(EXIT_SUCCESS);
    }
    vector<string> argsCopy = cmdNode.args;
    vector<char*> argv = buildArgv(argsCopy);
    if (execvp(argv[0], argv.data()) == -1) {
        perror("execvp");
        _exit(EXIT_FAILURE);
    }
}

// Execute a single-command pipeline (fork, redirect, run)
static void executeSingleCommand(const CommandNode& cmdNode, bool background,
                                 ShellContext& context) {
    int shell_in = dup(0);
    int shell_out = dup(1);

    int pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        setpgid(0, 0);
        executeInChild(cmdNode, context);
    } else {
        setpgid(pid, pid);
        if (!background) {
            context.jobManager.startForeground(pid);
            context.jobManager.waitForForeground();
        } else {
            context.jobManager.startBackground(pid);
        }
    }
    dup2(shell_in, 0);
    dup2(shell_out, 1);
    close(shell_in);
    close(shell_out);
}

// Execute a multi-command pipeline (fork per stage, pipe between them)
static void executePipeline(const PipelineNode& pipeline, ShellContext& context) {
    int in = 0;
    int fd[2];
    pid_t pgid = -1;
    size_t n = pipeline.commands.size();

    for (size_t j = 0; j < n; ++j) {
        const CommandNode& cmdNode = pipeline.commands[j];
        if (cmdNode.args.empty()) continue;

        if (j + 1 < n) {
            if (pipe(fd) == -1) { perror("pipe failed"); exit(EXIT_FAILURE); }
        }

        int pid = fork();
        if (pid == -1) { perror("fork failed"); exit(EXIT_FAILURE); }

        if (pid == 0) {
            if (pgid == -1) pgid = getpid();
            setpgid(0, pgid);
            if (in != 0) { dup2(in, 0); close(in); }
            if (j + 1 < n) { dup2(fd[1], 1); }
            close(fd[0]);
            executeInChild(cmdNode, context);
        } else {
            if (pgid == -1) pgid = pid;
            setpgid(pid, pgid);
            if (!pipeline.background) {
                context.jobManager.startForeground(pgid);
            } else {
                context.jobManager.startBackground(pid);
            }
            close(fd[1]);
            in = fd[0];
            if (!pipeline.background) {
                context.jobManager.waitForPid(pid);
            }
        }
    }
}

// ================================================================
// Main REPL
// ================================================================

int main() {
    string system_name, home_dir, username;
    get_name(system_name, home_dir, username);

    // Initialize the shell façade
    shellContext.systemName = system_name;
    shellContext.username   = username;
    shellContext.homeDir    = home_dir;
    shellContext.currDir    = home_dir;
    shellContext.prevDir    = home_dir;
    shellContext.currDirHandle = opendir(".");
    shellContext.prevDirHandle = shellContext.currDirHandle;

    send_signal();
    commandRegistry.registerDefaults();
    parseChain = buildDefaultParseChain();
    history_initiate(shellContext.historyStore, shellContext.homeDir);

    do {
        string input = readInputLine();
        handlePendingSignals();
        cout << shellContext.prompt();

        // Parse input through the chain
        ParseContext parseCtx{input, {}, true, {}};
        parseChain->handle(parseCtx);
        if (!parseCtx.valid) {
            cerr << parseCtx.error << endl;
            printf("\n");
            continue;
        }

        // Execute each pipeline in the sequence
        for (auto& pipeline : parseCtx.sequence.pipelines) {
            // Record history
            add_history(shellContext.historyStore,
                        const_cast<char*>(pipeline.rawText.c_str()));

            // Filter out empty pipelines
            if (pipeline.commands.empty() || pipeline.commands[0].args.empty()) {
                continue;
            }

            // Single command — check for parent-only builtins
            if (pipeline.commands.size() == 1) {
                CommandNode& cmd = pipeline.commands[0];

                if (cmd.args[0] == "exit") {
                    exit(EXIT_SUCCESS);
                }

                ICommand* icmd = commandRegistry.lookup(cmd.args[0]);
                if (icmd && (cmd.args[0] == "cd" || cmd.args[0] == "pinfo")) {
                    if (cmd.args[0] == "pinfo" && cmd.args.size() < 2) {
                        cmd.args.push_back("0");
                    }
                    ExecContext exec{cmd.args};
                    icmd->execute(shellContext, exec);
                    continue;
                }

                executeSingleCommand(cmd, pipeline.background, shellContext);
            } else {
                executePipeline(pipeline, shellContext);
            }
        }

        printf("\n");
    } while (true);
    return 0;
}
