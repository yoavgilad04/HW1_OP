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
#define COMPLEX_CHAR "*?"

#include <fcntl.h>

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

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}


int _parseCommandLine(const char *cmd_line, char **args) { ////!!!!!!! remember to free everytime calling this function
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
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

char **Command::setUpArgs(char ***args, const char *cmd_line, string *cmd, int *args_num) {
    *cmd = this->getCommand();
    if (args_num == nullptr)
        return nullptr;
    *args[COMMAND_ARGS_MAX_LENGTH]; //char**
    *args_num = _parseCommandLine(this->getCommandLine(), *args);
    return nullptr;
}
/*string Command::getCommand() {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    return firstWord;
}*/

/**
 * This function get a string and return rather the string can be
 * converted to integer.
 * @param str
 * @return bool
 */
bool is_an_integer(string str) {
    char minus = '-';
    int counter=0;
    for (char const &ch : str) {
        char current_char = ch;
        if (std::isdigit(current_char) == 0 && current_char != minus)
            return false;
        if (current_char == minus){
            counter++;
        }
        if(counter > 1)
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
bool is_valid_signal(string signal) {
    string signal_without_makaf = signal.substr(1, signal.length());
    if (!startsWith(signal, "-") || !is_an_integer(signal_without_makaf))
        return false;

    int signal_num = stoi(signal_without_makaf);

    if (signal_num >= MAX_SIG || signal_num == 0)
        return false;

    return true;
}

//todo: entering all utility functions to classes
bool is_valid_core(string core_str) {
    if (!is_an_integer(core_str))
        return false;

    int number_of_cores = sysconf(_SC_NPROCESSORS_CONF);
    if (number_of_cores == SYS_FAIL) {
        perror(" ");
    }
    int core_num = stoi(core_str);

    if (!core_num <= number_of_cores) //core given doesnt exists in computer
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

string Command::getCommand() {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (firstWord.back() == '&') {
        firstWord.pop_back();
    }
    return firstWord;
}

/***--------------Build In Commands--------------***/


void Chprompt::execute() { //TODO: ADD ERRORS
    SmallShell &shell = SmallShell::getInstance();
    char *args[PATH_MAX];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    if (num_args == 1) {
        shell.ChangePrompt("");
    } else {
        shell.ChangePrompt(args[1]);
    }
}


void ShowPidCommand::execute() {
    pid_t curr_pid = getpid();
    cout << "smash pid is " << curr_pid << std::endl;
}

void GetCurrDirCommand::execute() {
    char curr_path[PATH_MAX];
    if (getcwd(curr_path, (size_t) PATH_MAX) == NULL) {
        this->err.PrintSysFailError("getcwd");
        return;
    } else {
        cout << curr_path << endl;
        return;
    }
}

void ChangeDirCommand::execute() {
    char *args[PATH_MAX];
    string cmd = this->getCommand();
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    if (num_args > 2) {
        this->err.PrintTooManyArgs(cmd);
        free_args(args, num_args);
        return;
    }
    if (num_args == 1) {
        /// todo: Taking care if only cd was written
        free_args(args, num_args);
        return;
    }
    char *p_current = new char[PATH_MAX];
    char *makaf = "-";

    if (getcwd(p_current, (size_t) PATH_MAX) == NULL) {
        this->err.PrintSysFailError("getcwd");
        return;
    }
    if (strcmp(args[1], makaf) == 0) // second argument is -
    {
        if (*p_last_dir == NULL) {
            this->err.PrintOLDPWDFail(cmd);
            free_args(args, num_args);
            return;
        }
        if ((chdir(*p_last_dir)) == SYS_FAIL) {
            this->err.PrintSysFailError("chdir");
            free_args(args, num_args);
            return;
        }
        delete *p_last_dir;
        *p_last_dir = p_current;
        free_args(args, num_args);
        return;
    } else {
        if ((chdir(args[1])) == SYS_FAIL) {
            this->err.PrintSysFailError("chdir");
            free_args(args, num_args);
            return;
        }
        if (p_last_dir == nullptr) {
            *p_last_dir = p_current;
        } else {
            delete *p_last_dir;
            *p_last_dir = p_current;
        }
        free_args(args, num_args);
        return;
    }
}

void QuitCommand::execute() {
    const char *cmd_c = this->getCommandLine();
    string cmd_s = _trim(string(cmd_c));
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num = _parseCommandLine(this->getCommandLine(), args);
    bool is_loud = false;
    if (num > 1) {
        if (strcmp((args[1]), "kill") == 0) {
            cout << "smash: sending SIGKILL signal to " << jobs->getCurrJobsNum() << " jobs:" << endl;
            jobs->killAllJobs();
        }
    }
    //jobs->killAllJobs(is_loud);
    delete this;
    exit(0);
}

/***ForegroundCommand implementation
 * this command brings a stopped process or a process that runs
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
        if (!is_an_integer(job_id)) {
            this->err.PrintInvalidArgs(cmd);
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
    } else //more then 2 arguments
    {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }
    SmallShell& shell = SmallShell::getInstance();
    pid_t pid = job_to_fg->getJobPid();
    cout << job_to_fg->getCmd()->getCommandLine() << " : " << pid << endl; //success
    if (kill(pid, SIGCONT) == SYS_FAIL) {
        this->err.PrintSysFailError("kill");
        free_args(args, num_args);
        return;
    } else {
        shell.setFgPID(pid);
        shell.setFgCmd(job_to_fg->getCmd());
        shell.setFgJobID(job_id_num);
        if (waitpid(pid, nullptr, WUNTRACED) == SYS_FAIL) {
            this->err.PrintSysFailError("waitpid");
            free_args(args, num_args);
            return;
        }
        jobs->removeJobById(job_id_num);
        shell.setFgPID(-1);
        shell.setFgCmd(nullptr);
        shell.setFgJobID(-1);
    }
    free_args(args, num_args);
}


/***BackgroundCommand implementation
 * this command resumes one of the stopped processes in the background
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

    } else if (num_args == 2) // specific job was entered
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
        } else if (!job_to_bg->isStopped()) {
            this->err.PrintJobAlreadyRunningInBG(cmd, job_id_num);
            free_args(args, num_args);
            return;
        }
    } else {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }

    pid_t pid = job_to_bg->getJobPid();
    cout << job_to_bg->getCmd()->getCommandLine() << " : " << pid << endl;

    //resume in background
    if (kill(pid, SIGCONT) == SYS_FAIL) {
        this->err.PrintSysFailError("kill");
        free_args(args, num_args);
        return;
    }
    job_to_bg->resume();
    free_args(args, num_args);
}


void KillCommand::execute() {
    string cmd = this->getCommand();
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    int signal_num;
    JobsList::JobEntry *job;
    if (num_args != 3) {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }
    string signal_str = args[1];
    if (!is_valid_signal(signal_str)) //check if the command second args startswith -
    {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }
    string signal_without_makaf = signal_str.substr(1, signal_str.length());
    signal_num = stoi(signal_without_makaf);
    string job_id = args[2];
    if (!is_an_integer(job_id)) {
        this->err.PrintInvalidArgs(cmd); //todo: if job id isn't an integer should it be jobdoesntexits or syntax err
        free_args(args, num_args);
        return;
    }
    job_id_num = stoi(job_id); // stoi convert a string to number
    job = jobs->getJobById(job_id_num);
    if (job == nullptr) {
        this->err.PrintJobIDDoesntExits(cmd, job_id_num);
        free_args(args, num_args);
        return;
    }
    cout << "signal number " << signal_num << " was sent to pid " << job->getJobPid() << endl;

    // in here we found that the signum num the job id are valid
    if (kill(job->getJobPid(), signal_num) == SYS_FAIL) {
        this->err.PrintSysFailError("kill");
        free_args(args, num_args);
        return;
    }

    switch (signal_num) {
        case SIGCONT:
            job->resume();
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

/***TimeoutCommand implementation
 * this command sends an alarm for 'duration' seconds, runs the command on smash directly,
 * and when timed out sends SIGKILL.
*/
void TimeoutCommand::execute() {
    return;
}


/***FareCommand implementation
 * this command finds and replace every instance of word source to the word dest.
 * prints how many instances have been replaced.
*/
FareCommand::FareCommand(const char* cmd_line): BuiltInCommand(cmd_line){
    //TODO:remove & sign

    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    if (num_args != 4) {
        this->err.PrintInvalidArgs("fare");
        free_args(args, num_args);
        is_success=false;
        return;
    }
    is_success=true;
    filename = (char *)malloc(strlen(args[1]));
    strcpy(this->filename, args[1]);

    source = (char * ) malloc(strlen(args[2]));
    strcpy(this->source, args[2]);
    destination = (char * ) malloc(strlen(args[3]));
    strcpy(this->destination, args[3]);

    free_args(args, num_args);

}


void FareCommand::execute() {
   if(!is_success) return;
    //open file
    fd = open(filename, O_RDONLY);
    if (fd == SYS_FAIL) {
        this->is_redir=false;
        this->err.PrintSysFailError("open");
        cleanup();
        return;
    }

    //calc length and create buffer
    ssize_t length=0;
    char chr;
    while (read(fd, &chr, 1) == 1) {
        length++;
    }
    buff = (char *) malloc(length+1);

    //read into buff
    if(lseek(fd, 0, SEEK_SET)==SYS_FAIL){ //get back to the beginning
        this->err.PrintSysFailError("lseek");
        close(fd);
        cleanup();
        return;
    }
    if (read(fd, buff, length)==SYS_FAIL){
        this->err.PrintSysFailError("read");
        close(fd);
        cleanup();
        return;
    }
    buff[length]='\0';

    close(fd);

    //fare
    int new_len, i, cnt = 0;
    int src_len = strlen(source);
    int dst_len = strlen(destination);

    // Counting the number of times src in buff
    for (i = 0; buff[i] != '\0'; i++) {
        if (strstr(&buff[i], source) == &buff[i]) {
            cnt++;

            // Jumping to index after the old word.
            i += src_len - 1;
        }
    }

    // Making buff_replace of enough length
    new_len= i + cnt * (dst_len-src_len) + 1;
    buff_replace = (char*)malloc(new_len);

    i = 0;
    char * p = buff;
    while (*p) {
        // compare the substring with the result
        if (strstr(p, source) == p) {
            strcpy(&buff_replace[i], destination);
            i += dst_len;
            p += src_len;
        }
        else
            buff_replace[i++] = *p++;
    }
    buff_replace[i] = '\0';


    fd = open(filename, O_WRONLY | O_TRUNC);
    if (fd == SYS_FAIL) {
        this->err.PrintSysFailError("open");
        cleanup();
        return;
    }

    ssize_t write_length = write(fd,buff_replace, new_len);

    //if write < length then restore file
    if (write_length < new_len){
        if(close(fd)==SYS_FAIL) {
            this->err.PrintSysFailError("close");
        }
        if(open(filename, O_WRONLY | O_TRUNC)==SYS_FAIL){
            this->err.PrintSysFailError("open");
        }
        if (write(fd, buff, length-1)==SYS_FAIL){
            this->err.PrintSysFailError("write");
        }
    }
    else{
        cout<<"replaced "<<cnt<<" instances of the string \""<<source<<"\""<<endl;
    }
    close(fd);
    cleanup();
}

void FareCommand::cleanup(){
    if(is_redir){
        //free buffs
        free (buff);
        free (buff_replace);
    }
    free(source);
    free(destination);
    free(filename);

}

/***SetcoreCommand implementation
 * this command sets the core that the job with job id run on.
*/
void SetCoreCommand::execute() {
    string cmd = this->getCommand();
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(this->getCommandLine(), args);
    int job_id_num;
    int core_num;
    JobsList::JobEntry *job;
    if (num_args != 3) {
        this->err.PrintInvalidArgs(cmd);
        free_args(args, num_args);
        return;
    }
    string core_str = args[2];
    if (is_valid_core(core_str)) //check if the core given is valid
    {
        this->err.PrintInvalidCore(cmd);
        free_args(args, num_args);
        return;
    }
    core_num = stoi(core_str);
    string job_id = args[1];
    if (!is_an_integer(job_id)) {
        this->err.PrintInvalidArgs(cmd); //todo: if job id isn't an integer should it be jobdoesntexits or invalidargs
        free_args(args, num_args);
        return;
    }
    job_id_num = stoi(job_id); // stio convert a string to number
    job = jobs->getJobById(job_id_num);
    if (job == nullptr) {
        this->err.PrintJobIDDoesntExits(cmd, job_id_num);
        free_args(args, num_args);
        return;
    }
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(core_num, &my_set);
    pid_t job_pid = job->getJobPid();
    if(sched_setaffinity(job_pid, sizeof(cpu_set_t), &my_set) == SYS_FAIL){
        this->err.PrintSysFailError("sched_setaffinity");
        free_args(args, num_args);
        return;
    }
    free_args(args, num_args);
}



/***--------------JobList implementation--------------***/

/***addJob- this function inserts process to the JobList due to:
 * Run in the Background &
 * Stopped Process
 */
void JobsList::addJob(Command *cmd, pid_t job_pid, bool isStopped, int job_id) {
    removeFinishedJobs();
    if (job_id == -1 || job_id > this->max_job_id)
    {
        this->max_job_id++;
        JobEntry *new_job = new JobEntry(this->max_job_id, job_pid, isStopped, cmd);
        this->jobs_vect.push_back(new_job);
    }
    else{
        JobEntry *new_job = new JobEntry(job_id, job_pid, isStopped, cmd);
        int i = 0;
        for (auto  it : this->jobs_vect) {
            if ((*it).getJobId() > job_id)
            {
                this->jobs_vect.insert(this->jobs_vect.begin() + i, new_job);
                break;
            }
            i ++;
        }
    }

}


/***printJobsList- this function prints all the jobs that currently in JobList */
void JobsList::printJobsList() {
    if (this->max_job_id == 0) return;
    removeFinishedJobs();

    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); ++it) {
        time_t curr = time(NULL);
        if (curr == (time_t) -1) {
            this->err.PrintSysFailError("time");
            return;
        }

        time_t diff = difftime(curr, (*it)->getEnterTime());
        if ((*it)->isStopped()) {
            cout << '[' << (*it)->getJobId() << "] " << (*it)->getCmd()->getCommandLine() << " : " << (*it)->getJobPid()
                 << ' ' << diff << " secs " << "(stopped)" << endl;
        } else {
            cout << '[' << (*it)->getJobId() << "] " << (*it)->getCmd()->getCommandLine() << " : " << (*it)->getJobPid()
                 << ' ' << diff << " secs" << endl;
        }
    }
}

/***killAllJobs- this function kills all the jobs that currently in JobList */
void JobsList::killAllJobs() {
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); ++it) {
        cout << (*it)->getJobPid() << ": " << (*it)->getCmd()->getCommandLine() << endl;

        //cout << "smash: process " << (*it)->getJobPid() << " was killed" << endl;
        kill((*it)->getJobPid(), SIGKILL);
    }
}

