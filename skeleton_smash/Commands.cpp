#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>


//todo:: Remmember to free Command*. In joblist and in the executeCommand/smallShell
using namespace std;
#define SYS_FAIL -1
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
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


int _parseCommandLine(const char* cmd_line, char** args) { ////!!!!!!! remember to free everytime calling this function
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

void free_args(char **args, int args_size) {
    for (int i = 0; i < args_size; i++) {
        if (!args[i]) {
            free(args[i]);
        }
    }
}

string Command::getCommand() {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    return firstWord;
}

/**
 * This function get a string and return rather the string can be
 * converted to integer.
 * @param str
 * @return bool
 */
bool is_an_integer(string str) {
    for (char const &ch : str)
    {
        if (std::isdigit(ch) == 0)
            return false;
    }
    if (str.length() == 0)
        return false;
    return true;
}

/**
 * This function chekcs rather the prefix string is a prefix of the str string.
 * @param str
 * @param prefix
 * @return bool
 */
bool startsWith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

/**
 * This function check if a signal argument given by the kill command is valid
 * @param signal
 * @return bool
 */
bool is_valid_signal(string signal)

{
    string signal_without_makaf = signal.substr(1, signal.length());
   if (!startsWith(signal, "-") || !is_an_integer(signal_without_makaf) )
       return false;

   int signal_num = stoi(signal_without_makaf);

   if (signal_num >= MAX_SIG || signal_num == 0)
       return false;

   return true;
}



bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
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

/***--------------Build In Commands--------------***/

void ShowPidCommand::execute() {
    pid_t curr_pid = getpid();
    cout << "smash pid is " << curr_pid << std::endl;
}

void GetCurrDirCommand::execute() {
  char curr_path[PATH_MAX];
  if (getcwd(curr_path, (size_t)PATH_MAX) == NULL)
  {
      this->err.PrintSysFailError("getcwd");
      return;
  }
  else{
    cout<<curr_path<<endl;
    return;
  }
}

void ChangeDirCommand::execute() {
    char* args[PATH_MAX];
    string cmd = this->getCommand();
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    if(num_args > 2)
    {
        this->err.PrintTooManyArgs(cmd);
        free_args(args, num_args);
        return;
    }
    if(num_args == 1)
    {
        /// todo: Taking care if only cd was written
        free_args(args, num_args);
        return;
    }
    char*  p_current = new char[PATH_MAX];
    char * makaf = "-";

    if(getcwd(p_current, (size_t)PATH_MAX) == NULL)
    {
        this->err.PrintSysFailError("getcwd");
        return;
    }
    if (strcmp(args[1],makaf) == 0) // second argument is -
    {
        if (p_last_dir == nullptr)
        {
            this->err.PrintOLDPWDFail(cmd);
            free_args(args, num_args);
            return;
        }
        if ((chdir(*p_last_dir)) == SYS_FAIL)
        {
            this->err.PrintSysFailError("chdir");
            free_args(args, num_args);
            return;
        }
        delete *p_last_dir;
        *p_last_dir = p_current;
        free_args(args, num_args);
        return;
    }

    else {
        if((chdir(args[1])) == SYS_FAIL)
        {
            this->err.PrintSysFailError("chdir");
            free_args(args, num_args);
            return;
        }
        if(p_last_dir == nullptr)
        {
            *p_last_dir = p_current;
        }
        else
        {
            delete *p_last_dir;
            *p_last_dir = p_current;
        }
        free_args(args, num_args);
        return;
    }
}

void QuitCommand::execute() {
    return;
}

/***ForegroundCommand implementation
 * this function brings a stopped process or a process that runs
 * in the background to the foreground
 * */

void ForegroundCommand::execute() {
    string cmd = this->getCommand();
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    JobsList::JobEntry *job_to_fg;
    if (num_args == 1) // only fg was written
    {
        job_to_fg = this->jobs->getLastJob(&job_id_num);
        if (job_to_fg == nullptr) {
            this->err.PrintJobsListEmpty(cmd);
            free_args(args, num_args);
            return;
        }
    } else if (num_args == 2) // specific job was entered
    {
        string job_id = args[1];
        if (!is_an_integer(job_id))
        {
            this->err.PrintInvalidArgs(cmd); //todo: if job id isn't an integer should it be jobdoesntexits or syntax err
            free_args(args, num_args);
            return;
        }
        job_id_num = stoi(job_id); // stio convert a string to number
        job_to_fg = this->jobs->getJobById(job_id_num);
        if (job_to_fg == nullptr) {
            this->err.PrintJobIDDoesntExits(cmd, job_id_num);
            free_args(args, num_args);
            return;
        }
    }
    else //more then 2 arguments
    {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }

    pid_t pid = job_to_fg->getJobPid();
    cout << this->getCommandLine() << ": " << pid << endl; //success
    if (kill(pid, SIGCONT) == SYS_FAIL)
    {
        this->err.PrintSysFailError("kill");
        free_args(args, num_args);
        return;
    }
    else
    {
        if(waitpid(pid, nullptr, WCONTINUED) == SYS_FAIL)
        {
            this->err.PrintSysFailError("waitpid");
            free_args(args, num_args);
            return;
        }
    }
    free_args(args, num_args);
}


/***BackgroundCommand implementation
 * this function resumes one of the stopped processes in the background
 * */
void BackgroundCommand::execute() {
    string cmd = this->getCommand();
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    JobsList::JobEntry *job_to_bg;

    if (num_args == 1) // only bg was written
    {
        job_to_bg = this->jobs->getLastStoppedJob(&job_id_num);
        if (job_to_bg == nullptr) {
            this->err.PrintNoStoppedJobs(cmd);
            free_args(args, num_args);
            return;
        }

    }
    else if (num_args == 2) // specific job was entered
    {
        string job_id = args[1];
        if (!is_an_integer(job_id)) {
            this->err.PrintInvalidArgs(cmd);
            free_args(args, num_args);
            return;
        }
        job_id_num = stoi(job_id); // stio convert a string to number
        job_to_bg = this->jobs->getJobById(job_id_num);
        if (job_to_bg == nullptr) {
            this->err.PrintJobIDDoesntExits(cmd, job_id_num);
            free_args(args, num_args);
            return;
        }
        else if (!job_to_bg->isStopped()){
            this->err.PrintJobAlreadyRunningInBG(cmd, job_id_num);
            free_args(args, num_args);
            return;
        }
    }
    else {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }

    pid_t pid = job_to_bg->getJobPid();
    cout << job_to_bg->getCmd()->getCommand() << ' : ' << pid << endl;
    job_to_bg->resume();

    //resume in background
    if (kill(pid, SIGCONT) == SYS_FAIL) {
        free_args(args, num_args);
        return;
    }

    free_args(args, num_args);
}


void KillCommand::execute() {
    string cmd = this->getCommand();
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    int signal_num;
    JobsList::JobEntry *job;
    if(num_args != 3){
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }
    string signal_str = args[1];
    string sig_without_makaf = signal_str.substr(1, signal_str.length()); // removing the '-' from the beginning
    if(is_valid_signal(signal_str)) //check if the command second args startswith -
    {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }
    signal_num = stoi(signal_str);
    string job_id = args[2];
    if (!is_an_integer(job_id))
    {
        this->err.PrintInvalidArgs(cmd); //todo: if job id isn't an integer should it be jobdoesntexits or syntax err
        free_args(args, num_args);
        return;
    }
    job_id_num = stoi(job_id); // stio convert a string to number
    job = jobs->getJobById(job_id_num);
    if (job == nullptr)
    {
        this->err.PrintJobIDDoesntExits(cmd, job_id_num);
        free_args(args, num_args);
        return;
    }
    // in here we found that the signum num the job id are valid
    if (kill(job->getJobPid(), signal_num) == SYS_FAIL)
    {
        this->err.PrintSysFailError("kill");
        free_args(args,num_args);
        return;
    }

    switch(signal_num){
        case SIGCONT:
            jobs->removeJobById(job_id_num);
            break;
        case SIGINT:
            jobs->removeJobById(job_id_num);
            break;
        case SIGQUIT:

        case SIGABRT:
            ///whattttt tooo dooooo???
            break;
        case SIGKILL:
            jobs->removeJobById(job_id_num);
            break;
        case SIGSTOP:
            job->stop();
            break;
        default:
            break;
            //todo: add more cases and think how to act for each sig
    }





}

/***--------------JobList implementation--------------***/

/***addJob- this function inserts process to the JobList due to:
 * Run in the Background &
 * Stopped Process
 */
void JobsList::addJob(Command *cmd, pid_t job_pid, bool isStopped) {
    removeFinishedJobs();
    this->max_job_id++;
    JobEntry *new_job = new JobEntry(this->max_job_id, job_pid, isStopped, cmd);
    this->jobs_vect.push_back(new_job);
}


/***printJobsList- this function prints all the jobs that currently in JobList */
void JobsList::printJobsList() {
    removeFinishedJobs();

    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); ++it) {
        time_t *curr;
        time(curr);
        time_t diff = difftime((*it)->getEnterTime(), *curr);
        if ((*it)->isStopped()) {
            cout << (*it)->getJobId() << ' ' << (*it)->getCmd()->getCommandLine() << " : " << (*it)->getJobPid()
                 << ' ' << diff << ' ' << "(stopped)" << endl;
        } else {
            cout << (*it)->getJobId() << ' ' << (*it)->getCmd()->getCommandLine() << " : " << (*it)->getJobPid()
                 << ' ' << diff << ' ' << endl;
        }
    }
}

