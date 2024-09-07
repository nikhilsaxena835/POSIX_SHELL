//
// Created by nikhil-saxena on 8/28/24.
//
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <ostream>
#include <stack>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
using namespace std;
bool recursive_search(char * target) {
    stack<string> s;
    char buf[256];
    getcwd(buf, 256);
    string t = buf;
    s.push(t);
    while(!s.empty()) {
        string p = s.top();s.pop();
        DIR *dirp = opendir(p.c_str());
        struct dirent *entry;
        while ((entry = readdir(dirp)) != NULL) {
            char *temp = entry->d_name;
            if(strcmp(temp, target)==0) {
                printf("Yes\n");
                return true;
            }
            struct stat sb;

            string str = p+"/"+entry->d_name;

            if (stat(str.c_str(), &sb) == -1) {
                perror("stat");
                continue;
            }
            if((S_ISDIR(sb.st_mode)) == true) {
                if(strcmp(temp, ".")==0 || strcmp(temp, "..")==0) {continue;}
                s.push(str);
            }


        }
    }
    printf("No\n");
    return false;
}