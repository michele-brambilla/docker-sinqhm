/*
 * This is a little support program which fills a sinqhm  with data 
 * from a NeXus file. With optional scaling. It connects directly to 
 * the data shared memory. It does some basic testing such that dimensions 
 * match. This is to be used for software testing of the chain sinqhm-SICS-
 * data file. 
 *
 * Mark Koennecke, April 2013
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "datashm.h"
#include "controlshm.h"
#include <napi.h>

int hst_size = DEFAULT_HISTO_MEM_SIZE;

static void printUsage(void)
{
  printf("Usage:\n\tnxfiller filename path-in-file [scale]\n");
}
/*-------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  float scale = 1.0;
  NXhandle hfil;
  int status, i, hmdata, *fileData, exitval = EXIT_SUCCESS;
  unsigned int nbin = 1;
  int32 rank, type, dim[NX_MAXRANK]; 
  volatile axis_descr_type *axis;
  volatile uint32 *dataPtr;
  unsigned long lSum = 0;

  if(argc < 2){
    printUsage();
    exit(EXIT_FAILURE);
  }

  status = NXopen(argv[1],NXACC_READ,&hfil);
  if(status == NX_ERROR){
    printf("Failed to open %s for reading\n",argv[1]);
    exit(EXIT_FAILURE);
  }

  status = NXopenpath(hfil,argv[2]);
  if(status == NX_ERROR){
    printf("Failed to open data path %s\n",argv[2]);
    NXclose(hfil);
    exit(EXIT_FAILURE);
  } 

  if(argc > 3){
    scale = atof(argv[3]);
  } 

  NXgetinfo(hfil,&rank,dim,&type);
  if(type != NX_INT32) {
    NXclose(hfil);
    printf("%s is not of NX_INT32 type\n",argv[2]);
    exit(EXIT_FAILURE);
  }

  /*
    connect to configuration shared memory
  */
  initShmControl();
  getControlVar(CFG_FIL_HST_SHM_SIZE,&hst_size);
  shmdt((const void *)getControlVar(0,&i));
  printf("Histsize = %d\n", hst_size);

  /*
    check dimensions
  */
  hmdata = initShmHisto();
  if(hmdata < 0){
    printf("Failed to connect to sinqhm shared memory\n");
    exitval = EXIT_FAILURE;
    goto finish;
  }
  for(i = 0; i < rank ; i++){
    axis = getAxisDescription(0,i);
    if(axis == NULL || axis->length != dim[i]){
      if(axis == NULL){
	printf("No axis for dimension %d\n", i); 
      } else {
	printf("Dimension mismatch on dimension %d, %d versus %d\n", i, axis->length, dim[i]);
      }
      exitval = EXIT_FAILURE;
      goto finish;
    }
    nbin *= dim[i];
  }

  /*
    copy data
  */
  dataPtr = getBankData(0);
  fileData = malloc(nbin*sizeof(int));
  if(dataPtr == NULL || fileData == NULL){
      printf("Failed to get HM data pointer or out of memory\n");
      exitval = EXIT_FAILURE;
      goto finish;
  }
  NXgetdata(hfil,fileData);
  for(i = 0; i < nbin; i++){
    dataPtr[i] = (int)round((float)fileData[i] * scale);
    lSum += dataPtr[i];
  }
  printf("%ld counts copied\n", lSum);

 finish:
  shmdt((const void *)getShmHistoPtr());
  NXclose(hfil);
  free(fileData);
  exit(exitval);

}
