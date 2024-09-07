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
        return 0;
    }

    else if(cmd == ".") {
        return 1;
    }

    else if(cmd == "..") {
        if(currD == "/home") {
            return -1;
        }
        int last = currD.find_last_of("/");
        string parent = currD.substr(0, last);
        prevD = currD;
        currD = parent;
        chdir(parent.c_str());
        return 1;
    }

    else if(cmd == "-") {
        string temp = currD;
        currD = prevD;
        prevD = temp;
        chdir(prevD.c_str());
        char buf[1024];
        getcwd(buf, sizeof(buf));
        cout<<buf<<endl;
        return 1;
    }
    else {
        //If cd to some other path. If / not given, add it. If path given relative to ~, append it as such.
        //
        string temp = cmd;
        if(temp[0] != '/') {
            temp = "/"+temp;
        }

        if(strstr(temp.c_str(), "/~") != NULL) {
            temp = home_dir+temp;
        }
        else if(strstr(temp.c_str(), "/home") != NULL) {
            int uid = geteuid();
            passwd *user;
            user = getpwuid(uid);
            string username = user->pw_name;
            temp = temp.substr(6,temp.length());
            temp = "/home/"+username+"/"+temp;
        }
        else {
            temp = currD+temp;
        }
        chdir(temp.c_str());
        prevD = currD;
        currD = temp;

        return 1;
    }

}

