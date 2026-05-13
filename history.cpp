//
// Created by nikhil-saxena on 8/28/24.
//

#include "history.h"
#include <fstream>
#include <iostream>
#include <ostream>
#include <queue>
#include <string>
#include <cctype>
#include <cstdlib>

using namespace std;
const int maxSize = 20;
string filePath;
static vector<string> *history_ptr = nullptr;
static bool history_dirty = false;
static bool atexit_registered = false;
static int history_pending = 0;
static const int history_flush_interval = 10;

static bool isWhitespaceOnly(const string &s) {
    for (char c : s) {
        if (!isspace(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

static void flush_history_on_exit() {
    if (history_ptr != nullptr && history_dirty) {
        save_history(*history_ptr);
    }
}

void history_initiate(vector<string> &q, string homedir) {

    filePath = homedir + "/history.txt";
    history_ptr = &q;
    if (!atexit_registered) {
        atexit(flush_history_on_exit);
        atexit_registered = true;
    }
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
    if (command == nullptr) {
        return;
    }
    string cmd = command;
    if (isWhitespaceOnly(cmd)) {
        return;
    }
    if (!q.empty() && q.back() == cmd) {
        return;
    }
    if (q.size() == maxSize) {
        q.erase(q.begin());
    }
    q.push_back(cmd);
    history_dirty = true;
    history_pending++;
    if (history_pending >= history_flush_interval) {
        save_history(q);
        history_dirty = false;
        history_pending = 0;
    }
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
