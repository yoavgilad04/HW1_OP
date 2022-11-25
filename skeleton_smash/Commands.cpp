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
void ShowPidCommand::execute() {
  pid_t curr_pid = getpid();
  cout<<"smash pid is "<<curr_pid << std::endl;
}

void GetCurrDirCommand::execute() {
  char * curr_path= new char(PATH_MAX);
  if (getcwd(curr_path, (size_t)PATH_MAX)==NULL){
    return;
  }
  else{
    cout<<curr_path<<endl;
  }
}

void ChangeDirCommand::execute() {
    char * args = new char (PATH_MAX);
    int num_args = _parseCommandLine(this->getCommand(), &args);

    char * p_current;
    getcwd(p_current, PATH_MAX);

    if (args[1] == '-'){
        chdir(*p_last_dir);
        *p_last_dir = p_current;
        return;
    }
    else {
        chdir(args+1);
    }
}

void QuitCommand::execute() {
    return;
}


void JobsList::addJob(Command* cmd, bool isStopped){
    removeFinishedJobs();
    this->max_job_id++;
    JobEntry * new_job = new JobEntry(this->max_job_id, isStopped, cmd);
    this->jobs_vect.push_back(new_job);
    if(isStopped){
        this->last_stopped_job = new_job;
    }
}

void JobsList::printJobsList(){
    removeFinishedJobs();

    for (auto it = jobs_vect.begin();  it!= jobs_vect.end(); it++) {
        time_t * curr;
        time(curr);
        time_t diff = difftime((*it)->getEnterTime(),*curr);
        if((*it)->isStopped()){
            cout<<(*it)->getJobId()<<' '<<(*it)->getCmd()->getCommand()<<" : "<<(*it)->getCmd()->getCmdPid()
            <<' '<< diff <<' '<< "(stopped)"<<endl;
        }
        else{
            cout<<(*it)->getJobId()<<' '<<(*it)->getCmd()->getCommand()<<" : "<<(*it)->getCmd()->getCmdPid()
                <<' '<< diff <<' '<<endl;
        }
    }
}

void JobsList::killAllJobs(){
    jobs_vect.clear();
    finished_jobs.clear();
}

void JobsList::removeFinishedJobs() {
    for (auto it = finished_jobs.rbegin(); it!= finished_jobs.rend(); it++) {
        std::swap(**it,jobs_vect.back());
        jobs_vect.pop_back();
        finished_jobs.pop_back();
    }
}
JobsList::JobEntry * JobsList::getJobById(int jobId) {
    JobEntry * job= nullptr;
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId()==jobId){
            job = *it;
        }
    }
    return job;
}
void JobsList::removeJobById(int jobId){
    JobEntry ** p_job= nullptr;
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if ((*it)->getJobId()==jobId){
            p_job = static_cast<JobEntry*>(it);
        }
    }
    if(p_job){
        finished_jobs.push_back(p_job);
    }
}

JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){


}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId){

}


SmallShell::SmallShell() {
// TODO: add your implementation
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
//chprompt, showpid, pwd, cd, jobs, fg, bg, quit, kill


  if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, p_last_dir);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("bg") == 0) {
    return new BackgroundCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line, jobs_list);
  }
  else if (firstWord.compare("fare") == 0) {
    return new FareCommand(cmd_line);
  }

  else {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
   Command* cmd = CreateCommand(cmd_line);
   cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}