/***removeFinishedJobs- this function removes all the finished jobs from JobList */
void JobsList::removeFinishedJobs() {
    if (jobs_vect.empty()) {
        max_job_id = 0;
        return;
    }
    pid_t job_p;
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        job_p = (*it)->getJobPid();
        pid_t return_pid = waitpid(job_p, nullptr, WNOHANG);
        if (return_pid == SYS_FAIL) return;
        else if (return_pid == job_p) {
            jobs_vect.erase(it);
            it--;
        }
    }
    //change max_job_id
    max_job_id = 0;
    for (auto it = jobs_vect.begin(); it != jobs_vect.end(); it++) {
        if (max_job_id < (*it)->getJobId()) {
            max_job_id = (*it)->getJobId();
        }
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
    if(jobs_vect.empty()) {
        *lastJobId=-1;
        return nullptr;
    }

    if(lastJobId) {
        *lastJobId = jobs_vect.back()->getJobId();
    }

    return jobs_vect.back();
}

/***getLastStoppedJob- this function returns the most recent stopped job that was inserted to JobList. */
JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    JobEntry *last_stopped = nullptr;
    *jobId = -1;

    for (JobEntry *it : jobs_vect) {
        if (it->isStopped()) {
            last_stopped = it;
            *jobId = (*it).getJobId();
        }
    }
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

