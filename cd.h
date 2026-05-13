//
// Created by nikhil-saxena on 8/27/24.
//

#ifndef CD_H
#define CD_H
#include <string>
#include <dirent.h>
int changeDirectory(const std::string& cmd, DIR *curr, DIR *prev, std::string &currD, std::string &prevD, const std::string &home_dir);
#endif //CD_H
