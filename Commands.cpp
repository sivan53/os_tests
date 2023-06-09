#include <unistd.h>
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>

#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include "signals.h"

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()

#define OVERWRITE 1
#endif

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}
//receives cmd_line and an empty array of strings args. Separates the words in cmd_line into different array slots in args. Returns the number of words in cmdline.
int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

char** getCmdArgs(const char* cmd_line, int *numOfArgs)
{
    char** argsArray = (char**) malloc(COMMAND_MAX_ARGS * sizeof(char**));
    if(!argsArray)
    {
        return nullptr;
    }
    for(int i = 0; i < COMMAND_MAX_ARGS; i++)
    {
        argsArray[i] = nullptr;
    }
    int numArgs = _parseCommandLine(cmd_line,argsArray);
    if (numArgs == -1)
    {
        argsArray = nullptr;
    }
    *numOfArgs = numArgs;
    return argsArray;
}

bool checkIfNumber(const char* input)
{
    if(*input == '-')
    {
        input++;
        if(!isdigit(*input))
        {
            return false;
        }
    }
    while(*input != '\0')
    {
        if(!isdigit(*input))
        {
            return false;
        }
        input++;
    }
    return true;
}

void freeCmdArgs(char** args)
{
    for(int i = 0; i < COMMAND_MAX_ARGS; i++)
    {
        free(args[i]);
    }
    delete[] args;
}

Command::Command(const char *cmd_line, bool active_alarm, time_t alarm_duration):cmdline(cmd_line), pid(-1), job_id(-1), active_alarm(active_alarm), alarm_duration(alarm_duration)
{}

Command::~Command() {}

BuiltInCommand::BuiltInCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration)
{
    char* cmd_no_bg = (char*) malloc(sizeof (cmd_no_bg) * string(cmdline).length() + 1);
    for (size_t i=0; i<string(cmdline).length() + 1; i++)
    {
        cmd_no_bg[i] = cmdline[i];
    }
    _removeBackgroundSign(cmd_no_bg);
    cmdline = cmd_no_bg;
}

ChangePromptCommand::ChangePromptCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration)
{
}

void ChangePromptCommand::execute(){

    SmallShell &shell = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        shell.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(this->cmdline,&num_of_args);
    if(args == nullptr)
    {
        perror("smash error: malloc failed");
        return;
    }
    if(num_of_args == 1)
    {
        shell.prompt = "smash";
        shell.fg_pid = -1;
    }
    else
    {
        shell.prompt = args[1];
    }
    freeCmdArgs(args);
}

ShowPidCommand::ShowPidCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration)
{}

void ShowPidCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    cout << "smash pid is " << smash.shell_pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration): BuiltInCommand(cmd_line, active_alarm, alarm_duration)
{}

void GetCurrDirCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    char* buffer = new char[WD_MAX_LENGTH+1];
    if(getcwd(buffer, WD_MAX_LENGTH+1) == nullptr)
    {
        delete[] buffer;
        perror("CWD size exceeds limit");
        return;
    }
    cout << buffer << endl;
    delete[] buffer;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration)
{}

void ChangeDirCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    if(num_of_args > 2)
    {
        cerr << "smash error: cd: too many arguments" << endl; //TODO
        freeCmdArgs(args);
        return;
    }
    if(num_of_args == 1) //TODO
    {
        cerr <<"smash error: cd: no arguments specified" << endl;
        freeCmdArgs(args);
        return;
    }
    int max_length = pathconf("/", _PC_PATH_MAX);
    char* curr_dir = (char*) malloc(max_length) ;
    if(!curr_dir)
    {
        perror("smash error: malloc failed");
        freeCmdArgs(args);
        return;
    }
    if(getcwd(curr_dir, max_length) == nullptr)
    {
        perror("smash error: getcwd failed");
        free(curr_dir);
        freeCmdArgs(args);
        return;
    }
    if(strcmp(args[1],"-") == 0)
    {
        //SmallShell &smash = SmallShell::getInstance();
        if(!(smash.last_wd))
        {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            free(curr_dir);
            freeCmdArgs(args);
            return;
        }
        if(chdir(smash.last_wd) == -1)
        {
            perror("smash error: chdir failed");
            free(curr_dir);
            freeCmdArgs(args);
            return;
        }
        free(smash.last_wd);
        smash.last_wd = curr_dir;
        freeCmdArgs(args);
        return;
    }
    else
    {
        if((chdir(args[1]) == -1))
        {
            perror("smash error: chdir failed");
            freeCmdArgs(args);
            return;
        }
        free(smash.last_wd);
        smash.last_wd = curr_dir;
        freeCmdArgs(args);
        return;
    }


}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration), jobs(jobs)
{}

