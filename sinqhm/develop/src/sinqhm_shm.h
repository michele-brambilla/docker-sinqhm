/*******************************************************************************
  sinqhm_shm.h
*******************************************************************************/

/**
 * This file belongs to the SINQ Histogram Memory implementation for 
 * LINUX-RTAI. SINQHM-RTAI uses two shared memory sections: one for control
 * information and another one where the bank information and the actual
 * histogram is stored. This file describes the interface to the control
 * section of SINQHM-RTAI. The idea is that the knowledge about the layout
 * of this section stays within these files.
 * 
 * Gerd Theidel, Mark Koennecke, September 2005
 */

#ifndef _SINQHM_SHM_H_
#define _SINQHM_SHM_H_


/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#define __INLINE static inline
#else
#define __INLINE
#endif


/*******************************************************************************
  includes
*******************************************************************************/

#ifdef FILLER_RTAI
#include <linux/kernel.h>
//#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_shm.h>

#elif defined FILLER_USER

#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#endif

#include "sinqhm_errors.h"

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/





/*******************************************************************************
  global vars
*******************************************************************************/


/*******************************************************************************
  function prototypes
*******************************************************************************/

/*******************************************************************************
  sinqhm_shm_malloc
*******************************************************************************/

static inline void *sinqhm_shm_malloc(int key, int *id, int size)
{
#ifdef FILLER_RTAI

#ifdef APP_FILLER
  return rtai_kmalloc(key, size);
#elif defined APP_SERVER
  return rtai_malloc(key, size);
#endif


#elif defined FILLER_USER
  // shared memory in linux user space
  struct shmid_ds shm;
  void* dataPtr = (void *)-1;

  *id = shmget(key, size, IPC_CREAT | 0666);
  if(*id < 0)
  {
    return 0;
  }
  dataPtr = (void*)shmat(*id,NULL,0);
  if(dataPtr == (void *)-1)
  {
    shmctl(*id,IPC_RMID,&shm);
    return 0;
  }
  else
  {
    return dataPtr;
  }
#endif

}

/*******************************************************************************
  sinqhm_shm_free
*******************************************************************************/

static inline int sinqhm_shm_free(int key, int id, void *ptr)
{
#ifdef FILLER_RTAI

  return rt_shm_free(key);

#elif defined FILLER_USER
  // shared memory in linux user space
  int status;
  struct shmid_ds shm;

  status = shmdt(ptr);
  if(status < 0) return status;

  shmctl(id,IPC_RMID,&shm);
  return 0;
#endif
}

/******************************************************************************/

#endif //SINQHM_SHM_H
