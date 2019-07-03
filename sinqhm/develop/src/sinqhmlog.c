
//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Mon Apr 24 14:24:35 2017
#include "log.h"
#include "mongo.h"

void sinqhmlog(const int level, const char* subsystem, const char * msg) {  
  mongohm_log(level, subsystem, msg);
  write_logfile(level, subsystem, msg);
}

void sinqhmlog_cleanup() {
  mongohm_cleanup();
  logfile_cleanup();
}
