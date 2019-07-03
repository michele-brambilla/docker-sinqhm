#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "mongo.h"

int main() {
  int i;
  //  write_logfile(1,"com","Multi packet data message");
  srand ( time(NULL) );
  for(i=0;i<10;++i) {
    sleep(rand()%5);
    mongohm_log(1,"com","Multi packet data message");
  }

  return 0;

}
