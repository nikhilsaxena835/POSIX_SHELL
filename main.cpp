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
#include "commandCentre.h"
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
volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigtstp_received = 0;
volatile sig_atomic_t sigchld_received = 0;


void CSigHandler(int signo){
    sigint_received = 1;
}

void ZSigHandler(int signo){
    sigtstp_received = 1;
}

void SIGCHLDHandler(int signo) {
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
        if (shellContext.foregroundPID > 0 && shellContext.foreground) {
            kill(shellContext.foregroundPID, SIGINT);
        } else {
            cout << "No foreground job to interrupt";
        }
    }
    if (sigtstp_received) {
        sigtstp_received = 0;
        if (shellContext.foregroundPID > 0 && shellContext.foreground) {
            setpgid(shellContext.foregroundPID, shellContext.foregroundPID);
            cout << "Foreground job suspended\n";
            kill(shellContext.foregroundPID, SIGSTOP);
            shellContext.foregroundPID = -1;
            shellContext.foreground = false;
        } else {
            cout << "No foreground job to suspend";
        }
    }
    if (sigchld_received) {
        sigchld_received = 0;
        int status = 0;
        while (waitpid(-1, &status, WNOHANG) > 0) {
        }
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
    bool in_quotes = false;
    for (char c : input) {
        if (c == '"') {
            in_quotes = !in_quotes;
        }
        if (!in_quotes && c == delimiter) {
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
    bool in_quotes = false;
    for (char c : input) {
        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (!in_quotes && isspace(static_cast<unsigned char>(c))) {
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
    bool in_quotes = false;
    for (size_t i = 0; i < rest.size(); ++i) {
        if (rest[i] == '"') {
            in_quotes = !in_quotes;
        }
        if (!in_quotes && rest[i] == '>') {
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
    return input.find('|') != string::npos;
}

void handleRedirectionswithoutPipe(const vector<string> &command, bool piped, bool background,
                        DIR *curr, DIR *prev, string &currD, string &prevD, const string &home_dir,
                        ShellContext &context) {
    int shell_in = dup(0);
    int shell_out = dup(1);

    int pid = fork();
    context.foregroundPID = pid;
    if (pid < 0)
        perror("fork");

    else if (pid == 0) {
        if(background)
        setpgid(pid,getpid());

        int file_descriptor;
        vector<string> cleaned;
        for (size_t index = 0; index < command.size(); ++index) {
            const string &token = command[index];
            if (token == ">" || token == ">>" || token == "<") {
                if (index + 1 >= command.size()) {
                    cerr << "Redirection error: missing file" << endl;
                    _exit(EXIT_FAILURE);
                }
                const string &filename = command[index + 1];
                if (token == ">") {
                    file_descriptor = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                } else if (token == ">>") {
                    file_descriptor = open(filename.c_str(), O_WRONLY | O_APPEND, 0644);
                } else {
                    file_descriptor = open(filename.c_str(), O_RDONLY);
                }
                if (file_descriptor == -1) {
                    perror(filename.c_str());
                    _exit(EXIT_FAILURE);
                }
                if (token == "<") {
                    if (dup2(file_descriptor, STDIN_FILENO) == -1) perror("dup2");
                } else {
                    if (dup2(file_descriptor, STDOUT_FILENO) == -1) perror("dup2");
                }
                if (close(file_descriptor) == -1) perror("close");
                ++index;
            } else {
                cleaned.push_back(token);
            }
        }

        if (cleaned.empty()) {
            _exit(EXIT_FAILURE);
        }

        vector<char *> argv = buildArgv(cleaned);
        char **com = argv.data();
        int count = cleaned.size();
        if (piped == 1 || isMyCommand(com[0]) == -1) {
            if (execvp(com[0], com) == -1) {
                perror("execvp");
                _exit(EXIT_FAILURE);
            }
        } else {
            executeCommand(isMyCommand(com[0]), com[1], curr, prev, currD, prevD, com, home_dir, context, count);
        }
        _exit(EXIT_SUCCESS);

    } else {
        setpgid(context.foregroundPID, context.foregroundPID);
        context.foregroundPID = pid;
        context.foreground = true;
        if (!background)
            waitpid(pid, NULL, WUNTRACED | WCONTINUED);
        else {
            cout<<"PID : "<<pid<<endl;
        }
    }
    dup2(shell_in, 0);
    dup2(shell_out, 1);
    close(shell_in);
    close(shell_out);
}



void handleRedirectionswithPipe(const vector<string> &command, bool piped, bool background,
                          DIR *curr, DIR *prev, string &currD, string &prevD, const string &home_dir,
                          ShellContext &context) {
    int shell_in = dup(0);
    int shell_out = dup(1);

    int file_descriptor;
    vector<string> cleaned;
    for (size_t index = 0; index < command.size(); ++index) {
        const string &token = command[index];
        if (token == ">" || token == ">>" || token == "<") {
            if (index + 1 >= command.size()) {
                cerr << "Redirection error: missing file" << endl;
                _exit(EXIT_FAILURE);
            }
            const string &filename = command[index + 1];
            if (token == ">") {
                file_descriptor = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            } else if (token == ">>") {
                file_descriptor = open(filename.c_str(), O_WRONLY | O_APPEND, 0644);
            } else {
                file_descriptor = open(filename.c_str(), O_RDONLY);
            }
            if (file_descriptor == -1) {
                perror(filename.c_str());
                _exit(EXIT_FAILURE);
            }
            if (token == "<") {
                if (dup2(file_descriptor, STDIN_FILENO) == -1) perror("dup2");
            } else {
                if (dup2(file_descriptor, STDOUT_FILENO) == -1) perror("dup2");
            }
            if (close(file_descriptor) == -1) perror("close");
            ++index;
        } else {
            cleaned.push_back(token);
        }
    }

    if (cleaned.empty()) {
        _exit(EXIT_FAILURE);
    }

    vector<char *> argv = buildArgv(cleaned);
    char **com = argv.data();
    int count = cleaned.size();
    if (isMyCommand(com[0]) != -1) {
        executeCommand(isMyCommand(com[0]), com[1], curr, prev, currD, prevD, com, home_dir, context, count);
        _exit(EXIT_SUCCESS);
    }
    if (execvp(com[0], com) == -1) {
        perror("execvp");
        _exit(EXIT_FAILURE);
    }
}

bool isBackground(const string &command) {
    return command.find('&') != string::npos;
}

void execute_statements(const vector<string> &statements, DIR *curr, DIR *prev, string curr_directory, string prev_directory,
                        string home_dir, ShellContext &context) {
    for (int i = 0; i < statements.size(); i++) {
        add_history(context.historyStore, const_cast<char *>(statements[i].c_str()));
        vector<string> piped_clear_statements = splitByDelimiter(statements[i], '|');

        int in = 0;
        int fd[2];

        for (int j = 0; j < piped_clear_statements.size(); ++j) {
            vector<string> tokenized = tokenizeLine(piped_clear_statements[j]);
            if (tokenized.empty()) {
                continue;
            }

            if (j < piped_clear_statements.size() - 1) {
                if (pipe(fd) == -1) {
                    perror("pipe failed");
                    exit(EXIT_FAILURE);
                }
            }
            int pid  = fork();
            context.foregroundPID = pid;
            if (pid == -1) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
            bool background = isBackground(tokenized[tokenized.size() - 1]);
            if (pid == 0) {
                if (background) {setpgid(getpid(),getpid());}

                if (in != 0) {
                    dup2(in, 0);
                    close(in);
                }
                if (j < piped_clear_statements.size() - 1) {
                    dup2(fd[1], 1);
                }
                close(fd[0]);
                handleRedirectionswithPipe(tokenized, true, false, curr, prev, curr_directory, prev_directory, home_dir, context);
            } else {
                context.foreground = true;
                close(fd[1]);
                in = fd[0];
                if(!background) {
                    waitpid(pid, NULL, WUNTRACED);
                }
                else {
                    cout<<"PID :"<<pid;
                }
            }
        }
    }
}

int main() {

    string system_name, home_dir, username;
    get_name(system_name, home_dir, username);

    send_signal();

    string print_dir;
    string curr_directory = home_dir;
    string prev_directory = home_dir;
    shellContext.homeDir = home_dir;

    DIR *curr = opendir(".");
    DIR *prev = curr;
    DIR *home = curr;
    history_initiate(shellContext.historyStore, home_dir);
    do {
        string input = readInputLine();
        handlePendingSignals();
        if (curr_directory == home_dir) {
            print_dir = '~';
        }
        cout<<username<<"@"<<system_name<<":"<<print_dir<<"> ";
        // input already captured
        vector<string> statements = splitByDelimiter(input, ';');
        for (int i = 0; i < statements.size(); i++) {
            if (hasPipes(statements[i])) {
                vector<string> single_statement;
                single_statement.push_back(statements[i]);
                execute_statements(single_statement, curr, prev, curr_directory, prev_directory, home_dir, shellContext);
                continue;
            }

            add_history(shellContext.historyStore, const_cast<char *>(statements[i].c_str()));
            vector<string> tokenized = tokenizeLine(statements[i]);
            if (tokenized.empty()) {
                continue;
            }
            if(tokenized[0] == "pinfo") {
                if(tokenized.size() < 2) {
                    tokenized.push_back("0");
                }
                getPInfor(atoi(tokenized[1].c_str()));
                continue;
            }

            if(tokenized[0] == "cd") {
                vector<char *> com = buildArgv(tokenized);
                int count = tokenized.size();
                executeCommand(isMyCommand(com[0]), com[1], curr, prev, curr_directory, prev_directory, com.data(), home_dir, shellContext, count);
                continue;
            }
            bool background = isBackground(tokenized[tokenized.size() - 1]);
            handleRedirectionswithoutPipe(tokenized, false, background,
                               curr, prev, curr_directory, prev_directory, home_dir, shellContext);
        }

        // For CD
        print_dir = curr_directory;
        if (print_dir.find(home_dir) == 0 && print_dir.length() != home_dir.length()) {
            string temp = print_dir.substr(home_dir.length(), print_dir.length());
            print_dir = "~" + temp;
        }
        printf("\n");
    } while (true);
    return 0;
}
