//
// Created by nikhil-saxena on 8/28/24.
//
#include "ls.h"

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <ostream>
#include <pwd.h>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>

using namespace std;

const int BUF_SIZE = 1024;

struct EntryInfo {
    string name;
    struct stat sb;
};

static void listLong(const char *dirname, const string &home_dir, bool show_all) {
    string base = ".";
    if (dirname != NULL) {
        if (strcmp(dirname, "~") == 0) {
            base = home_dir;
        } else {
            base = dirname;
        }
    }

    DIR *dir = opendir(base.c_str());
    if (dir == NULL) {
        cout << "Failed to open directory. Check Path\n";
        return;
    }

    vector<EntryInfo> entries;
    long total_blocks = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }
        string path = base;
        if (!path.empty() && path.back() != '/') {
            path += "/";
        }
        path += entry->d_name;

        EntryInfo info;
        info.name = entry->d_name;
        if (lstat(path.c_str(), &info.sb) == -1) {
            perror("lstat");
            continue;
        }
        total_blocks += info.sb.st_blocks;
        entries.push_back(info);
    }
    closedir(dir);

    long total_k = total_blocks / 2;
    printf("Total %ld\n", total_k);

    for (const auto &info : entries) {
        const struct stat &sb = info.sb;
        passwd *psw = getpwuid(sb.st_uid);
        group *g = getgrgid(sb.st_gid);

        const char *uid = psw ? psw->pw_name : "unknown";
        const char *gid = g ? g->gr_name : "unknown";

        printf((S_ISDIR(sb.st_mode)) ? "d" : "-");
        printf((sb.st_mode & S_IRUSR) ? "r" : "-");
        printf((sb.st_mode & S_IWUSR) ? "w" : "-");
        printf((sb.st_mode & S_IXUSR) ? "x" : "-");
        printf((sb.st_mode & S_IRGRP) ? "r" : "-");
        printf((sb.st_mode & S_IWGRP) ? "w" : "-");
        printf((sb.st_mode & S_IXGRP) ? "x" : "-");
        printf((sb.st_mode & S_IROTH) ? "r" : "-");
        printf((sb.st_mode & S_IWOTH) ? "w" : "-");
        printf((sb.st_mode & S_IXOTH) ? "x" : "-");
        printf("\t%ld", sb.st_nlink);
        printf("\t%s", uid);
        printf("\t%s", gid);
        printf("\t%ld", sb.st_size);
        char *time_str = strtok(ctime(&sb.st_mtim.tv_sec), "\n");
        printf("\t%s", time_str);
        printf("\t%s\n", info.name.c_str());
    }
}
void onlyLS() {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        cout << "Failed to open directory. Check Path\n";
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char *temp = entry->d_name;
        if(temp[0] != '.')
        printf("%s\n", temp);
        }
    closedir(dir);
    }

void minusA(const char* dirname, const string &home_dir) {
    DIR *dir;
    if(dirname == NULL)
    dir = opendir(".");
    else {
        if(strcmp(dirname, "~") == 0) dirname = home_dir.c_str();
        dir = opendir(dirname);
    }
    if (dir == NULL) {
        cout << "Failed to open directory. Check Path\n";
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char *temp = entry->d_name;
        printf("%s\n", temp);
    }
    closedir(dir);
}

void dirLS(const char *dirname, const string &home_dir) {
    if(strcmp(dirname, "~") == 0) dirname = home_dir.c_str();
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        cout << "Failed to open directory. Check Path\n";
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char *temp = entry->d_name;
        if(temp[0] != '.')
        printf("%s\n", temp);
    }
    closedir(dir);
}

void minusL(const char *dirname, const string &home_dir) {
    listLong(dirname, home_dir, false);
}

void minusAL(const char* dirname, const string &home_dir) {
    listLong(dirname, home_dir, true);
}


void lsMain(char* args[], string home_dir) {
    int i = 0, j = 0;
    vector<char*> flags;  // Store flags for reuse with each directory

    while (args[i] != nullptr) {
        string x = args[i];

        // Check if the argument is a flag or a directory
        if (x == "-l" || x == "-a" || x == "-la" || x == "-al") {
            flags.push_back(args[i]);  // Store the flags
        } else if (x != "ls" && x != "-") {
            // Found a directory or a non-flag argument
            int flag_count = flags.size();
            int command_length = flag_count + 2;  // +2 for "ls" and the directory

            char* atomic_ls[command_length + 1]; // +1 for nullptr
            int k = 0;

            atomic_ls[k++] = (char*)"ls";  // Add "ls" at the start

            // Add all the flags
            for (auto flag : flags) {
                atomic_ls[k++] = flag;
            }

            atomic_ls[k++] = args[i];
            atomic_ls[command_length] = nullptr; // Null-terminate the array
            lsInitiate(atomic_ls, command_length, home_dir);
            flags.clear();

            j = i + 1; // Move the start index for the next command
        }
        i++;
    }

    // After the loop, handle the case where the last arguments need to be processed
    if (j < i) {
        int flag_count = flags.size();
        int command_length = flag_count + 1;  // +1 for "ls"

        char* atomic_ls[command_length + 1];

        atomic_ls[0] = (char*)"ls";  // Add "ls" at the start

        // Add all the flags
        for (int k = 0; k < flag_count; k++) {
            atomic_ls[k + 1] = flags[k];
        }

        atomic_ls[command_length] = nullptr;  // Null-terminate the array

        // Call the function to execute the ls command
        lsInitiate(atomic_ls, command_length, home_dir);
    }
}

void lsInitiate(char * args[], int count, const string &home_dir) {

    if(count == 1 || (strcmp(args[1], ".") == 0)) {
        onlyLS();
    }
    else if(strcmp(args[1], "-la") == 0 || strcmp(args[1], "-al") == 0) {
        minusAL(args[2], home_dir);
    }

    else if(count > 2 &&
        ((strcmp(args[1], "-l") == 0 && strcmp(args[2], "-a") == 0) ||
        (strcmp(args[2], "-l") == 0 && strcmp(args[1], "-a") == 0))) {
            minusAL(args[3], home_dir);
        }

    else if(strcmp(args[1], "-a") == 0) {
       minusA(args[2], home_dir);
    }

    else if(strcmp(args[1], "-l") == 0) {
        minusL(args[2], home_dir);
    }

    else {
        dirLS(args[1], home_dir);
    }

}
