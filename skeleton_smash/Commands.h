#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <stdio.h>
#include <climits>
#include <string.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_SIG 31
using namespace std;

class SmashErrors
{
    string pre_error = "smash error: ";
public:
    void PrintJobIDDoesntExits(string cmd,int job_id )
    {cerr <<this->pre_error<<cmd<<": "<<"job id "<<job_id<<" does not exist";}

    void PrintJobsListEmpty(string cmd)
    {cerr <<this->pre_error<<cmd<<": jobs list is empty";}

    void PrintInvalidArgs(string cmd)
    {cerr <<this->pre_error<<cmd<<": invalid arguments";}

    void PrintInvalidCore(string cmd)
    {cerr <<this->pre_error<<cmd<<": invalid core number";}

    void PrintNoStoppedJobs(string cmd)
    {cerr <<this->pre_error<<cmd<<": there is no stopped jobs to resume";}

    void PrintJobAlreadyRunningInBG(string cmd, int job_id)
    {cerr <<this->pre_error<<cmd<<": job id "<<job_id<<" is already running in the background";}

    void PrintTooManyArgs(string cmd)
    {cerr <<this->pre_error<<cmd<<": too many argumnets";}

    void PrintOLDPWDFail(string cmd)
    {cerr <<this->pre_error<<cmd<<": OLDPWD not set";}

    void PrintSysFailError(string sys_call_name)
    {
        string msg = this->pre_error + sys_call_name + " failed ";
        perror(msg.c_str());
    } //c_str convert string to char
};

class Command {
    const char* cmd_line;
protected:
    SmashErrors err;
    char** setUpArgs(char*** args, const char * cmd_line, string * cmd, int *args_num = nullptr);
 public:
  Command(const char* cmd_line){
      this->cmd_line = strdup(cmd_line);
  }
  virtual ~Command(){ delete(this->cmd_line);};
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  /***Our Own Methods*/
  const char * getCommandLine(){return cmd_line;}
  std::string getCommand();
};

class BuiltInCommand : public Command {
 public:
    BuiltInCommand(const char* cmd_line): Command(cmd_line){};
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
    pid_t * fg_pid;
public:
  ExternalCommand(const char* cmd_line, pid_t * fg_pid): Command(cmd_line), fg_pid(fg_pid){};
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  char command1[COMMAND_ARGS_MAX_LENGTH];
  char command2[COMMAND_ARGS_MAX_LENGTH];
  bool is_error;
  void closePipe(int * fd);
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
    char command[COMMAND_ARGS_MAX_LENGTH];
    char filename[PATH_MAX];
 bool is_append = false;
 int fd;
 bool is_redir;
 int copy_stdout;

 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() =default;
  void execute() override;
  void prepare() ;
  void cleanup() ;
};

class Chprompt : public BuiltInCommand {
public:
    Chprompt(const char* cmd_line): BuiltInCommand(cmd_line){}
    virtual ~Chprompt() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char** p_last_dir;
public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), p_last_dir (plastPwd){}
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList * jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
      int job_id;
      pid_t job_pid;
      time_t enter_time;
      bool is_stopped;
      Command * cmd;

  public:

      JobEntry(int job_id, pid_t job_pid, bool is_stopped, Command * cmd):
            job_id(job_id), job_pid(job_pid), is_stopped(is_stopped),cmd(cmd) {
            time(&this->enter_time);

      };
      ~JobEntry();
      bool isStopped(){return is_stopped;}
      int getJobId(){return job_id;}
      time_t getEnterTime(){return enter_time;}
      Command * getCmd(){return cmd;}
      pid_t getJobPid() {return job_pid;}
      void resume(){this->is_stopped=false;}
      void stop(){this->is_stopped=true;}
  };
protected:
    SmashErrors err;
 int max_job_id = 0; //counts the num of jobs. Need for naming the next job
 std::vector<JobEntry *> jobs_vect;
 public:
  JobsList() : jobs_vect(){};
  ~JobsList()=default;
  void addJob(Command* cmd, pid_t job_pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  /*** our own methods ***/
  time_t getEntryTime(int jobId);
  int getCurrJobsNum(){return jobs_vect.size();}
};

class JobsCommand : public BuiltInCommand {
     JobsList * jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList * jobs;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~ForegroundCommand() =default;
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList * jobs;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~BackgroundCommand() = default;
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  FareCommand(const char* cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetCoreCommand : public BuiltInCommand {
  JobsList * jobs;
 public:
  SetCoreCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~SetCoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
  JobsList* jobs;
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs) {};
  virtual ~KillCommand() {}
  void execute() override;
};


class SmallShell {
 private:
    char* p_last_dir;
    JobsList* jobs_list;
    string prompt = "smash> ";
    pid_t * fg_pid;
    Command * fg_cmd;
public:
    SmallShell();
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
      void operator=(SmallShell const&)  = delete; // disable = operator
      static SmallShell& getInstance() // make SmallShell singleton
      {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
      }
      ~SmallShell();
      void executeCommand(const char* new_prompt);
      pid_t * getFgPID(){return this->fg_pid;}
      void setFgPID(pid_t * pid){
          if (pid == nullptr)
              *(this->fg_pid) = -1;
          else
            *(this->fg_pid) = *pid;
      }
      Command * getFgCmd(){return this->fg_cmd;}
      void setFgCmd(Command* cmd){this->fg_cmd = cmd;}
      void ChangePrompt(const string new_prompt);
      string GetPrompt(){return prompt;}
      JobsList * GetJobList(){return jobs_list;}
};


#endif //SMASH_COMMAND_H_
