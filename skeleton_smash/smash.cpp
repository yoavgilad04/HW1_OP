#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"


int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
/*    if(signal(SIGXFSZ , sigXfsz)==SIG_ERR) {
        perror("smash error: failed to set SIGXFSZ handler");
    }*/
    struct sigaction sa = {0};
    sa.sa_handler=&sigXfsz;
    sa.sa_flags=SA_RESTART;
    if(sigaction(SIGXFSZ, &sa, nullptr)==-1) {
        perror("smash error: failed to set SIGXFSZ handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.GetPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}