void JobsCommand::execute()
{
    jobs->removeFinishedJobs();
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(),is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    jobs->printJobsList();
}



ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration), jobs(jobs)
{}

void ForegroundCommand::fgHelper(JobsList::JobEntry* job, char* cmdline_copy)
{
    SmallShell& smash = SmallShell::getInstance();
    if(job->stopped)
    {
        if (kill(job->pid, SIGCONT) == -1)
        {
            perror("smash error: kill failed");
            return;
        }
    }
    smash.fg_alarm = job->job_alarm;
    smash.alarm_duration = job->alarm_duration;
    if (job->job_alarm)
    {
        cout << "timeout " << job->alarm_duration << " " << cmdline_copy << " : " << job->pid << endl;
    }
    else
    {
        cout << cmdline_copy << " : " << job->pid << endl;
    }
    smash.fg_cmdline = cmdline_copy;
    smash.fg_pid = job->pid;
    smash.fg_job_id = job->job_id;
    jobs->removeJobById(job->job_id);
    if(waitpid(job->pid, nullptr, WUNTRACED) == -1) //TODO check if WUNTRACED is needed
    {
        perror("smash error: waitpid failed");
        return;
    }
}

void ForegroundCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(num_of_args > 2)
    {
        cerr << "smash error: fg: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    if(num_of_args == 2)
    {
        if(!checkIfNumber(args[1]))
        {
            cerr << "smash error: fg: invalid arguments" << endl;
            freeCmdArgs(args);
            return;
        }
        JobsList::JobEntry* job = jobs->getJobByID(stoi(args[1]));
        if(!job)
        {
            cerr << "smash error: fg: job-id " << args[1] << " does not exist" << endl;
            freeCmdArgs(args);
            return;
        }
        char* cmdline_copy = (char*) malloc(sizeof (cmdline_copy) * string(job->cmd_line).length() + 1);
        for (size_t i=0; i<string(job->cmd_line).length() + 1; i++)
        {
            cmdline_copy[i] = job->cmd_line[i];
        }
        fgHelper(job, cmdline_copy);
    }
    if(num_of_args == 1)
    {
        if(SmallShell::getInstance().jobs_list.list_of_jobs.empty())
        {
            cerr << "smash error: fg: jobs list is empty" << endl;
            freeCmdArgs(args);
            return;
        }
        JobsList::JobEntry* job = jobs->getLastJob();
        char* cmdline_copy = (char*) malloc(sizeof (cmdline_copy) * string(job->cmd_line).length() + 1);
        for (size_t i=0; i<string(job->cmd_line).length() + 1; i++)
        {
            cmdline_copy[i] = job->cmd_line[i];
        }
        fgHelper(job, cmdline_copy);
    }
    freeCmdArgs(args);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration), jobs(jobs)
{}


