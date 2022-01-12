//
// Created by Frank Li on 11/15/21.
//

#ifndef DRTM_EXTRA_H
#define DRTM_EXTRA_H

/*
 * Created by Tianxi, include all constants, variables and functions used.
 */

#include <string>

using namespace std;

// Consts
const size_t HEADER_DASHED_WIDTH_DEFAULT = 100;
const string ADAPTIVE_POLLING_DEFAULT = "0";
const string PRE_BPOLL_DURATION_DEFAULT = "1000";
const string EPOLL_TIMEOUT_DEFAULT = "2000";
const string USE_PERF_DEFAULT = "0";
const string DATE_FORMAT_DEFAULT = "%d%02d%02d-%02d:%02d:%02d";
const string PERF_EXTENSION = ".perf";

// Vars
extern int g_adaptive_polling;                  // Whether to use adpative polling or busy poll
extern int g_pre_bpoll_duration;                // Busy polling duration before sleep for event
extern int g_epoll_timeout;                    // Epoll sleep timeout
extern int g_use_perf;                          // Use per for workers or not
extern string perf_command;
extern string perf_output_dir;

// Funcs
string dashed_line(string header = "", int width = HEADER_DASHED_WIDTH_DEFAULT);
string read_env(string env_name, string default_val);

//std::string date_str(std::string format = FORMAT_DEFAULT);
std::string date_str();
int fork_excel(std::string command);
int interrupt_kill(int child_pid);

#endif //DRTM_EXTRA_H
