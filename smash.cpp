#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction alarm_action = {0};
    alarm_action.sa_flags = SA_RESTART;
    alarm_action.sa_handler = &alarmHandler;
    sigemptyset(&alarm_action.sa_mask);
    if(sigaction(SIGALRM, &alarm_action, NULL) == -1)
    {
        perror("smash error: failed to set alarm handler");
    }


    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.prompt << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        //cout << "main pid: " << getpid() << endl;
        if(cmd_line != "")
        {
            smash.executeCommand(cmd_line.c_str(), false);
        }
    }
    return 0;
}