/*
 * This is a little support program which fills a sinqhm  with data
 * from a NeXus file. With optional scaling. It connects directly to
 * the data shared memory. It does some basic testing such that dimensions
 * match. This is to be used for software testing of the chain sinqhm-SICS-
 * data file.
 *
 * Mark Koennecke, April 2013
 */
#include "controlshm.h"
#include "datashm.h"
#include "hdf5.h"
#include "hdf5_hl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXRANK 32

int hst_size = DEFAULT_HISTO_MEM_SIZE;

static void printUsage(void) {
  printf("Usage:\n\tnxfiller filename path-in-file [scale]\n");
}
/*-------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  float scale = 1.0;
  int status, hmdata, *fileData, exitval = EXIT_SUCCESS;
  unsigned int nbin = 1;
  hsize_t rank, type, dim[MAXRANK];
  volatile axis_descr_type *axis;
  volatile uint32 *dataPtr;
  unsigned long lSum = 0;
  hid_t file_id;
  herr_t err;
  int64_t i;

  if (argc < 2) {
    printUsage();
    exit(EXIT_FAILURE);
  }
  if (argc > 3) {
    scale = atof(argv[3]);
  }

  file_id = H5Fopen(argv[1], H5F_ACC_RDONLY, H5P_DEFAULT);

  err = H5LTget_dataset_ndims(file_id, argv[2], &rank);
  if (err < 0) {
    printf("Unable to determine the histogram rank");
    exit(-1);
  }
  err = H5LTget_dataset_info(file_id, argv[2], dim, NULL, NULL);
  if (err < 0) {
    printf("Unable to determine the histogram dimensions");
    exit(-1);  
  }
  printf("rank = %d\n", rank);
  for (i = 0; i < 2*rank;  i+=2) {
    printf("%d\t", dim[i]);
  }
  printf("\n");

  // NXgetinfo(hfil, &rank, dim, &type);
  // if (type != NX_INT32) {
  //   NXclose(hfil);
  //   printf("%s is not of NX_INT32 type\n", argv[2]);
  //   exit(EXIT_FAILURE);
  // }

  /*
    connect to configuration shared memory
  */
  initShmControl();
  getControlVar(CFG_FIL_HST_SHM_SIZE, &hst_size);
  shmdt((const void *)getControlVar(0, &i));
  printf("Histsize = %d\n", hst_size);

  /*
    check dimensions
  */
  hmdata = initShmHisto();
  if (hmdata < 0) {
    printf("Failed to connect to sinqhm shared memory\n");
    exitval = EXIT_FAILURE;
    goto finish;
  }

  for (i = 0; i < rank;  i++) {
    printf("%d\t", getAxisDescription(0, i));
  }
  printf("\n");

  /*
  for (i = 0; i < rank; i++) {
    axis = getAxisDescription(0, i);
    if (axis == NULL || axis->length != dim[rank-i-1]) {
      if (axis == NULL) {
        printf("No axis for dimension %d\n", i);
      } else {
        printf("Dimension mismatch on dimension %d, %d versus %d\n", i,
               axis->length, dim[i]);
      }
      exitval = EXIT_FAILURE;
      goto finish;
    }
    nbin *= dim[i];
  }
  */

  /*
    copy data
  */
  dataPtr = getBankData(0);
  fileData = malloc(nbin * sizeof(int));
  if (dataPtr == NULL || fileData == NULL) {
    printf("Failed to get HM data pointer or out of memory\n");
    exitval = EXIT_FAILURE;
    goto finish;
  }
  H5LTread_dataset_int(file_id, argv[2], fileData);
  for (i = 0; i < nbin; i++) {
    dataPtr[i] = (int)round((float)fileData[i] * scale);
    lSum += dataPtr[i];
  }

  printf("%ld counts copied\n", lSum);

finish:
  shmdt((const void *)getShmHistoPtr());
  free(fileData);
  exit(exitval);
}