/***killAllJobs- this function kills all the jobs that currently in JobList */
void JobsList::killAllJobs() {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); ++it) {
        cout << "smash: process " << (*it)->getJobPid() << "was killed" << endl;
        kill((*it)->getJobPid(), SIGKILL);
        jobs_vect.erase(it);
        it--;
    }
}

/***removeFinishedJobs- this function removes all the finished jobs from JobList */
void JobsList::removeFinishedJobs() {
    if (jobs_vect.empty()) {
        return;
    }
    pid_t job_p;
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        job_p = (*it)->getJobPid();
        pid_t return_pid = waitpid(job_p, nullptr, WNOHANG);
        if (return_pid == SYS_FAIL) return; //TODO: add error handler?
        else if (return_pid == job_p) {
            jobs_vect.erase(it);
            it--;
        }
    }
    //change max_job_id
    if (job_p == max_job_id) {
        int max_j = 0;
        for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
            if (max_j < (*it)->getJobId()) {
                max_j = (*it)->getJobId();
            }
        }
        max_job_id = max_j;
    }
}

/***getJobById- this function returns pointer to JobEntry that matches to jobId */
JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId() == jobId) {
            return *it;
        }
    }
    return nullptr;
}

/***removeJobById- this function removes job from JobList that matches to jobId */
void JobsList::removeJobById(int jobId) {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId() == jobId) {
            jobs_vect.erase(it);
            return;
        }
    }
}

