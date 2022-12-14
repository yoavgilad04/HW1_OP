#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl ;
    SmallShell &shell = SmallShell::getInstance();
    pid_t fg_pid = shell.getFgPID();
    Command* fg_cmd = shell.getFgCmd();
    int fg_job_id = shell.getFgJobID();
    if (fg_pid == -1 || fg_cmd == nullptr){
        return;
    }
    if (kill(fg_pid, SIGSTOP) == -1){
        perror("smash error: kill failed");
        return;
    }
    cout<< "smash: process "<< fg_pid << " was stopped" <<endl;
    shell.GetJobList()->addJob(shell.getFgCmd(),fg_pid,true, fg_job_id);
    shell.setFgCmd(nullptr);
    shell.setFgPID(-1);
    shell.setFgJobID(-1);
    return;
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl ;
    SmallShell &shell = SmallShell::getInstance();
    if (shell.getFgPID() == -1 || shell.getFgCmd() == nullptr){
        return;
    }
    else{
        pid_t fg_pid = shell.getFgPID();
        if (kill(fg_pid, SIGINT) == -1){
            perror("smash error: kill failed");
            return;
        }
        cout<< "smash: process "<< fg_pid << " was killed" <<endl;
        shell.setFgCmd(nullptr);
        shell.setFgPID(-1);
        shell.setFgJobID(-1);
    }

}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}
