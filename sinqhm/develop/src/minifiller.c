/**
* This is a program which mimics a filler program. Its serves to debug
* the filler-control  handshake.
* 
* Mark Koennecke, Gerd Theidel, Spetember 2005, January 2006
*/
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include "controlshm.h"
#include "datashm.h"
#include "fillerhandshake.h"

/**
 * RTAI has problems with dynamic allocation of shared memory.
 * We therefore use static allocation. We need two sizes: datasize and 
 * debug size. Boths sizes are in MB
 */
#define HMSIZE 10
#define DEBSIZE 2

int hst_size;
/*---------------------------------------------------------------------*/
int main(int argc, char *argv[]){
	int oldStatus, status;
	
	initShmControl();
	setControlVar(CFG_SRV_HST_SHM_SIZE, HMSIZE*1024*1000);
	hst_size = HMSIZE*1024*1000;
        initShmHisto();
        setControlVar(CFG_SRV_CFG_VALID, CFG_VALID);
	oldStatus = doHandshake();
	printf("Mini Filler Started with DAQ = %d\n", oldStatus);
	while(1){
		sleep(1);
		status = doHandshake();
		if(status != oldStatus){
			printf("DAQ switched from %d to %d\n", oldStatus, status);
			oldStatus = status;
		}
	}
	exit(0);
}