/*** ExternalCommand implementation
 * this command provides running External commands that are not part of Build-in or Special commands.
 * Differs between SimpleExternalCommand and ComplexExternalCommand.
 */
void ExternalCommand::execute() {
    bool is_background = _isBackgroundComamnd(this->getCommandLine());
/***PARSER:*/
    //remove &
    string command_trim = _trim(this->getCommandLine());
    char command_copy[COMMAND_ARGS_MAX_LENGTH];
    strcpy(command_copy, command_trim.c_str());
    if (is_background) {
        _removeBackgroundSign(command_copy);
    }

    //parse to args without &
    char *args[COMMAND_ARGS_MAX_LENGTH];
    int num_args = _parseCommandLine(command_copy, args);
    args[num_args] = 0;

    string cmd_s = _trim(string(this->getCommandLine()));

    SmallShell &shell = SmallShell::getInstance();

    if (cmd_s.find_first_of(COMPLEX_CHAR) != std::string::npos) {
        //char *cmd_line = this->getCommandLineNoneConst();
        char *file_path = "/bin/bash";
        char *flag = "-c";
        char *array_of_arg[] = {file_path, flag, command_copy, nullptr};

        pid_t pid = fork();
        pid_t child_pid;
        if (pid == SYS_FAIL) {
            this->err.PrintSysFailError("fork");
            return;
        }
        if (pid == 0) {
            if (setpgrp() == SYS_FAIL) {
                this->err.PrintSysFailError("setpgrp");
                return;
            }
            if (execv(file_path, array_of_arg) == SYS_FAIL) {
                this->err.PrintSysFailError("execv");
                return;
            } else { // pid != 0 Parent code
                child_pid = pid;
                if (is_background) {
                    shell.GetJobList()->addJob(this, child_pid, false);
                }
                else{
                    shell.setFgPID(child_pid);
                    shell.setFgCmd(this);
                    if (waitpid(child_pid, nullptr, WUNTRACED) == SYS_FAIL) {
                        this->err.PrintSysFailError("waitpid");
                        return;
                    }
                    shell.setFgPID(-1);
                    shell.setFgCmd(nullptr);
                    shell.setFgJobID(-1);

                }
            }
        }

    } else {
        pid_t pid = fork();
        if (pid == SYS_FAIL) {
            perror("fork");
            return;
        }
        if (pid == 0) { //son
            if (setpgrp() == SYS_FAIL) {
                this->err.PrintSysFailError("setpgrp");
                return;
            }
            if (execvp(args[0], args) == SYS_FAIL) {
                this->err.PrintSysFailError("execvp");
                return;
            }
        } else { //father
            if (is_background) {
                shell.GetJobList()->addJob(this, pid, false);
            } else {
                shell.setFgPID(pid);
                shell.setFgCmd(this);
                if (waitpid(pid, nullptr, WUNTRACED) == SYS_FAIL) {
                    this->err.PrintSysFailError("waitpid");
                }
                shell.setFgCmd(nullptr);
                shell.setFgPID(-1);
                shell.setFgJobID(-1);
            }
        }
    }
}


