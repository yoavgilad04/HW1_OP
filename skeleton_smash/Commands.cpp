#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>

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

void free_args(char** args, int args_size)
{
    for (int i=0; i<args_size; i++)
    {
        if(!args[i])
        {
            free(args[i]);
        }
    }
}

string Command::getCommand(){
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    return firstWord;
}

bool is_an_integer(string str)
{
    for (char const &ch : str)
    {
        if (std::isdigit(ch) == 0)
            return false;
    }
    return true;
}

bool startsWith(const std::string &str, const std::string &prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
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

/***--------------Build In Commands--------------***/

void ShowPidCommand::execute() {
  pid_t curr_pid = getpid();
  cout<<"smash pid is "<<curr_pid << std::endl;
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

/***ForegroundCommand implementation*/

void ForegroundCommand::execute()
{
    string cmd = this->getCommand();
    char * args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    JobsList::JobEntry* job_to_fg;
    if (num_args == 1) // only fg was written
    {
        job_to_fg = this->jobs->getLastJob(&job_id_num);
        if (job_to_fg == nullptr)
        {
            this->err.PrintJobsListEmpty(cmd);
            free_args(args, num_args);
            return;
        }
    }
    else if (num_args == 2) // specific job was entered
    {
        string job_id = args[1];
        if (!is_an_integer(job_id))
        {
            this->err.PrintInvalidArgs(cmd);
            free_args(args, num_args);
            return;
        }
        job_id_num = stoi(job_id); // stio convert a string to number
        job_to_fg = this->jobs->getJobById(job_id_num);
        if (job_to_fg == nullptr)
        {
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
    if (kill(pid, SIGCONT) == SYS_FAIL)
    {
        this->err.PrintSysFailError("kill");
        free_args(args, num_args);
        return;
    }
    else
    {
        if(waitpid(pid, NULL, WCONTINUED) == SYS_FAIL)
        {
            this->err.PrintSysFailError("waitpid");
            free_args(args, num_args);
            return;
        }
    }
    free_args(args, num_args);
    cout << this->getCommandLine()<<": "<< pid<< endl; //success
}


/***BackgroundCommand implementation*/


/***--------------JobList implementation--------------***/

/***addJob- this function inserts process to the JobList due to:
 * Run in the Background (bg command or &)
 * Stopped Process
 */
void JobsList::addJob(Command* cmd, pid_t job_pid, bool isStopped) {
    removeFinishedJobs();
    this->max_job_id++;
    JobEntry *new_job = new JobEntry(this->max_job_id, job_pid, isStopped, cmd);
    this->jobs_vect.push_back(new_job);
}


/***printJobsList- this function prints all the jobs that currently in JobList */
void JobsList::printJobsList(){
    removeFinishedJobs();

    for (auto it = jobs_vect.begin();  it!= jobs_vect.end(); ++it) {
        time_t * curr;
        time(curr);
        time_t diff = difftime((*it)->getEnterTime(),*curr);
        if((*it)->isStopped()){
            cout<<(*it)->getJobId()<<' '<<(*it)->getCmd()->getCommandLine()<<" : "<<(*it)->getJobPid()
            <<' '<< diff <<' '<< "(stopped)"<<endl;
        }
        else{
            cout<<(*it)->getJobId()<<' '<<(*it)->getCmd()->getCommandLine()<<" : "<<(*it)->getJobPid()
                <<' '<< diff <<' '<<endl;
        }
    }
}
/***killAllJobs- this function kills all the jobs that currently in JobList */
void JobsList::killAllJobs(){
    for (auto it = jobs_vect.begin(); it!=jobs_vect.end() ; ++it) {
        kill((*it)->getJobPid(), SIGKILL);
        jobs_vect.erase(it);
    }
}

/***removeFinishedJobs- this function removes all the finished jobs from JobList */
void JobsList::removeFinishedJobs() {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if((*it)->isFinished()){
            jobs_vect.erase(it);
        }
    }
}

/***getJobById- this function returns pointer to JobEntry that matches to jobId */
JobsList::JobEntry * JobsList::getJobById(int jobId) {
    JobEntry * job= nullptr;
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId()==jobId){
            job = *it;
        }
    }
    return job;
}

/***removeJobById- this function removes job from JobList that matches to jobId */
void JobsList::removeJobById(int jobId){
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId()==jobId){
                jobs_vect.erase(it);
                break;
        }
    }
}

/***getLastJob- this function returns the most recent (and not finished) job that was inserted to JobList. */
JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){
    for (auto it = jobs_vect.rbegin(); it != jobs_vect.rend(); it++) {
        if (!(*it)->isFinished()){
            return *it;
        }
    }
    return nullptr;
}

/***getLastStoppedJob- this function returns the most recent stopped job that was inserted to JobList. */
JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId){
    JobEntry * last_stopped = nullptr;
//find last stopped and not finished job (reversed iterator)
    for (auto it = jobs_vect.rbegin(); it != jobs_vect.rend(); it++) {
        if ((*it)->isStopped() && !(*it)->isFinished()){
            last_stopped = *it;
            break;
        }
    }
    return last_stopped;
}

/***getEntryTime- this function returns returns entry_time of jobID*/
time_t JobsList::getEntryTime(int jobId) {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId()==jobId){
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
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  if (firstWord.back() == '&'){
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
    //return new KillCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("fare") == 0) {
    //return new FareCommand(cmd_line);
  }

  else {
    //return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
   Command* cmd = CreateCommand(cmd_line);
   if (cmd == nullptr){ //if unrecognize command was entered
       return;
   }
   cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}