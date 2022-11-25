#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <climits>
#include "Commands.h"

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

// TODO: Add your implementation for classes in Commands.h
void ShowPidCommand::execute() {
  pid_t curr_pid = getpid();
  cout<<"smash pid is "<<curr_pid << std::endl;
}

void GetCurrDirCommand::execute() {
  char curr_path[PATH_MAX];
  if (getcwd(curr_path, PATH_MAX)==NULL){
    return;
  }
  else{
    cout<<curr_path<<endl;
  }
}

void ChangeDirCommand::execute() {
    char * args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);

    char * p_current;
    getcwd(p_current, PATH_MAX);

    if (strcmp(args[1],"-")==0){
        chdir(*p_last_dir);
        *p_last_dir = p_current;
        return;
    }
    else {
        chdir(args[1]);
    }
}

void JobsCommand::execute() {
    this->jobs->printJobsList();
}

void ForegroundCommand::execute()
{
    char * args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    JobsList::JobEntry* job_to_fg;
    if (num_args == 1) // only fg was written
    {
        job_to_fg = this->jobs->getLastJob(&job_id_num);
        if (job_to_fg == nullptr)
        {
            //send an error that the jobs list is empty
        }
    }
    else if (num_args == 2) // specific job was entered
    {
        string job_id = args[2];
        if (is_an_integer(job_id) == false)
        {
            //error that job id given is not a number
        }
            job_id_num = stoi(job_id); // stio convert a string to number
            job_to_fg = this->jobs->getJobById(job_id_num);
            if (job_to_fg == nullptr){
                //need to send an error that the job id doesn't exist
            }
    }
    else
    {
        // error invailid number of arguments were passed
        //remmember to exit after an error
    }

    pid_t pid = job_to_fg->getJobPid();
    if (kill(pid, SIGCONT) == SYS_FAIL)
    {
        //error: couldn't send sigcont to the pid
    }
    else
    {
        if(waitpid(pid, NULL, WCONTINUED) == SYS_FAIL)
        {
            //error: wait pid failed
        }
    }
    cout << this->getCommandLine()<<": "<< pid; //success
}

void QuitCommand::execute() {
    return;
}


void JobsList::addJob(Command* cmd, bool isStopped){
    this->max_job_id++;
    JobEntry * new_job = new JobEntry(this->max_job_id, isStopped, cmd);
    this->jobs_vect.push_back(new_job);
    if(isStopped){
        this->last_stopped_job = new_job;
    }
}

/**Remember: calc curr time every single time*/
void JobsList::printJobsList(){
    for (auto it = jobs_vect.begin();  it!= jobs_vect.end(); it++) {
        time_t * curr;
        time(curr);
        time_t diff = difftime((*it)->getEnterTime(),*curr);
        if((*it)->isStopped()){
//            cout<<(*it)->getJobId()<<' '<<(*it)->getCmd()->getCommandLine()<<" : "<<(*it)->getCmd()->getCmdPid()
//            <<' '<< diff <<' '<< "(stopped)"<<endl;
        }
        else{
//            cout<<(*it)->getJobId()<<' '<<(*it)->getCmd()->getCommandLine()<<" : "<<(*it)->getCmd()->getCmdPid()
//                <<' '<< diff <<' '<<endl;
        }
    }
}

void JobsList::killAllJobs(){
    for (auto it = jobs_vect.begin();  it!= jobs_vect.end(); it++){
        
    }
}

void JobsList::removeFinishedJobs(){

}

JobsList::JobEntry * JobsList::getJobById(int jobId){

}

void JobsList::removeJobById(int jobId){

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
  if (firstWord.back() == '&'){
      firstWord.pop_back();
  }
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
//  else if (firstWord.compare("bg") == 0) {
//    return new BackgroundCommand(cmd_line, jobs_list);
//  }
//  else if (firstWord.compare("quit") == 0) {
//    return new QuitCommand(cmd_line, jobs_list);
//  }
//  else if (firstWord.compare("kill") == 0) {
//    return new KillCommand(cmd_line, jobs_list);
//  }
//  else if (firstWord.compare("fare") == 0) {
//    return new FareCommand(cmd_line);
//  }

//  else {
//    return new ExternalCommand(cmd_line);
//  }

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