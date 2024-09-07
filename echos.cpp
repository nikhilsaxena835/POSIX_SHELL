#include "echos.h"
#include <string>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <unistd.h>
using namespace std;


bool isEchoQuote(string str) {
    int flag = 0;
    for(int i = 0; i < str.size(); i++) {
        if(str[i] == '\"')
            flag++;
    }
    if(flag%2 == 0 && flag != 0)
        return true;
    return false;
}

string removeWhiteSpace(string str)
{
    string newString = "";
    bool lastWasWhitespace = false;  // Tracks if the last character added was a whitespace character (space or tab)

    for (char c : str) {
        if (c == ' ' || c == '\t') {
            if (!lastWasWhitespace) {
                newString.push_back(' ');  // Replace any sequence of spaces/tabs with a single space
                lastWasWhitespace = true;
            }
        } else {
            newString.push_back(c);
            lastWasWhitespace = false;
        }
    }
    return newString;
}

void echo(string &s) {
    //puts(s.c_str());
    cout << s;
}


void pwd() {
    char buf[100];
    printf("%s\n", getcwd(buf, 100));
}
