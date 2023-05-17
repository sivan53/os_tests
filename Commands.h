#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

#define WD_MAX_LENGTH (80)

using namespace std;

class Command {
 public:
    const char* cmdline;
    pid_t pid;
    int job_id;
    bool active_alarm;
    time_t alarm_duration;


  Command(const char* cmd_line, bool active_alarm, time_t alarm_duration);

  virtual ~Command();

  virtual void execute() = 0;
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line, bool active_alarm, time_t alarm_duration = -1);

  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
    bool failed;
  ExternalCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);

  virtual ~ExternalCommand() {}

  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand
{
public:
    explicit ChangePromptCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
    virtual ~ChangePromptCommand(){}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
//char** last_wd;
  ChangeDirCommand(const char* cmd_line, char** plastPwd, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~ShowPidCommand() {}
  void execute() override;
};
//
//

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
  public:
   bool finished;
   bool stopped;
   int job_id;
   pid_t pid;
   const char* cmd_line;
   time_t time_added;
   bool job_alarm;
   time_t alarm_duration;
   JobEntry(bool stopped, int job_id, const char* cmd_line, pid_t pid, bool job_alarm, time_t alarm_duration);
  };
 // TODO: Add your data members
 list<JobEntry> list_of_jobs;
 int nextJobID;
  JobsList();
  ~JobsList();
  void addJob(const char* cmd_line, pid_t pid, int job_id, bool isStopped = false, bool job_alarm = false, time_t alarm_duration = -1);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  void removeJobById(int jobId);
  JobEntry * getLastJob();
  JobEntry *getLastStoppedJob();
  JobEntry* getJobByID(int job_id);
  JobEntry* getJobByPID(pid_t pid);
  // TODO: Add extra methods or modify existing ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
    JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
    JobsList *jobs;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~ForegroundCommand() {}
  void execute() override;
  void fgHelper(JobsList::JobEntry* job, char* cmdline_copy);
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList *jobs;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~BackgroundCommand() {}
  void execute() override;
    void bgHelper(JobsList::JobEntry* job, char* cmdline_copy);
};

class TimeoutCommand : public Command {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand: public Command {
  // TODO: Add your data members
 public:
  ChmodCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public Command {
  // TODO: Add your data members
 public:
  GetFileTypeCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public Command {
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 JobsList* jobs;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs, bool active_alarm = false, time_t alarm_duration = -1);
  virtual ~KillCommand() {}
  void execute() override;
};

class AlarmList
{
public:
    class Alarm
    {
    public:
        const char* cmd_line;
        time_t timestamp;
        time_t duration;
        pid_t pid;
        bool is_fg;
        bool is_builtin;
        Alarm(const char* cmd_line, time_t timestamp, time_t duration, pid_t pid, bool is_fg, bool is_builtin);
    };
    list <Alarm*> alarm_list;
    void addAlarm (Alarm* new_alarm);
    AlarmList();
    void endAlarms();
};

class SmallShell {
 private:
  // TODO: Add your data members
  SmallShell();
 public:
    string prompt;
    pid_t shell_pid;
    pid_t fg_pid;
    char* last_wd;
    //Command* fg_cmd;
    //bool fg_timeout;
    const char* fg_cmdline;
    int fg_job_id;
    JobsList jobs_list;
    AlarmList alarm_list;
    bool fg_alarm;
    time_t alarm_duration;
  Command *CreateCommand(const char* cmd_line, bool active_alarm, time_t alarm_duration = -1);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line, bool active_alarm, time_t alarm_duration = -1);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
