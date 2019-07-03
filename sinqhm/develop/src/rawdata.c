/*******************************************************************************
  rawdata.c
*******************************************************************************/

#include "sinqhm_types.h"
#include "controlshm.h"
#include "datashm.h"
#include "debugshm.h"
#include "rawdata.h"
#include "needfullthings.h"


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


#ifndef _INLINE
int doStoreRawData=0;
volatile uint32 *RawDataBuffer=0;

volatile uint32 *RawDataStoredPtr=0; 
volatile uint32 *RawDataMissedPtr=0;
volatile uint32 *RawDataSizePtr  =0;
 
#else

extern int doStoreRawData;
extern volatile uint32 *RawDataBuffer;

#endif


/*******************************************************************************
  function definitions
*******************************************************************************/

#ifndef _INLINE
/*******************************************************************************
 *
 * FUNCTION
 *   RawDataInit
 *
 ******************************************************************************/


void RawDataInit(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  int max_size;

  histo_descr_ptr = getShmHistoPtr();

  histo_descr_ptr->rawdata_stored = 0;
  histo_descr_ptr->rawdata_missed = 0;

  if (shm_cfg_ptr[CFG_SRV_DO_STORE_RAW_DATA])
  {
    if (!histo_descr_ptr->rawdata_offs)
    {
      max_size = dataShmAvail();
      histo_descr_ptr->rawdata_offs = dataShmAlloc(max_size);

      if (!histo_descr_ptr->rawdata_offs)
      {
        dbg_printf(DBGMSG_ERROR, "RawDataInit(): Error allocating Memory for Raw Data Buffer");
        shm_cfg_ptr[CFG_SRV_DO_STORE_RAW_DATA] = 0;
        histo_descr_ptr->rawdata_size = 0;
        doStoreRawData   = 0;
        RawDataBuffer    = 0;
        RawDataStoredPtr = 0;
        RawDataMissedPtr = 0;
        RawDataSizePtr   = 0;
        return;
      }
      else
      {
        histo_descr_ptr->rawdata_size = max_size;
      }
    }

    RawDataBuffer = dataShmOffsToPtr(histo_descr_ptr->rawdata_offs);
    RawDataStoredPtr = &histo_descr_ptr->rawdata_stored;
    RawDataMissedPtr = &histo_descr_ptr->rawdata_missed;
    RawDataSizePtr   = &histo_descr_ptr->rawdata_size;
    doStoreRawData=1;
  }
  else
  {
    doStoreRawData   = 0;
    RawDataBuffer    = 0;
    RawDataStoredPtr = 0;
    RawDataMissedPtr = 0;
    RawDataSizePtr   = 0;
  }
};

/*******************************************************************************/

void RawDataStop(void)
{
  doStoreRawData   = 0;
}
#endif /* _INLINE */

/*******************************************************************************
 *
 * FUNCTION
 *   storeRawData
 *
 ******************************************************************************/

__INLINE void storeRawData(lwl_pmc_val_type value)
{
  if ((*RawDataStoredPtr)*sizeof(uint32) < (*RawDataSizePtr))
  {
    RawDataBuffer[(*RawDataStoredPtr)++] = value;
  }
  else
  {
    UINT32_INC_CEIL(*RawDataMissedPtr);
  }
};

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 14:33:57 2017
__INLINE void store0mqRawData(packet_type* p) {
  memcpy(RawDataBuffer,p->data,p->length*sizeof(unsigned long long));
  (*RawDataStoredPtr) = 2*p->length;
};


/******************************************************************************/

