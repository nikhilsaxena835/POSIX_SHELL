//
// Created by nikhil-saxena on 8/28/24.
//

#ifndef HISTORY_H
#define HISTORY_H
#include <string>
#include <vector>
using namespace std;
void history_initiate(vector<string> &q, string);
void add_history(vector<string> &q, char *);
void print_history(vector<string> &q, int n);
void save_history(vector<string> &q);
#endif //HISTORY_H
