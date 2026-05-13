//
// Created by nikhil-saxena on 8/25/24.
//

#include "getInfo.h"
#include <ostream>
#include <pwd.h>
#include <string>
#include <unistd.h>
#include <sys/utsname.h>

using namespace std;

int const buffer_size = 1024;



void get_name(string &systemname, string &home_dir, string &username) {
    //Get system name
    utsname utsname_struct;
    uname(&utsname_struct);
    systemname = utsname_struct.sysname;

    //Get username
    int uid = geteuid();
    passwd *user;
    user = getpwuid(uid);
    username = user->pw_name;

    //Get home directory
    const char *home_env = getenv("HOME");
    if (home_env != nullptr) {
        home_dir = home_env;
    } else if (user != nullptr && user->pw_dir != nullptr) {
        home_dir = user->pw_dir;
    } else {
        char hbuf[buffer_size];
        home_dir = getcwd(hbuf, buffer_size);
    }
    nshell_pid = getpid();
}
