//
// Created by Frank Li on 11/15/21.
//

#include "extra.h"
#include <cstring>
#include <stdlib.h>
#include <algorithm>
#include <glog/logging.h>
#include <unistd.h>
#include <signal.h>


using namespace std;


// Below are added by Tianxi
int g_adaptive_polling;
int g_pre_bpoll_duration;
int g_epoll_timeout;
int g_use_perf;
string perf_command = "perf record -e cycles ";
string perf_output_dir = "/users/PES0781/frankli/git/drtm/perf_record/";

std::string dashed_line(std::string header, int width) {
    if (header != "") {
        int header_width = header.length() + 2;
        int left_width = (width - header_width) / 2;
        int right_width = width - left_width - header_width;
        char ret[width + 1];
        memset(ret, '-', left_width);

        ret[left_width] = ' ';
        memcpy(ret + left_width + 1, header.data(), header.length());
        ret[left_width + 1 + header.length()] = ' ';
        memset(ret + left_width + header_width, '-', right_width);
        ret[width] = 0;
        return std::string(ret);
    } else {
        return std::string(width, '-');
    }
}

std::string read_env(std::string env_name, std::string default_val) {
    char *env_str = getenv(env_name.data());
    if (!env_str) {
        return default_val;
    } else {
        return std::string(env_str);
    }
}

std::string date_str() {
    std::string format = DATE_FORMAT_DEFAULT;
    // A more strict check needed.
    if (std::count(format.begin(), format.end(), 'd') != 6) {
        format = DATE_FORMAT_DEFAULT;
    }

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date_buf[200];
    memset(date_buf, 0, 200);
    sprintf(date_buf, format.data(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(date_buf);
}

int fork_excel(std::string command) {
    int cpid = fork();

    if (cpid == 0) {
        // child process .  Run your perf stat
        VLOG(2) << "fork_excel, in child process, command=" << command;
        // From execl manpage: The exec() functions only return if an error has occurred. The return value is -1, and errno is set to indicate the error.
        execl("/bin/sh", "sh", "-c", command.data(), NULL);

        int cpid_in_child = getpid();
        PLOG(ERROR) << "fork_excel, fail to execute command=" << command << ", child_process_id=" << cpid_in_child;
        return cpid_in_child;
    } else {
        // set the child the leader of its process group
        setpgid(cpid, 0);

        VLOG(0) << "fork_excel, in parent process, command=" << command << ", child_pid=" << cpid;

        return cpid;
    }
}

// Stop and kill the child perf process
int interrupt_kill(int child_pid) {
    int ret = kill(-child_pid, SIGINT);
    if (ret) {
        PLOG(ERROR) << "Fail to kill child process, pid=" << child_pid;
    }

    return ret;
}