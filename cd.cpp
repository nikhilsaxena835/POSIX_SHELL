//
// Created by nikhil-saxena on 8/27/24.
//

#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <sys/wait.h>
#include "commandCentre.h"
#include <dirent.h>
#include "cd.h"
#include <pwd.h>

using namespace std;

/*
 * ~ HOME 0
 * . CURR 1
 */
int changeDirectory(const string& cmd, DIR *curr, DIR *prev, string &currD, string &prevD, const string& home_dir) {
    if(cmd == "~") {
        if (chdir(home_dir.c_str()) != 0) {
            perror("cd");
            return -1;
        }
        prevD = currD;
        currD = home_dir;
        return 1;
    }

    if(cmd == ".") {
        return 1;
    }

    if(cmd == "..") {
        int last = currD.find_last_of("/");
        string parent = (last == 0) ? string("/") : currD.substr(0, last);
        if (chdir(parent.c_str()) != 0) {
            perror("cd");
            return -1;
        }
        prevD = currD;
        currD = parent;
        return 1;
    }

    if(cmd == "-") {
        string temp = currD;
        if (chdir(prevD.c_str()) != 0) {
            perror("cd");
            return -1;
        }
        currD = prevD;
        prevD = temp;
        char buf[1024];
        getcwd(buf, sizeof(buf));
        cout<<buf<<endl;
        return 1;
    }

    string target = cmd;
    if (!target.empty() && target[0] == '~') {
        if (target.size() == 1) {
            target = home_dir;
        } else if (target[1] == '/') {
            target = home_dir + target.substr(1);
        }
    } else if (!target.empty() && target[0] != '/') {
        target = currD + "/" + target;
    }

    if (chdir(target.c_str()) != 0) {
        perror("cd");
        return -1;
    }
    prevD = currD;
    currD = target;
    return 1;
}
