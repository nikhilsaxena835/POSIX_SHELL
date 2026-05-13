#ifndef JOBMANAGER_H
#define JOBMANAGER_H

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

enum class JobState { Foreground, Background, Stopped, Done };

struct Job {
    pid_t pgid = -1;
    JobState state = JobState::Done;
};

class JobManager {
public:
    // --- State queries ---
    bool hasForeground() const { return fg_.pgid > 0 && fg_.state == JobState::Foreground; }
    pid_t foregroundPgid() const { return fg_.pgid; }

    // --- State transitions ---
    void startForeground(pid_t pgid) {
        fg_.pgid  = pgid;
        fg_.state = JobState::Foreground;
    }

    void startBackground(pid_t pgid) {
        fg_.pgid  = pgid;
        fg_.state = JobState::Background;
        std::cout << "PID : " << pgid << std::endl;
    }

    void clearForeground() {
        fg_.pgid  = -1;
        fg_.state = JobState::Done;
    }

    // --- Signal-driven transitions ---
    void interruptForeground() {
        if (hasForeground()) {
            kill(-fg_.pgid, SIGINT);
        } else {
            std::cout << "No foreground job to interrupt";
        }
    }

    void suspendForeground() {
        if (hasForeground()) {
            setpgid(fg_.pgid, fg_.pgid);
            std::cout << "Foreground job suspended\n";
            kill(-fg_.pgid, SIGSTOP);
            fg_.state = JobState::Stopped;
            fg_.pgid  = -1;
        } else {
            std::cout << "No foreground job to suspend";
        }
    }

    void reapChildren() {
        int status = 0;
        while (waitpid(-1, &status, WNOHANG) > 0) {
        }
    }

    void waitForForeground() {
        if (fg_.pgid > 0) {
            waitpid(fg_.pgid, nullptr, WUNTRACED | WCONTINUED);
        }
    }

    void waitForPid(pid_t pid) {
        waitpid(pid, nullptr, WUNTRACED);
    }

private:
    Job fg_;
};

#endif // JOBMANAGER_H
