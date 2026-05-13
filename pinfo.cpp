//
// Created by nikhil-saxena on 8/28/24.
//

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>

#include "getInfo.h"
using namespace std;

/*
 *
 */

void parseProc(int pid) {
    char path[40];
    snprintf(path, 40, "/proc/%d/stat", pid);
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << path << endl;
        return;
    }

    string line;
    if (!getline(file, line)) {
        cerr << "Failed to read file: " << path << endl;
        return;
    }

    size_t open_paren = line.find('(');
    size_t close_paren = line.rfind(')');
    if (open_paren == string::npos || close_paren == string::npos || close_paren <= open_paren) {
        cerr << "Malformed stat format: " << path << endl;
        return;
    }

    string after = line.substr(close_paren + 2);
    istringstream iss(after);
    char state = '?';
    long v_size = 0;
    iss >> state;
    long dummy = 0;
    for (int i = 0; i < 20; i++) {
        iss >> dummy;
    }
    iss >> v_size;

    int pidpid = getpgid(pid);

    string t ;
    if(nshell_pid == pidpid) {
        t += "+";
    }

    cout<<"pid: "<<pid<<endl;
    cout<<"State: "<<state<<t<<endl;
    cout<<"Virtual Memory Size: "<<v_size<<endl;
}


// 0 fg, 1 bg


void getPInfor(int pid) {
    if(pid == 0)
        pid = getpid();
    parseProc(pid);
    char path[1024];
    string exe = "/exe";
    string proc = "/proc/";
    string pidPath = proc+to_string(pid)+exe;

    ssize_t len = readlink(pidPath.c_str(), path, sizeof(path) - 1);
    if (len == -1) {
        perror("readlink");
        return;
    }
    path[len] = '\0';
    cout<<"Executable Path: "<<path<<"\n";
}