void BackgroundCommand::bgHelper(JobsList::JobEntry* job, char* cmdline_copy){
    //SmallShell& smash = SmallShell::getInstance();
    if (kill(job->pid, SIGCONT) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    job->stopped = false;
    cout << cmdline_copy << " : " << job->pid << endl;
}

void BackgroundCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(num_of_args > 2)
    {
        cerr << "smash error: bg: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    if(num_of_args == 2)
    {
        if (!checkIfNumber(args[1]))
        {
            cerr << "smash error: bg: invalid arguments" << endl;
            freeCmdArgs(args);
            return;
        }
        JobsList::JobEntry* job = jobs->getJobByID(stoi(args[1]));
        if(!job)
        {
            cerr << "smash error: bg: job-id " << args[1] << " does not exist" << endl;
            freeCmdArgs(args);
            return;

        }
        if(!job->stopped) //if running in bg
        {
            cerr << "smash error: bg: job-id " << job->job_id << " is already running in the background" << endl;
            freeCmdArgs(args);
            return;
        }
        char* cmdline_copy = (char*) malloc(sizeof (cmdline_copy) * string(job->cmd_line).length() + 1);
        for (size_t i=0; i<string(job->cmd_line).length() + 1; i++)
        {
            cmdline_copy[i] = job->cmd_line[i];
        }
        bgHelper(job, cmdline_copy);
    }
    if(num_of_args == 1)
    {
        JobsList::JobEntry* last_stopped = jobs->getLastStoppedJob();
        if(last_stopped == nullptr)
        {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            freeCmdArgs(args);
            return;
        }
        char* cmdline_copy = (char*) malloc(sizeof (cmdline_copy) * string(last_stopped->cmd_line).length() + 1);
        for (size_t i=0; i<string(last_stopped->cmd_line).length() + 1; i++)
        {
            cmdline_copy[i] = last_stopped->cmd_line[i];
        }
        bgHelper(last_stopped, cmdline_copy);
    }
    freeCmdArgs(args);
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration), jobs(jobs)
{}

void QuitCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(num_of_args > 1 && strcmp(args[1], "kill") == 0)
    {
        jobs->killAllJobs();
    }
    freeCmdArgs(args);
    exit(0);
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs, bool active_alarm, time_t alarm_duration) : BuiltInCommand(cmd_line, active_alarm, alarm_duration), jobs(jobs)
{
}



void KillCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    //char* first_arg = &args[1][1];
    if(num_of_args != 3)
    {
        cerr << "smash error: kill: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    string signal = string (args[1]).substr(1, string (args[1]).size());
    if(args[1][0] != '-' || !checkIfNumber(signal.c_str()) ||
       !checkIfNumber(args[2]))
    {
        cerr << "smash error: kill: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    JobsList::JobEntry* job = jobs->getJobByID(stoi(args[2]));
    if(job == nullptr)
    {
        cerr << "smash error: kill: job-id " << args[2] <<" does not exist" << endl;
        freeCmdArgs(args);
        return;
    }
    if(kill(job->pid, stoi(signal)) == -1)
    {
        perror("smash error: kill failed");
        freeCmdArgs(args);
        return;
    }
    if(stoi(signal) == SIGCONT)
    {
        job->stopped = false;
    }
    if(stoi(signal) == SIGSTOP)
    {
        job->stopped = true;
    }
    if(stoi(signal) == SIGKILL)
    {
        job->finished = true;
    }
    cout << "signal number " << signal << " was sent to pid " << job->pid << endl;
}

SmallShell::SmallShell() : jobs_list()
{
    prompt = "smash";
    shell_pid = getppid();
    fg_pid = -1; //TODO to init or not?
    fg_cmdline = ""; //
    last_wd = nullptr;
    fg_job_id = -1; //
    fg_alarm = false;
    alarm_duration = -1;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line, bool active_alarm, time_t alarm_duration) {
    int numOfArgs;
    char** args = getCmdArgs(cmd_line, &numOfArgs);
    string first_word = args[0];
    if(string (cmd_line).find('>') != string::npos || string (cmd_line).find(">>")
                                                      != string::npos)
    {
        return new RedirectionCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(string (cmd_line).find('|') != string::npos || string (cmd_line).find("|&")
                                                           != string::npos)
    {
        return new PipeCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("chprompt") || !first_word.compare("chprompt&"))
    {
        return new ChangePromptCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("showpid") || !first_word.compare("showpid&"))
    {
        return new ShowPidCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("pwd") || !first_word.compare("pwd&"))
    {
        return new GetCurrDirCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("cd") || !first_word.compare("cd&"))
    {
        return new ChangeDirCommand(cmd_line, &last_wd, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("jobs") || !first_word.compare("jobs&"))
    {
        return new JobsCommand(cmd_line, &jobs_list, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("fg") || !first_word.compare("fg&"))
    {
        return new ForegroundCommand(cmd_line, &jobs_list, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("bg") || !first_word.compare("bg&"))
    {
        return new BackgroundCommand(cmd_line,&jobs_list, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("quit") || !first_word.compare("quit&"))
    {
        return new QuitCommand(cmd_line, &jobs_list, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("kill") || !first_word.compare("kill&"))
    {
        return new KillCommand(cmd_line, &jobs_list, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("setcore") || !first_word.compare("setcore&"))
    {
        return new SetcoreCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("getfiletype") || !first_word.compare("getfiletype&"))
    {
        return new GetFileTypeCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("chmod") || !first_word.compare("chmod&"))
    {
        return new ChmodCommand(cmd_line, active_alarm, alarm_duration);
    }
    else if(!first_word.compare("timeout") || !first_word.compare("timeout&"))
    {
        return new TimeoutCommand(cmd_line);
    }
    else
    {
        return new ExternalCommand(cmd_line, active_alarm, alarm_duration);
    }
    // For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) {
    // TODO: Add your implementation here
    Command* cmd = CreateCommand(cmd_line, active_alarm, alarm_duration);
    jobs_list.removeFinishedJobs();
    cmd->execute();
    delete cmd;
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}


ExternalCommand::ExternalCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration), failed(false)
{}

void ExternalCommand::execute()
{
    AlarmList::Alarm* possible_alarm = nullptr;
    int num_of_args;
    bool isBackGround =_isBackgroundComamnd(cmdline);
    char* cmd_no_bg = (char*) malloc(sizeof (cmd_no_bg) * string(cmdline).length() + 1);
    for (size_t i=0; i<string(cmdline).length() + 1; i++)
    {
        cmd_no_bg[i] = cmdline[i];
    }
    _removeBackgroundSign(cmd_no_bg);
    char** args = getCmdArgs(cmd_no_bg,&num_of_args);
    bool isComplex = false;
    const char* temp = cmd_no_bg;
    while(*temp)
    {
        if(*temp == '?' || *temp == '*')
        {
            isComplex = true;
        }
        temp++;
    }
    char bash[] = "/bin/bash";
    char flag[] = "-c";
    char* complex_args[] = {bash, flag, const_cast<char *>(cmd_no_bg), nullptr};
    pid_t pid = fork();
    SmallShell &smash = SmallShell::getInstance();
    if(pid == -1)
    {
        perror("smash error: fork failed");
        freeCmdArgs(args);
        return;
    }
    if(pid == 0) //son
    {
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            freeCmdArgs(args);
            return;
        }
        if(!isComplex)
        {
            if(execvp(args[0],args) < 0)
            {
                perror("smash error: execvp failed");
                failed = true;
                exit(-1);
            }
        }
        else
        {
            if(execvp(bash, complex_args) == -1)
            {
                perror("smash error: execvp failed");
                failed = true;
                exit(-1);
            }
        }
        exit(0);
    }
    else //parent
    {
        if(active_alarm)
        {
            bool is_fg = true;
            if(_isBackgroundComamnd(cmdline))
            {
                is_fg = false;
            }
            AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,pid, is_fg, false);
            possible_alarm = cmd_alarm;
            smash.alarm_list.addAlarm(cmd_alarm);
        }
        if (failed)
        {
            return;
        }
        //string copy = string (cmd_no_bg);
        if (isBackGround) //if background command
        {
            string t_string = "timeout " + to_string(alarm_duration) + " ";
            char* cmdline_copy = (char*) malloc(sizeof (cmdline_copy) * (string(cmdline).length() + t_string.length()) + 1);
            if(active_alarm) {
                for (size_t i = 0; i < string(cmdline).length() + t_string.length() + 1; i++) {
                    if(i < t_string.length())
                    {
                        cmdline_copy[i] = t_string[i];
                    }
                    else {
                        cmdline_copy[i] = cmdline[i - t_string.length()];
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < string(cmdline).length() + 1; i++)
                {
                    cmdline_copy[i] = cmdline[i];
                }
            }
            smash.jobs_list.addJob(cmdline_copy,pid,job_id,false );
        }
        else
        {
            if(active_alarm)
            {
                smash.fg_alarm = true;
                smash.alarm_duration = alarm_duration;
            }
            smash.fg_cmdline = cmdline;
            smash.fg_pid = pid;
            if(waitpid(pid, nullptr, WUNTRACED) == -1) //TODO check if WUNTRACED is needed
            {
                perror("smash error: waitpid failed");
                return;
            }
            if(possible_alarm)
            {
                possible_alarm->is_fg = false;
            }
            smash.fg_cmdline = nullptr;
            smash.fg_pid = -1;
        }
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration)
{}

void RedirectionCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, false);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    size_t pos1 = string (cmdline).find('>');
    size_t pos2 = string (cmdline).find(">>");
    int flag;
    if(pos1 == string::npos && pos2 == string::npos)
    {
        cerr <<"Invalid format" << endl;
        return;
    }
    string cmd;
    string out;
    if(pos2 == string::npos) //if >
    {
        flag = OVERWRITE; // >
        cmd = string(cmdline).substr(0, pos1);
        cmd = _trim(cmd);
        out = string(cmdline).substr(pos1 + 1, string(cmdline).length());
        out = _trim(out);
    }
    else
    {
        flag = 0; // >>
        cmd = string(cmdline).substr(0,pos2);
        cmd = _trim(cmd);
        out = string (cmdline).substr(pos2+2, string(cmdline).length());
        out = _trim(out);
    }
    Command* cmd_exe = smash.CreateCommand(cmd.c_str(),active_alarm,alarm_duration);
    pid_t pid = fork();

    if(pid == -1)
    {
        perror("smash error: fork failed");
    }
    if(pid == 0)
    {
        //cout << getpid() << endl;
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            exit(1);
        }
        int  old_fd;
        if(flag == OVERWRITE)
        {
            old_fd = open(string(out).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
            if(old_fd == -1)
            {
                perror("smash error: open failed");
                exit(1);
            }
        }
        else
        {
            old_fd = open(string(out).c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);
            if(old_fd == -1)
            {
                perror("smash error: open failed");
                exit(1);
            }
        }
        if(dup2(old_fd,1) == -1)
        {
            perror("smash error: dup2 failed");
            //exit(1);
        }
        cmd_exe->execute();
        if(!_isBackgroundComamnd(cmd.c_str()))
        {
            delete cmd_exe;
        }
        exit(0);
    }
    else
    {
        //cout << pid << endl;
        if (waitpid(pid, nullptr, WUNTRACED) == -1)
        {
            perror("smash error: waitpid failed");
        }
    }
}

PipeCommand::PipeCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration)
{}

bool closePipe(int fd[])
{
    if(close(fd[0]) == -1 || close(fd[1]) == -1)
    {
        perror("smash error: close failed");
        return false;
    }
    return true;
}

void PipeCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, false);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    size_t pos1 = string (cmdline).find('|');
    size_t pos2 = string (cmdline).find("|&");
    int flag;
    if(pos1 == string::npos && pos2 == string::npos)
    {
        cerr <<"Invalid format" << endl;
        return;
    }
    string cmd1;
    string cmd2;
    if(pos2 == string::npos) //if |
    {
        flag=1;
        cmd1 = string(cmdline).substr(0, pos1);
        cmd1 = _trim(cmd1);
        cmd2 = string(cmdline).substr(pos1 + 1, string(cmdline).length());
        cmd2 = _trim(cmd2);
    }
    else
    {
        flag=2;
        cmd1 = string(cmdline).substr(0,pos1);
        cmd1 = _trim(cmd1);
        cmd2 = string (cmdline).substr(pos1+2, string(cmdline).length());
        cmd2 = _trim(cmd2);
    }
    int fd[2];
    if(pipe(fd) == -1)
    {
        perror("smash error: pipe failed");
        exit(1);
    }
    pid_t pid1 = fork();
    if(pid1 == -1)
    {
        perror("smash error: fork failed");
        closePipe(fd);
        exit(1);
    }
    if(pid1 == 0)
    {
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            closePipe(fd);
            exit(1);
        }
        if(dup2(fd[1],flag) == -1)
        {
            perror("smash error: dup2 failed");
            closePipe(fd);
            exit(1);
        }
        closePipe(fd);
        smash.executeCommand(cmd1.c_str(), alarm_duration);
        exit(0);
    }
    /*if (waitpid(pid1, nullptr, WUNTRACED) == -1) //TODO where to put waitpids
    {
        perror("smash error: waitpid failed");
        return;
    }*/
    pid_t pid2 = fork();
    if(pid2 == -1)
    {
        perror("smash error: fork failed");
        closePipe(fd);
        exit(1);
    }

    if(pid2 == 0)
    {
        if(setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            closePipe(fd);
            exit(1);
        }
        if(dup2(fd[0],0) == -1)
        {
            perror("smash error: dup2 failed");
            closePipe(fd);
            exit(1);
        }
        closePipe(fd);
        smash.executeCommand(cmd2.c_str(), alarm_duration);
        exit(0);
    }
    closePipe(fd);
    if (waitpid(pid1, nullptr, WUNTRACED) == -1) //TODO where to put waitpids
    {
        perror("smash error: waitpid failed");
        return;
    }
    if(waitpid(pid2, nullptr,WUNTRACED) == -1)
    {
        perror("smash error: waitpid failed");
        return;
    }
}

SetcoreCommand::SetcoreCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration)
{}

void SetcoreCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(num_of_args != 3 || !checkIfNumber(args[1]) || !checkIfNumber(args[2]))
    {
        cerr << "smash error: setcore: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    JobsList::JobEntry* job = smash.jobs_list.getJobByID(stoi(args[1]));
    if(job == nullptr)
    {
        cerr << "smash error: setcore: job-id " << args[1] << " does not exist" << endl;
        freeCmdArgs(args);
        return;
    }
    int core_num = sysconf(_SC_NPROCESSORS_ONLN);
    if(core_num == -1)
    {
        perror("smash error: sysconf failed");
        freeCmdArgs(args);
        return;
    }
    if(stoi(args[2]) < 0 || core_num <= stoi(args[2]))
    {
        cerr << "smash error: setcore: invalid core number" << endl;
        freeCmdArgs(args);
        return;
    }
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(stoi(args[2]), &mask);
    if(sched_setaffinity(job->pid, sizeof(mask), &mask) == -1)
    {
        perror("smash error: sched_setaffinity failed");
        freeCmdArgs(args);
        return;
    }
    freeCmdArgs(args);
}

GetFileTypeCommand::GetFileTypeCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration)
{}

void GetFileTypeCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(num_of_args != 2)
    {
        cerr << "smash error: getfiletype: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    struct stat buffer;
    int stat_res = lstat(args[1], &buffer);
    if(stat_res == -1)
    {
        perror("smash error: stat failed");
        freeCmdArgs(args);
        return;
    }
    //bool file_exists = (stat_res == 0);
    if(S_ISDIR(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"directory\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else if(S_ISREG(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"regular file\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else if(S_ISCHR(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"character device\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else if(S_ISBLK(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"block device\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else if(S_ISFIFO(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"FIFO\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else if(S_ISLNK(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"symbolic link\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else if(S_ISSOCK(buffer.st_mode))
    {
        cout <<  args[1] <<"'s type is \"socket\" and takes up " << buffer.st_size << " bytes" << endl;
    }
    else //filetype doesn't match known types
    {
        cerr <<"smash error: getfiletype: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }//success
    freeCmdArgs(args);
}

ChmodCommand::ChmodCommand(const char *cmd_line, bool active_alarm, time_t alarm_duration) : Command(cmd_line, active_alarm, alarm_duration)
{}

void ChmodCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    if(active_alarm)
    {
        bool is_fg = true;
        if(_isBackgroundComamnd(cmdline))
        {
            is_fg = false;
        }
        AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmdline,time(NULL), alarm_duration,getpid(), is_fg, true);
        smash.alarm_list.addAlarm(cmd_alarm);
    }
    int num_of_args = -1;
    char** args = getCmdArgs(cmdline,&num_of_args);
    struct stat buffer;
    int stat_res = stat(args[2], &buffer);
    if(num_of_args != 3 || !checkIfNumber(args[1]) || args[1][0] == '-')
    {
        cerr << "smash error: chmod: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    if(stat_res == -1)
    {
        cerr << "smash error: stat failed" << endl;
        freeCmdArgs(args);
        return;
    }
    int mode = strtol(args[1], NULL, 8);
    if(chmod(args[2], mode) == -1)
    {
        perror("smash error: chmod failed");
        freeCmdArgs(args);
        return;
    }
    freeCmdArgs(args);
}

AlarmList::AlarmList() : alarm_list()
{}

AlarmList::Alarm::Alarm(const char *cmd_line, time_t timestamp, time_t duration, pid_t pid, bool is_fg, bool is_builtin) : cmd_line(cmd_line), timestamp(timestamp), duration(duration), pid(pid), is_fg(is_fg), is_builtin(is_builtin)
{}

void AlarmList::addAlarm (AlarmList::Alarm* new_alarm)
{
    alarm_list.push_back(new_alarm);
    time_t min_alarm = difftime(new_alarm->timestamp + new_alarm->duration, time(NULL));
    for(auto alarm = alarm_list.begin(); alarm != alarm_list.end(); alarm++)
    {
        if (difftime(alarm.operator*()->timestamp + alarm.operator*()->duration, time(NULL)) < min_alarm)
        {
            min_alarm = difftime(alarm.operator*()->timestamp + alarm.operator*()->duration, time(NULL));
        }
    }
    alarm(min_alarm);
}

void AlarmList::endAlarms()
{
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list.removeFinishedJobs();
    for(auto alarm = alarm_list.begin(); alarm != alarm_list.end(); alarm++)
    {
        if(difftime(time(NULL), alarm.operator*()->timestamp) >= alarm.operator*()->duration)
        {
            //cout << "shell pid: " << smash.fg_pid << endl;
            //cout << "timed out pid: " << alarm.operator*()->pid << endl;
            if((smash.jobs_list.getJobByPID(alarm.operator*()->pid) || alarm.operator*()->is_fg) && !alarm.operator*()->is_builtin)
            {
                cout << "smash: timeout " << alarm.operator*()->duration <<" " << alarm.operator*()->cmd_line << " timed out!" << endl; //TODO
                if (kill(alarm.operator*()->pid, SIGKILL) == -1) {
                    perror("smash error: kill failed");
                    return;
                }
            }
            if (alarm.operator*()->is_fg)
            {
                smash.fg_pid = -1;
                smash.fg_job_id = -1;
                smash.fg_cmdline = "";
            }
        }
    }
    alarm_list.remove_if([](Alarm *alarm) {return (difftime(time(NULL), alarm->timestamp) >= alarm->duration);});
    if(alarm_list.empty())
        return;
    time_t min_alarm = difftime(alarm_list.begin().operator*()->timestamp + alarm_list.begin().operator*()->duration, time(NULL));
    for(auto alarm = alarm_list.begin(); alarm != alarm_list.end(); alarm++)
    {
        if (difftime(alarm.operator*()->timestamp + alarm.operator*()->duration, time(NULL)) < min_alarm)
        {
            min_alarm = difftime(alarm.operator*()->timestamp + alarm.operator*()->duration, time(NULL));
        }
    }
    alarm(min_alarm);
}

TimeoutCommand::TimeoutCommand(const char *cmd_line) : Command(cmd_line, false, -1)
{}

void TimeoutCommand::execute()
{
    int num_of_args;
    char** args = getCmdArgs(cmdline,&num_of_args);
    if(args[1] == nullptr || !checkIfNumber(args[1]) || args[1][0] == '-' || num_of_args <=2)
    {
        cerr <<"smash error: timeout: invalid arguments" << endl;
        freeCmdArgs(args);
        return;
    }
    int i = 2;
    string cmd;
    while(args[i] != nullptr)
    {
        cmd.append(args[i]);
        cmd.append(" ");
        i++;
    }
    cmd = _trim(cmd);
    char* cmd_copy = (char*) malloc(sizeof (cmd_copy) * cmd.length() + 1);
    for (size_t i=0; i<cmd.length() + 1; i++)
    {
        cmd_copy[i] = cmd[i];
    }
    Command* cmd_exe = SmallShell::getInstance().CreateCommand(cmd_copy, true, stoi(args[1])); //TODO - verify the significance of alarmlist: Why is it needed, for which types of processes, etc.
    //alarm(stoi(args[1]));
    /*AlarmList::Alarm* cmd_alarm = new AlarmList::Alarm(cmd_copy,time(NULL),stoi(args[1]),cmd_exe->pid);
    if (!_isBackgroundComamnd(cmd_copy))
    {
        cmd_alarm->is_fg = true;
    }
    SmallShell::getInstance().alarm_list.addAlarm(cmd_alarm);
     */
    cmd_exe->execute();
    delete cmd_exe;
}


//TODO JobsList Functions:

JobsList::JobsList():list_of_jobs(), nextJobID(1)
{
}

JobsList::~JobsList() {}

JobsList::JobEntry::JobEntry(bool stopped, int job_id, const char* cmd_line, pid_t pid, bool job_alarm, time_t alarm_duration):finished(false),stopped(stopped),job_id(job_id), pid(pid), cmd_line(cmd_line), time_added(-1), job_alarm(job_alarm), alarm_duration(alarm_duration)
{}



void JobsList::addJob(const char* cmd_line, pid_t pid, int job_id, bool isStopped, bool job_alarm, time_t alarm_duration)
{
    JobsList::removeFinishedJobs();
    JobEntry new_job(isStopped,nextJobID, cmd_line, pid, job_alarm, alarm_duration);
    if(job_id != -1)
    {
        new_job.job_id = job_id;
    }
    new_job.time_added = time(nullptr);
    if(job_id == -1)
    {
        nextJobID++;
    }
    list_of_jobs.push_back(new_job);

}

void JobsList::printJobsList() //TODO check space validity
{
    //SmallShell& smash = SmallShell::getInstance();
    list_of_jobs.sort([](const JobEntry &job1, const JobEntry &job2){return job1.job_id < job2.job_id;});
    for(const auto& job : list_of_jobs)
    {
        if(!job.finished)
        {
            if (!job.stopped)
            {
                if(job.job_alarm)
                {
                    cout << "[" << job.job_id << "] " << "timeout " << job.alarm_duration << " " <<job.cmd_line << " : " << job.pid << " "
                         << (int) difftime(time(nullptr), job.time_added) << " secs" << endl;
                }
                else
                {
                    cout << "[" << job.job_id << "] " << job.cmd_line << " : " << job.pid << " "
                         << (int) difftime(time(nullptr), job.time_added) << " secs" << endl;
                }
            }
            else
            {
                if(job.job_alarm)
                {
                    cout << "[" << job.job_id << "] " << "timeout " << job.alarm_duration << " " << job.cmd_line << " : " << job.pid << " "
                         << (int) difftime(time(nullptr), job.time_added) << " secs (stopped)" << endl;
                }
                else
                {
                    cout << "[" << job.job_id << "] " << job.cmd_line << " : " << job.pid << " "
                         << (int) difftime(time(nullptr), job.time_added) << " secs (stopped)" << endl;
                }

            }
        }
    }
}

void JobsList::removeFinishedJobs()
{
    for (auto &job: list_of_jobs) {
        if (job.pid == waitpid(job.pid, NULL, WNOHANG)) {
            job.finished = true;
        }
    }
    list_of_jobs.remove_if([](JobEntry job) { return job.finished; }); //TODO check validity
    int max_job_id = 1;
    if (list_of_jobs.empty())
    {
        this->nextJobID = 1;
        return;
    }
    for(auto& job : list_of_jobs)
    {
        if(job.job_id > max_job_id)
        {
            max_job_id = job.job_id;
        }
    }
    /*if (smash.fg_job_id > max_job_id)
    {
        max_job_id = smash.fg_job_id;
    }*/
    this->nextJobID = max_job_id + 1;
}

JobsList::JobEntry* JobsList::getJobByID(int jobId)
{
    for(auto& job : list_of_jobs)
    {
        if(job.job_id == jobId)
        {
            return &job;
        }
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getJobByPID(pid_t pid)
{
    for(auto& job : list_of_jobs)
    {
        if(job.pid == pid)
        {
            return &job;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId)
{
    //list_of_jobs.remove_if([&jobId](JobEntry job) {if(job.job_id == jobId) return true;}); //TODO check validity
    removeFinishedJobs();
    for(auto job = list_of_jobs.begin(); job != list_of_jobs.end(); job++)
    {
        if(job->job_id == jobId)
        {
            list_of_jobs.erase(job);
            job--;
            return;
        }
    }
}

JobsList::JobEntry *JobsList::getLastJob()
{
    return &list_of_jobs.back();
}

JobsList::JobEntry* JobsList::getLastStoppedJob()
{
    JobEntry* last_stopped = nullptr;

    for(auto& job : list_of_jobs)
    {
        if(job.stopped)
        {
            last_stopped = &job;
        }
    }
    return last_stopped;
}

void JobsList::killAllJobs()
{
    cout << "smash: sending SIGKILL signal to "<< list_of_jobs.size() << " jobs:" << endl;

    for(auto& job : list_of_jobs)
    {
        cout << job.pid << ": " << job.cmd_line << endl;
    }
    for(auto& job : list_of_jobs)
    {

        if (!job.finished)
        {
            if(kill(job.pid,SIGKILL) == -1)
            {
                //perror("smash error: kill failed");
                return;
            }
            job.finished = true;
        }
    }
}