/***getLastJob- this function returns the most recent (and not finished) job that was inserted to JobList. */
JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    for (auto it = jobs_vect.rbegin(); it != jobs_vect.rend(); it++) {
        pid_t job_p = (*it)->getJobPid();
        pid_t return_pid = waitpid(job_p, nullptr, WNOHANG);

        if (return_pid == SYS_FAIL) return nullptr; //TODO: add error handler?
        else if (!(job_p == return_pid)) {
            *lastJobId = (*it)->getJobId();
            return *it;
        }
    }
    *lastJobId = -1;
    return nullptr;
}

/***getLastStoppedJob- this function returns the most recent stopped job that was inserted to JobList. */
JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    JobEntry *last_stopped = nullptr;
//find last stopped and not finished job (reversed iterator)
    for (auto it = jobs_vect.rbegin(); it != jobs_vect.rend(); it++) {
        pid_t job_p = (*it)->getJobPid();
        pid_t return_pid = waitpid(job_p, nullptr, WNOHANG);

        if (return_pid == SYS_FAIL) {//TODO: add error handler?
            *jobId = -1;
            return nullptr;
        } else if ((*it)->isStopped() && job_p != return_pid) {
            *jobId = (*it)->getJobId();
            last_stopped = *it;
            break;
        }
    }
    *jobId = -1;
    return last_stopped;
}

/***getEntryTime- this function returns returns entry_time of jobID*/
time_t JobsList::getEntryTime(int jobId) {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId() == jobId) {
            return (*it)->getEnterTime();
        }
    }
    return 0;
}

/***getEntryTime- this function prints JobList */
void JobsCommand::execute() {
    this->jobs->printJobsList();
}

/***--------------External Command implementation--------------***/


void ComplexExternalCommand::execute() {
    char* cmd_line = this->getCommandLineNoneConst();
    char * file_path = "/bin/bash";
    char * flag = "-c";
    char * array_of_arg[] = {file_path, flag, cmd_line, nullptr};
    pid_t pid = fork();
    pid_t child_pid;
    if (pid == SYS_FAIL){
        this->err.PrintSysFailError("fork");
        free(cmd_line);
        return;
    }
    if (pid == 0){
        if (setpgrp() == SYS_FAIL){
            this->err.PrintSysFailError("setpgrp");
            free(cmd_line);
            return;
        }
        if(execv(file_path, array_of_arg) == SYS_FAIL){
            this->err.PrintSysFailError("execv");
            free(cmd_line);
            return;
        }
    else{ // pid != 0 Parent code
        free(cmd_line);
        child_pid = pid;
        if(!this->is_background){
            if (waitpid(child_pid, nullptr, WUNTRACED) == SYS_FAIL) {
                this->err.PrintSysFailError("waitpid");
                return;
            }
        else{ //in background
            this->jobs->addJob(this, child_pid, false);
    }


    }
}



/***--------------Pipe Command implementation--------------***/



/***--------------Redirection Command implementation--------------***/



/***--------------SmallShell implementation--------------***/

SmallShell::SmallShell() {
//    this->jobs_list = new JobsList();
    this->p_last_dir = nullptr;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (firstWord.back() == '&') {
        firstWord.pop_back();
    }

  if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, &p_last_dir);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("bg") == 0) {
    //return new BackgroundCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("quit") == 0) {
    //return new QuitCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("fare") == 0) {
    //return new FareCommand(cmd_line);
  }

  else {
      // In case the command is not a built in command
    return new ExternalCommand(cmd_line);
  }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    Command *cmd = CreateCommand(cmd_line);
    if (cmd == nullptr) { //if unrecognize command was entered
        return;
    }
    cmd->execute();
    delete cmd;
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

// ComplextExternalCommand(cmd_line, is_background, args*)
//rm *.txt       execv("bin/bash", args)
// rm file1.txt  execv(file1.txt,