void PipeCommand::closePipe(int *fd) {
    if (close(fd[0]) == SYS_FAIL || close(fd[1]) == SYS_FAIL) {
        this->err.PrintSysFailError("close");
    }
}

/***--------------Pipe Command implementation--------------***/

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
    //is_append
    string cmd_s = _trim(string(cmd_line));
    if (cmd_s.find("|&") != std::string::npos) {
        this->is_error = true;
    }

    string delimiter;
    if (is_error) delimiter = "|&";
    else delimiter = '|';

    size_t pos = 0;
    std::string cmd1 = cmd_s.substr(0, cmd_s.find(delimiter));
    cmd_s.erase(0, pos + cmd1.length() + delimiter.length());
    std::string cmd2 = cmd_s.substr(0, cmd_s.find(delimiter));
    cmd2 = _trim(cmd2);

    cmd1 = _trim(cmd1);
    strcpy(command1, cmd1.c_str());
    strcpy(command2, cmd2.c_str());

    //TODO: add function to extract cmd1,cmd2

}

void PipeCommand::execute() {
    SmallShell &shell = SmallShell::getInstance();
    int fd[2];
    pipe(fd);
    pid_t c1_pid = fork();
    if (c1_pid == SYS_FAIL) {
        this->err.PrintSysFailError("fork");
        this->closePipe(fd);
        return;
    }
    if (c1_pid == 0) {
        if (setpgrp() == SYS_FAIL) {
            this->err.PrintSysFailError("setpgrp");
            this->closePipe(fd);
            return;
        }
        int d_res;
        if (is_error)
            d_res = dup2(fd[1], 2);
        else
            d_res = dup2(fd[1], 1);
        if (d_res == SYS_FAIL) {
            this->err.PrintSysFailError("dup2");
            this->closePipe(fd);
            return;
        }
        this->closePipe(fd);
        shell.executeCommand(this->command1);
        exit(0);
    }
    pid_t c2_pid = fork();
    if (c2_pid == SYS_FAIL) {
        this->err.PrintSysFailError("fork");
        this->closePipe(fd);
        return;
    }

    if (c2_pid == 0) {
        if (setpgrp() == SYS_FAIL) {
            this->err.PrintSysFailError("setpgrp");
            this->closePipe(fd);
            return;
        }
        int d_res = dup2(fd[0], 0);
        if (d_res == SYS_FAIL) {
            this->err.PrintSysFailError("dup2");
            this->closePipe(fd);
            return;
        }
        this->closePipe(fd);
        shell.executeCommand(this->command2);
        exit(0);
    }
    this->closePipe(fd);
    if (waitpid(c1_pid, nullptr, WUNTRACED) == SYS_FAIL) {
        this->err.PrintSysFailError("waitpid");
        return;
    }
    if (waitpid(c2_pid, nullptr, WUNTRACED) == SYS_FAIL) {
        this->err.PrintSysFailError("waitpid");
        return;
    }
}

