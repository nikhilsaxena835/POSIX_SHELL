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
#include "pinfo.h"

using namespace std;

string zero = " \t\r\n\a";
char *delim = (char *)zero.c_str();
constexpr int buffer_size = 1024;
int foregroundPID = -1;
bool foreground = false;
vector<string> historyStore;


void CSigHandler(int signo){
    cout<<"\n"<<foregroundPID<<"\n";
    if(foregroundPID > 0 && foreground)
    kill(foregroundPID, SIGINT);
    else {
        cout<<"No foreground job to interrupt";
    }
}

void ZSigHandler(int signo){
    cout<<"\n"<<foregroundPID<<"\n";

    if(foregroundPID > 0 && foreground) {
        setpgid(foregroundPID, foregroundPID);
        cout<<"Foreground job suspended\n";
        kill(foregroundPID, SIGSTOP);
        foregroundPID = -1;
        foreground = false;
    }
    else {
        cout<<"No foreground job to suspend";
    }
}

void SIGCHLDHandler(int signo) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}


void send_signal(){
    // Set up signal handlers
    struct sigaction sa_int, sa_tstp, sa_chld;

    sa_int.sa_handler = CSigHandler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, nullptr);

    sa_tstp.sa_handler = ZSigHandler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    sigaction(SIGTSTP, &sa_tstp, nullptr);
/*
    sa_chld.sa_handler = SIGCHLDHandler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = 0;
    sigaction(SIGCHLD, &sa_chld, nullptr);
*/
}

void DHandler() {
    errno = 0;
    char *buffer = NULL;
    size_t bufsize = 0;
    ssize_t characters;
    characters = getline(&buffer, &bufsize, stdin);
    if (characters == -1 && errno == 0)
    {
        exit(1);
    }
}

void getInputCommand(char *buffer) {
    int buffer_index = 0;
    char c;
    do {
        c = (char) getchar();

        if (c == EOF || c == '\n') {
            if (c == EOF) {
                DHandler();
            }
            buffer[buffer_index] = '\0';
            return;
        } else {
            buffer[buffer_index] = c;
        }
        buffer_index++;
    } while (true);
}

vector<char *> tokenize(char *str, char *delim) {
    string temp = str;
    bool echo_flag = false;
    vector<char *> tokens;
    char *token = strtok(str, delim);
    while (token != NULL) {
        if (strcmp(token, "echo") == 0 && tokens.empty()) {
            echo_flag = true;
            tokens.push_back(token);
            break;
        }
        if (strcmp(token, "exit") == 0 && tokens.empty()) {
            exit(EXIT_SUCCESS);
        }
        tokens.push_back(token);
        token = strtok(NULL, delim);
    }
    if (echo_flag) {
        int x = temp.find_last_of(">");
        if(x!=temp.npos) {
            string t = temp;
            temp = temp.substr(5, x - 5); //get echo string
            if(isEchoQuote(temp) == false) {
                temp = removeWhiteSpace(temp);
            }
            char *temp2 = strdup(temp.c_str());
            tokens.push_back(temp2); //push echo string as single token

            t = t.substr(x, t.length()); // get output file string
            char *t_copy = strdup(t.c_str());

            char *token = strtok(t_copy, delim); // push everything else as seperate tokens
            while (token != nullptr) {
                tokens.push_back(strdup(token));
                token = strtok(nullptr, delim);
            }
        }
        else {

            temp = temp.substr(5);
            if(isEchoQuote(temp) == false) {
                temp = removeWhiteSpace(temp);
            }
            char *temp2 = strdup(temp.c_str());
            tokens.push_back(temp2);
        }
    }
    return tokens;
}

void seperatePipes(char *str, vector<char *> &piped_statements) {
    char *token = strtok(str, "|");
    while (token != NULL) {
        piped_statements.push_back(token);
        token = strtok(NULL, "|");
    }
}

bool hasPipes(char *input) {
    for (int i = 0; i < strlen(input); i++) {
        if (input[i] == '|') {
            return true;
        }
    }
    return false;
}


