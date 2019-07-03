/*******************************************************************************
  datashm.c
*******************************************************************************/

/**
 * This file belongs to the Linux-RTAI implementation of the SINQ histogram
 * memory software. HM software on Linux consists of two subsystems:
 * - the filler which is responsible for the actual histogramming
 * - a server (http) which is responsible for configuration and
 *   communication.
 * Both processes communicate through shared memory. Three regions are used:
 * - a shared memory area for control
 * - a data shared memory area.
 * - debugging area
 * This is the header file for the management of the data area. This also
 * includes the configuration of the histogram memory.
 *
 * The layout of the data section is as follows:
 * At the start there is a ID followed by the offsets to the bank data.
 * Multiple banks are supported. For each bank there is a number indicating the
 * rank of the histogram memory bank and an oftest to the data. This is
 * followed by rank axis descriptors: An axis descriptor consists of three
 * numbers: an axis flag denoting the type of
 * the axis, the length of the axis, and an offset to auxiliary data for this
 * axis. As of now, three axis modes will be implemented: direct: the value
 * from the LWL is the bin number, divisor: for equidistant binng. Auxiliary
 * data will be an offset to subtract and the divisor, lookuptable: the most
 * general form for irregular binnings, auxiliary data will be length integers
 * describing the bin boundaries. The axis description is followed by the
 * axis auxiliary data which is followed by the actual data area. Then the
 * section for the next bank will begin.
 *
 * All offset values are with regard to the start of the data shared memory
 *
 * Mark Koennecke, Gerd Theidel, September 2005, January 2006
 */


#ifndef _DATASHM_C_
#define _DATASHM_C_

/*******************************************************************************
  includes
*******************************************************************************/
#ifdef ARCH_X86
#include <stdlib.h>
#endif

#include "controlshm.h"
#include "datashm.h"
#include "sinqhm_errors.h"


/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#define __INLINE static inline
#else
#define __INLINE 
#endif


/*******************************************************************************
  global vars
*******************************************************************************/

/**
 * the pointer to histo configuration and data
 */
#ifndef _INLINE
 volatile histo_descr_type *dataPtr = NULL;
#else
extern volatile histo_descr_type *dataPtr;
#endif

static int dataID = -1;

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   initShmHisto
 *
 ******************************************************************************/

__INLINE int initShmHisto(void)
{
  dataPtr = (volatile histo_descr_type *)sinqhm_shm_malloc(SHM_KEY_HISTOMEM, &dataID, hst_size);
  if(dataPtr == NULL)
  {
    return SINQHM_ERR_ALLOCFAIL;
  }
  else
  {
    return SINQHM_OK;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   releaseShmHisto
 *
 ******************************************************************************/

__INLINE void releaseShmHisto(void)
{
  if(dataPtr)
  {
    sinqhm_shm_free(SHM_KEY_HISTOMEM, dataID, (void*)dataPtr);
    dataPtr = NULL;
  }
}


/******************************************************************************
 * true action section: data access                                           *
 ******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   getBankDescription
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

__INLINE volatile bank_descr_type* getBankDescription(int bankno)
{

  //  volatile histo_descr_type *histo_descr_ptr;
  volatile bankOffs_type   *bankOffs_ptr;
  volatile bank_descr_type *bank_descr_ptr;

  //  histo_descr_ptr = getShmHistoPtr();

  if(!dataPtr || bankno>dataPtr->nBank || !dataPtr->bank_descr.offs)
  {
    return 0;
  }

  bankOffs_ptr = (volatile bankOffs_type*) dataShmOffsToPtr(dataPtr->bank_descr.offs);

  if(!bankOffs_ptr[bankno])
  {
    return 0;
  }

  bank_descr_ptr = (volatile bank_descr_type*) dataShmOffsToPtr(bankOffs_ptr[bankno]);

  return bank_descr_ptr;
}


/*******************************************************************************
 *
 * FUNCTION
 *   getAxisDescription
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

__INLINE volatile axis_descr_type *getAxisDescription(int bankno, int axisno)
{
  volatile bank_descr_type *bank_descr_ptr;
  volatile axis_descr_type *axis_descr_ptr;

  bank_descr_ptr = getBankDescription(bankno);

  if(!bank_descr_ptr || axisno>=bank_descr_ptr->rank)
  {
    return 0;
  }

  axis_descr_ptr = (volatile axis_descr_type*) dataShmOffsToPtr(bank_descr_ptr->axis_descr.offs);

  return &axis_descr_ptr[axisno];
}


/*******************************************************************************
 *
 * FUNCTION
 *   getAxisData
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

__INLINE volatile uint32 *getAxisData(volatile axis_descr_type  *axis_descr_ptr)
{
  if(axis_descr_ptr == 0)
  {
    return 0;
  }

  return ((unsigned int *) dataShmOffsToPtr(axis_descr_ptr->axis_data.offs));
}


/*******************************************************************************
 *
 * FUNCTION
 *   getBankData
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

__INLINE volatile uint32 *getBankData(int bankno)
{
  volatile bank_descr_type *bank_descr_ptr;

  bank_descr_ptr = getBankDescription(bankno);

  if(!bank_descr_ptr || !bank_descr_ptr->bank_data_offs)
  {
    return 0;
  }

  return  (volatile uint32*) dataShmOffsToPtr(bank_descr_ptr->bank_data_offs);
}


/*******************************************************************************
 *
 * FUNCTION
 *   getBankDataSize
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

__INLINE int getBankDataSize(int bankno)
{
  volatile bank_descr_type *bank_descr_ptr;

  bank_descr_ptr = getBankDescription(bankno);
  return bank_descr_ptr->bank_size;
}


/*******************************************************************************
 *
 * FUNCTION
 *   getHistDataPtr
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

__INLINE volatile uint32 *getHistDataPtr(void)
{
  if (dataPtr)
  {
    return  (volatile uint32*) dataShmOffsToPtr(dataPtr->histo_mem_offs);
  }
  else
  {
    return 0;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   getHistoDataSize
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

__INLINE unsigned int getHistoDataSize(void)
{
  if (dataPtr)
  {
    return dataPtr->histo_mem_size;
  }
  else
  {
    return 0;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   getShmHistoPtr
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

__INLINE   volatile histo_descr_type *getShmHistoPtr(void)
{
  return dataPtr;
}


/*******************************************************************************
 *
 * FUNCTION
 *   dataShmOffsToPtr
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

__INLINE   volatile uint32 *dataShmOffsToPtr(uint32 offset)
{
  return (((uint32*)dataPtr)+offset);
}

/*******************************************************************************
 *
 * FUNCTION
 *   dataShmAvail
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

unsigned int dataShmAvail(void)
{
  unsigned int ptr;

  ptr = (dataPtr->cfg_mem_used+3) & 0xFFFFFFFC;    // align to 32 bit boundary
  return ((dataPtr->cfg_mem_size)-ptr);
}

/*******************************************************************************
 *
 * FUNCTION
 *   dataShmAlloc
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

unsigned int dataShmAlloc(unsigned int size)
{
  unsigned int ptr;

  if (size <= dataShmAvail())
  {
    ptr = (dataPtr->cfg_mem_used+3) & 0xFFFFFFFC;    // align to 32 bit boundary
    dataPtr->cfg_mem_used = ptr+size;
    return (ptr>>2);                           /* return offset in uint 32 array */
  }
  else
  {
    return 0;
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   dataShmFreeAll
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

void dataShmFreeAll(void)
{
  dataPtr->cfg_mem_used = (HM_CFG_DYNAMIC_MEM_START*sizeof(uint32));
}


/******************************************************************************/

#endif /* _DATASHM_C_ */