/***--------------Redirection Command implementation--------------***/
RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {
    //is_append
    string cmd_s = _trim(string(cmd_line));
    if (cmd_s.find(">>") != std::string::npos) {
        this->is_append = true;
    }


    //filename
    string delimiter;
    if (is_append) delimiter = ">>";
    else delimiter = '>';

    size_t pos = 0;
    std::string cmd = cmd_s.substr(0, cmd_s.find(delimiter));
    cmd_s.erase(0, pos + cmd.length() + delimiter.length());
    std::string file_name = cmd_s.substr(0, cmd_s.find(delimiter));
    file_name = _trim(file_name);


    string c = _trim(cmd);
    strcpy(command, c.c_str());
    strcpy(filename, file_name.c_str());

}


void RedirectionCommand::prepare() {}


void RedirectionCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();

    pid_t pid = fork();
    if (pid == SYS_FAIL) {
        this->err.PrintSysFailError("fork");
        return;
    }
    if (pid == 0) {
        if (setpgrp() == SYS_FAIL) {
            this->err.PrintSysFailError("setpgrp");
            return;
        }
        close(1);
        if (is_append) {
            this->fd = open(filename, O_APPEND | O_WRONLY | O_CREAT, 0655);
        } else {
            this->fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0655);
        }

        if (fd == SYS_FAIL) {
            this->is_redir=false;
            this->err.PrintSysFailError("open");
        }
        else is_redir = true;

        if(is_redir){
            smash.executeCommand(this->command);
        }
        exit(0);
    }
    else{
        waitpid(pid, nullptr, 0);
    }
}

