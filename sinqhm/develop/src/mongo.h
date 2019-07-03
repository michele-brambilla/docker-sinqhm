#ifndef _MONGOHM_H_
#define _MONGOHM_H_

#include <stdio.h>
#include <time.h>

//#include <bson.h>
//#include <bcon.h>
//#include <mongoc.h>

#include "log.h"

void mongohm_cleanup();
void mongohm_log(const int level, const char* subsystem, const char * msg);
int mongohm_ok();

#define MONGOHM_INITIALIZED      0
#define MONGOHM_NOT_INITIALIZED -1
#define MONGOHM_INIT_FAILURE    -2
#define MONGOHM_MESSAGE_ERROR  -10
#define MONGOHM_INSERT_ERROR   -11

#endif
