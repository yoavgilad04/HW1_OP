#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &shell = SmallShell::getInstance();
    if (shell.getFgPID() == -1 || shell.getFgCmd() == nullptr){
        return;
    }
    cout << "smash: got ctrl-Z" << endl ;
    pid_t fg_pid = shell.getFgPID();
    if (kill(fg_pid, SIGSTOP) == -1){
        perror("smash error: kill failed");
        return;
    }
    cout<< "smash: process "<< fg_pid << " was stopped" <<endl;
    shell.GetJobList()->addJob(shell.getFgCmd(),fg_pid,true);
    shell.setFgCmd(nullptr);
    shell.setFgPID(-1);
    return;
}

void ctrlCHandler(int sig_num) {
    SmallShell &shell = SmallShell::getInstance();
    if (shell.getFgPID() == -1 || shell.getFgCmd() == nullptr){
        return;
    }
    else{
        cout << "smash: got ctrl-C" << endl ;
        pid_t fg_pid = shell.getFgPID();
        if (kill(fg_pid, SIGINT) == -1){
            perror("smash error: kill failed");
            return;
        }
        cout<< "smash: process "<< fg_pid << " was killed" <<endl;
        shell.setFgCmd(nullptr);
        shell.setFgPID(-1);
    }

}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}
