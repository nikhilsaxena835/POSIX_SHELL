//
// Created by nikhil-saxena on 8/28/24.
//

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
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
    string proc = "/proc/";
    string stat = "stat";

    //string ppath = proc+to_string(pid)+stat;
    snprintf(path, 40, "/proc/%ld/stat", pid);
    const string filePath = path, line;
    FILE *file = fopen(path, "r");
    if (!file) {
        cerr << "Failed to open file: " << filePath << endl;
        return;
    }

    char state;
    char useless[100] = {0};
    int v_size = 0;

    fscanf(file,"%d %s %c", &pid, useless, &state);
    int dummy;
    for(int i = 4 ; i < 23; i++) {
        fscanf(file,"%d", &dummy);
    }
    fscanf(file,"%d", &v_size);

    int console_pid = tcgetpgrp(STDIN_FILENO);
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

    readlink(pidPath.c_str(), path, 1024); // execcutaable pathm
    string line = path;
    cout<<"Executable Path: "<<line<<"\n";
}