void checkSemicolons(char *str, vector<char *> &statements) {
    vector<char *> tokens;
    char *token = strtok(str, ";");
    while (token != NULL) {
        statements.push_back(token);
        token = strtok(NULL, ";");
    }
}

void handleRedirectionswithoutPipe(vector<char *> command, bool piped, bool background,
                        DIR *curr, DIR *prev, string &currD, string &prevD, const string &home_dir) {

    bool nullFlag = false;

    int shell_in = dup(0);
    int shell_out = dup(1);

    int pid = fork();
    foregroundPID = pid;
    if (pid < 0)
        perror("fork");

    else if (pid == 0) {
        if(background)
        setpgid(pid,getpid());

        int file_descriptor;
        for (int index = 0; index < command.size(); index++) {
            char *token = command[index];
            if ((strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) && command[index + 1] != nullptr) {
                if (strcmp(token, ">") == 0)
                    file_descriptor = open(command[index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                else {
                    file_descriptor = open(command[index + 1], O_WRONLY | O_APPEND, 0644);
                }
                if (file_descriptor == -1) {
                    perror(command[index + 1]);
                    exit(EXIT_FAILURE);
                }
                if (dup2(file_descriptor, STDOUT_FILENO) == -1) perror("dup2");
                if (close(file_descriptor) == -1) perror("close");
                command[index] = nullptr;
                nullFlag = true;
            } else if (strcmp(command[index], "<") == 0 && command[index + 1]) {
                file_descriptor = open(command[index + 1], O_RDONLY);
                if (file_descriptor == -1) {
                    perror(command[index + 1]);
                    exit(EXIT_FAILURE);
                }
                if (dup2(file_descriptor, STDIN_FILENO) == -1) perror("dup2");
                if (close(file_descriptor) == -1) perror("close");
                command[index] = nullptr;
                nullFlag = true;
            }
        }
        int count = 0;
        if (nullFlag) {
            while (command[count] != nullptr) count++;
        } else {
            count = command.size();
        }
        char *com[count + 1];
        int i = 0;
        while (i != count) {
            com[i] = command[i];
            i++;
        }
        com[i] = NULL;
        if (piped == 1 || isMyCommand(com[0]) == -1) {
            if (execvp(com[0], com) == -1) {
                perror("execvp");
            }
        } else {
            executeCommand(isMyCommand(com[0]), com[1], curr, prev, currD, prevD, com, home_dir, historyStore, count);
        }
        exit(EXIT_SUCCESS);

    } else {
        setpgid(foregroundPID, foregroundPID);
        foregroundPID = pid;
        foreground = true;
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



void handleRedirectionswithPipe(vector<char *> command, bool piped, bool background,
                         DIR *curr, DIR *prev, string &currD, string &prevD, const string &home_dir) {
    bool nullFlag = false;

    int shell_in = dup(0);
    int shell_out = dup(1);

    int file_descriptor;

    for (int index = 0; index < command.size(); index++) {
        char *token = command[index];
        if ((strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) && command[index + 1] != nullptr) {
            if (strcmp(token, ">") == 0)
                file_descriptor = open(command[index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            else {
                file_descriptor = open(command[index + 1], O_WRONLY | O_APPEND, 0644);
            }
            if (file_descriptor == -1) {
                perror(command[index + 1]);
                exit(EXIT_FAILURE);
            }
            if (dup2(file_descriptor, STDOUT_FILENO) == -1) perror("dup2");
            if (close(file_descriptor) == -1) perror("close");
            command[index] = nullptr;
            nullFlag = true;
        } else if (strcmp(command[index], "<") == 0 && command[index + 1]) {
            file_descriptor = open(command[index + 1], O_RDONLY);
            if (file_descriptor == -1) {
                perror(command[index + 1]);
                exit(EXIT_FAILURE);
            }
            if (dup2(file_descriptor, STDIN_FILENO) == -1) perror("dup2");
            if (close(file_descriptor) == -1) perror("close");
            command[index] = nullptr;
            nullFlag = true;
        }
    }

    int count = 0;
    if (nullFlag) {
        while (command[count] != nullptr) count++;
    } else {
        count = command.size();
    }
    char *com[count + 1];
    int i = 0;
    while (i != count) {
        com[i] = command[i];
        i++;
    }
    com[i] = nullptr;
    if (execvp(com[0], com) == -1) {
            perror("execvp");
    }
    dup2(shell_in, 0);
    dup2(shell_out, 1);
    close(shell_in);
    close(shell_out);

    //exit(EXIT_SUCCESS);
}

bool isBackground(const char *command) {
    for(int i = 0; i < strlen(command); i++) {
        if (command[i] == '&') {
            return true;
        }
    }
    return false;
}

void execute_statements(vector<char *> statements, DIR *curr, DIR *prev, string curr_directory, string prev_directory,
                        string home_dir) {
    for (int i = 0; i < statements.size(); i++) {
        add_history(historyStore, statements[i]);
        vector<char *> piped_clear_statements;
        seperatePipes(statements[i], piped_clear_statements);

        int in = 0;
        int fd[2];

        for (int j = 0; j < piped_clear_statements.size(); ++j) {
            if(piped_clear_statements[j][0] == 'c' && piped_clear_statements[j][1] == 'd') {
                vector<char *> tokenized = tokenize(piped_clear_statements[j], delim);
                changeDirectory(tokenized[1], curr, prev, curr_directory, prev_directory, home_dir);
                return  ;
            }

            if (j < piped_clear_statements.size() - 1) {
                if (pipe(fd) == -1) {
                    perror("pipe failed");
                    exit(EXIT_FAILURE);
                }
            }
            int pid  = fork();
            foregroundPID = pid;
            if (pid == -1) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }
            vector<char *> tokenized = tokenize(piped_clear_statements[j], delim);
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
                handleRedirectionswithPipe(tokenized, true, false, curr, prev, curr_directory, prev_directory, home_dir);
            } else {
                foreground = true;
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

    DIR *curr = opendir(".");
    DIR *prev = curr;
    DIR *home = curr;
    history_initiate(historyStore, home_dir);
    do {
        char *buffer = (char *) malloc(sizeof(char) * buffer_size);
        if (curr_directory == home_dir) {
            print_dir = '~';
        }
        cout<<username<<"@"<<system_name<<":"<<print_dir<<"> ";
        getInputCommand(buffer);
        vector<char *> statements;

        checkSemicolons(buffer, statements);
        bool isPiped = hasPipes(buffer);

        if (isPiped)
            execute_statements(statements, curr, prev, curr_directory, prev_directory, home_dir);

        else {
            for (int i = 0; i < statements.size(); i++) {
                add_history(historyStore, statements[i]);
                vector<char *> tokenized = tokenize(statements[i], delim);
                if(strcmp(tokenized[0], "pinfo") == 0) {
                    if(tokenized[1] == NULL) {
                        string zero = "0";
                        tokenized[1] = (char *)zero.c_str();
                    }
                    getPInfor(atoi(tokenized[1]));
                    continue;
                }

                if(strcmp(tokenized[0], "cd") == 0) {
                    int count = 0;

                    count = tokenized.size();
                    char *com[count + 1];
                    int i = 0;
                    while (i != count) {
                        com[i] = tokenized[i];
                        i++;
                    }
                    com[i] = NULL;
                    executeCommand(isMyCommand(com[0]), com[1], curr, prev, curr_directory, prev_directory, com, home_dir, historyStore, count);

                    continue;
                }
                bool background = isBackground(tokenized[tokenized.size() - 1]);
                handleRedirectionswithoutPipe(tokenized, false, background,
                                   curr, prev, curr_directory, prev_directory, home_dir);
            }
        }

        // For CD
        print_dir = curr_directory;
        if (print_dir.find(home_dir) == 0 && print_dir.length() != home_dir.length()) {
            string temp = print_dir.substr(home_dir.length(), print_dir.length());
            print_dir = "~" + temp;
        }
        free(buffer);
        printf("\n");
    } while (true);
    return 0;
}
