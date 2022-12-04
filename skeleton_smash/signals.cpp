#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell &shell = SmallShell::getInstance();
    pid_t fg_pid = shell.getFgPID();
    Command* fg_cmd = shell.getFgCmd();
    int fg_job_id = shell.getFgJobID();
    if (fg_pid == -1 || fg_cmd == nullptr){
        return;
    }
    cout << "smash: got ctrl-Z" << endl ;
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
        shell.setFgJobID(-1);
    }

}

void alarmHandler(int sig_num) {
    SmallShell &shell = SmallShell::getInstance();
    TimeoutEntry * timeout = shell.getTimeoutList()->setAlarm();
    if (timeout == nullptr) {
        return;
    }
    kill(timeout->getPID(), SIGKILL);
    cout<< "smash: got an alarm" << endl;
    cout <<"smash: "<<timeout->getCommandLine() << "timed out!" << endl;
    delete timeout;
    shell.getTimeoutList()->setAlarm();
}
