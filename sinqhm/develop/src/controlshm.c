/*******************************************************************************
  controlshm.c
*******************************************************************************/

/**
 * This file belongs to the SINQ Histogram Memory implementation for
 * LINUX-RTAI. SINQHM-RTAI uses three shared memory sections: one for control
 * information, one where the bank information and the actual
 * histogram is stored and a deabug area. This file describes the interface to the control
 * section of SINQHM-RTAI. The idea is that the knowledge about the layout
 * of the control shared memory stays within these files.
 *
 * Gerd Theidel, Mark Koennecke, September 2005
 */

#ifndef _CONTROLSHM_C_
#define _CONTROLSHM_C_


/*******************************************************************************
  includes
*******************************************************************************/
#ifdef ARCH_X86
#include <stdlib.h>
#endif

#include "controlshm.h"
#include "sinqhm_errors.h"


/*******************************************************************************
  constant and macro definitions
*******************************************************************************/



/*******************************************************************************
  global vars
*******************************************************************************/

#ifndef _INLINE
/**
 * the pointer to real data
 */
volatile unsigned int *controlDataPtr = NULL;
#else
extern volatile unsigned int *controlDataPtr;
#endif

static int controlID = -1;

/*******************************************************************************
  function declarations
*******************************************************************************/


/*******************************************************************************
 *
 * FUNCTION
 *   initShmControl
 *
 ******************************************************************************/

__INLINE int initShmControl(void)
{
  controlDataPtr = (volatile unsigned int*) sinqhm_shm_malloc(SHM_KEY_CONTROL,&controlID,SHM_CFG_SIZE);

  if(controlDataPtr == NULL)
  {
    return SINQHM_ERR_ALLOCFAIL;
  }
  return SINQHM_OK;
}


/*******************************************************************************
 *
 * FUNCTION
 *   releaseShmControl
 *
 ******************************************************************************/

__INLINE void releaseShmControl(void)
{
  if (controlDataPtr)
  {
    sinqhm_shm_free(SHM_KEY_CONTROL, controlID, (void*)controlDataPtr);
    controlDataPtr=NULL;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   getVarPointer
 *
 * DESCRIPTION
 *   
 *
 * PARAMETERS
 *   
 *
 * RETURNS
 *   
 *
 ******************************************************************************/

__INLINE volatile unsigned int *getVarPointer(int varID)
{

  if(varID < 0 || varID >= (SHM_CFG_SIZE/sizeof(int)) || controlDataPtr == NULL)
  {
    return NULL;
  }
  return (volatile unsigned int *)&controlDataPtr[varID];
}


/*******************************************************************************
 *
 * FUNCTION
 *   getControlVar
 *
 * DESCRIPTION
 *   
 *
 * PARAMETERS
 *   
 *
 * RETURNS
 *   
 *
 ******************************************************************************/

__INLINE int getControlVar(int varID, int* var)
{
  volatile unsigned int *dataPtr = NULL;

  dataPtr = getVarPointer(varID);
  if(dataPtr == NULL)
  {
    return SINQHM_ERR_NOTFOUND;
  }
  else
  {
    *var = *dataPtr;
    return 0;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   setControlVar
 *
 * DESCRIPTION
 *   
 *
 * PARAMETERS
 *   
 *
 * RETURNS
 *   
 *
 ******************************************************************************/

__INLINE int setControlVar(int varID, int value)
{
  volatile unsigned int *dataPtr = NULL;

  dataPtr = getVarPointer(varID);
  if(dataPtr == NULL)
  {
    return SINQHM_ERR_NOTFOUND;
  }
  else
  {
    *dataPtr = value;
    return 0;
  }
}

/******************************************************************************/

#endif /* _CONTROLSHM_C_ */