void RedirectionCommand::cleanup() {

}

/***--------------SmallShell implementation--------------***/

void SmallShell::ChangePrompt(string new_prompt) {
    if (new_prompt.empty()) {
        this->prompt = "smash> ";
    } else {
        this->prompt = new_prompt + "> ";
    }
}


SmallShell::SmallShell() {
    this->jobs_list = new JobsList();
    this->p_last_dir = nullptr;
    this->fg_pid = -1;
    this->fg_job_id=-1;
    this->fg_cmd = nullptr;
}

SmallShell::~SmallShell() {
    delete jobs_list;
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
    if (cmd_s.find(">>") != std::string::npos || cmd_s.find('>') != std::string::npos) {
        return new RedirectionCommand(cmd_line);
    }

    if (cmd_s.find("|&") != std::string::npos || cmd_s.find('|') != std::string::npos) {
        return new PipeCommand(cmd_line);
    }

    if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (firstWord.compare("chprompt") == 0) {
        return new Chprompt(cmd_line);
    } else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line, &p_last_dir);
    } else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line, jobs_list);
    } else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line, jobs_list);
    } else if (firstWord.compare("bg") == 0) {
        return new BackgroundCommand(cmd_line, jobs_list);
    } else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line, jobs_list);
    } else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line, jobs_list);
    } else if (firstWord.compare("fare") == 0) {
        return new FareCommand(cmd_line);
    } else if (firstWord.compare("setcore") == 0) {
        return new SetCoreCommand(cmd_line, jobs_list);
    } else {
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    jobs_list->removeFinishedJobs();
    Command *cmd = CreateCommand(cmd_line);
    if (cmd == nullptr) { //if unrecognized command was entered
        return;
    }
    cmd->execute();
}