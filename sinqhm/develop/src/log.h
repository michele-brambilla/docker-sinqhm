#ifndef _LOG_H_
#define _LOG_H_

#include <time.h>

#define MAX_MESSAGES_IN_LOGFILE 10000

#define LOGFILE_INITIALIZED      0
#define LOGFILE_NOT_INITIALIZED -1
#define LOGFILE_INIT_ERROR      -2


void write_logfile(const int level, const char* subsystem, const char * msg);
int logfile_cleanup();

#endif




