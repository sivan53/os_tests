#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if(smash.fg_pid == -1)
    {
        return;
    }
    char* cmdline_copy = (char*) malloc(sizeof (cmdline_copy) * string(smash.fg_cmdline).length() + 1);
    for (size_t i=0; i<string(smash.fg_cmdline).length() + 1; i++)
    {
        cmdline_copy[i] = smash.fg_cmdline[i];
    }
    smash.jobs_list.addJob(cmdline_copy,smash.fg_pid,smash.fg_job_id,true);
    if(kill(smash.fg_pid, SIGSTOP) == -1)
    {
        perror("smash: kill failed");
        return;
    }
    cout << "smash: process " << smash.fg_pid <<" was stopped" << endl;
    smash.fg_pid = -1;
    smash.fg_job_id = -1;
    smash.fg_cmdline = "";
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    if(smash.fg_pid == -1)
    {
        return;
    }
    if(kill(smash.fg_pid, SIGKILL) == -1)
    {
        perror("smash: kill failed");
        return;
    }
    cout << "smash: process " << smash.fg_pid <<" was killed" << endl;
    smash.fg_pid = -1;
    smash.fg_cmdline = "";
    smash.fg_job_id = -1;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
  SmallShell& smash = SmallShell::getInstance();
  /*if(smash.fg_cmdline != "" && smash.fg_timeout)
  {
      if(kill(smash.fg_pid, SIGINT) == -1)
      {
          perror("smash error: kill failed");
      }
      smash.fg_pid = -1;
      smash.fg_timeout = false;
      smash.fg_job_id = -1;
      smash.fg_cmdline = "";
  }*/
  smash.alarm_list.endAlarms();
  /*if(kill(smash.fg_pid, SIGKILL) == -1)
  {
      perror("smash: kill failed");
      return;
  }
    cout << "smash: got an alarm" << endl;
    cout << "smash: " << smash.fg_cmdline << " timed out!" << endl;*/
}

