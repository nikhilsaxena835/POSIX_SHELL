//
// Created by nikhil-saxena on 8/28/24.
//

#ifndef LS_H
#define LS_H
#include <string>

void lsInitiate(char * args[], int count, const std::string &home_dir);
void onlyLS();
void minusA(const char *dirname, const std::string &home_dir);
void dirLS(const char *dirname, const std::string &home_dir);
void minusAL(const char *dirname, const std::string &home_dir);
void lsMain(char * args[], std::string home_dir);
void minusL(const char *dirname, const std::string &home_dir);
#endif //LS_H
