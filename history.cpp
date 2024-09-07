//
// Created by nikhil-saxena on 8/28/24.
//

#include "history.h"
#include <fstream>
#include <iostream>
#include <ostream>
#include <queue>
#include <string>

using namespace std;
const int maxSize = 20;
string filePath;

void history_initiate(vector<string> &q, string homedir) {

    filePath = homedir + "/history.txt";
    ifstream file(filePath);
    string line;

    if (file.is_open()) {
        while (getline(file, line)) {
            if (q.size() >= maxSize) {
                q.erase(q.begin());
            }
            q.push_back(line);
        }
        file.close();
    }
}


void add_history(vector<string> &q, char * command) {
    if(q.back() == command) {return;}
    if (q.size() == maxSize) {
        q.erase(q.begin());
    }
    q.push_back(command);
    save_history(q);
}

void print_history(vector<string> &q, int n) {
    if(n == 0) n = 10;
    int len = q.size();
    int start = (len > n) ? len - n : 0;

    for (int i = len - 1; i >= start; --i) {
        std::cout << q[i] << std::endl;
    }

}

void save_history(vector<string> &q) {
    ofstream outFile(filePath);
    if (!outFile) {
        cerr << "Error: Could not open file for writing: "<< endl;
        return;
    }

    for (const string& command : q) {
        outFile << command << endl;
    }

    outFile.close();
}