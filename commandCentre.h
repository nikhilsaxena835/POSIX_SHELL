//
// Created by nikhil-saxena on 8/27/24.
//

#ifndef COMMANDCENTRE_H
#define COMMANDCENTRE_H
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include "getInfo.h"
#include <sys/wait.h>
#include <dirent.h>
using namespace std;
int isMyCommand(const char * temp);

void executeCommand(int index,
    const char *newdir, DIR *curr, DIR *prev, string &currD, string &prevD,
    char ** tokenString, const string & home_dir,
    vector<string> historyStore, int count);

#endif //COMMANDCENTRE_H
