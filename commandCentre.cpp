//
// Created by nikhil-saxena on 8/27/24.
//
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include "getInfo.h"
#include <sys/wait.h>
#include "commandCentre.h"
#include "cd.h"
#include "echos.h"
#include "ls.h"
#include "search.h"
#include "pinfo.h"
#include "history.h"

string commandList[] = {"cd","echo","pwd", "ls", "pinfo", "search", "history"};

int isMyCommand(const char * temp) {
    string command = temp;
    for(int i = 0; i < 7; i++) {
        if(commandList[i] == command) {
            return i;
        }
    }
    return -1;
}


void executeCommand(int index,
    const char *newdir, DIR *curr, DIR *prev, string &currD, string &prevD,
    char** args, const string &home_dir,
    vector<string> historyStore, int count){

        int i = 0;
        while(args[i]!= NULL) i++;
        switch (index) {
            case 0:
            {
                int x;
                if(count > 2) {
                    cout<<"Error: CD got more than one arguments"<<endl;
                }
                else if(count == 2) {
                    x = changeDirectory(newdir, curr, prev, currD, prevD, home_dir);
                }
                else {
                    x = 0;
                }

                if(x==0) {
                    string temp = currD;
                    currD = home_dir;
                    prevD = temp;
                    chdir(currD.c_str());
                }
                if(x==1 || x==-1) {
                    return ;
                }

            }   break;

            case 1: {
                string temp = args[1];
                echo(temp);
            }   break;

            case 2: pwd();
            break;

            case 3: {
                lsMain(args, home_dir);
            }
            break;
            case 4: {
                if(args[1] == NULL) {
                    string zero = "0";
                    args[1] = (char *)zero.c_str();
                }
                getPInfor(atoi(args[1]));
            }
            break;
            case 5: {
                recursive_search(args[1]);
            }
            break;;
            case 6: {
                if(args[1] == NULL)
                {
                    string zero = "0";
                    args[1] = (char *)zero.c_str();

                    }
                print_history(historyStore,atoi(args[1]));
            }
                break;
            default : cout<<" Invalid command"<<endl;
        }
    }


