#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <bcon.h>
#include <bson.h>
#include <mongoc.h>

#include "log.h"

FILE *logfile_limited = 0;

void open_logfile() {
  static int logfile_counter = 0;
  char filename[1024];
  char buffer[1024];
  time_t t;
  struct tm *tm;

  if (logfile_limited) {
    fclose(logfile_limited);
    logfile_limited = 0;
  }

  t = time(NULL);
  tm = localtime(&t);
  strftime(buffer, sizeof(buffer), "%F", tm);
  snprintf(filename, 1024, "sinqhm_%s.log.%d", buffer, logfile_counter);

  logfile_limited = fopen(filename, "a");
  if (!logfile_limited) {
    return;
  }
  printf("Open logfile: %s\n", filename);
  logfile_counter++;

  if (setvbuf(logfile_limited, NULL, _IONBF, 1024) != 0) {
    perror("Warning: unable to set no buffering debug file\n");
  }
  return;
}

void write_logfile(const int level, const char *subsystem, const char *msg) {}

// void write_logfile(const int level, const char* subsystem, const char * msg)
// {
//
//   struct tm *timeinfo;
//   time_t rawtime;
//   static int message_counter = 0;
//   char * timetext;
//   char * message;
//
//   if( !logfile_limited ) {
//     open_logfile();
//     if( !logfile_limited ) {
//       perror("Unable to open logfile\n");
//       return;
//     }
//   }
//
//   timetext = calloc(128,sizeof(char));
//   time(&rawtime);
//   timeinfo = localtime (&rawtime);
//   strftime (timetext,128,"%FT%H:%M:%S",timeinfo);
//
//   message = calloc(1024,sizeof(char));
//   fprintf(logfile_limited,"time : %s,\tseverity : %d,\tsub : %s,\tmessage :
//   %s\n",
// 	  timetext, level, subsystem, msg);
//   message_counter++;
//
//   if(message_counter % MAX_MESSAGES_IN_LOGFILE == (MAX_MESSAGES_IN_LOGFILE-1)
//   ) {
//     fclose(logfile_limited);
//     logfile_limited = 0;
//   }
//   free(message);
//   free(timetext);
// }

int logfile_cleanup() {
  if (logfile_limited) {
    return fclose(logfile_limited);
  }
  return LOGFILE_NOT_INITIALIZED;
}
