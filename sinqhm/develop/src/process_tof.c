/*******************************************************************************
  process_tof.c
*******************************************************************************/


/*******************************************************************************
  includes
*******************************************************************************/

#ifdef FILLER_RTAI
#include <rtai_malloc.h>
#else
#include <stdlib.h>
#define rt_malloc malloc
#define rt_free   free
#endif


#include "controlshm.h"
#include "debugshm.h"
#include "datashm.h"
#include "needfullthings.h"
#include "process_common.h"
#include "sinqhm_errors.h"


/*******************************************************************************
  global vars
*******************************************************************************/

extern volatile unsigned int *shm_cfg_ptr ;

static volatile uint32 *histoDataPtr = 0;
static uint32 nbank = 0;

/******************************
 vars for process_tof
 *****************************/

// axis 0 is channel axis
static process_axis_type axis_0;

// axis 1 is Time Of Flight axis
static process_axis_type axis_1;

static uint32 b0_rank;

/******************************
 vars for process_tof_mb
 *****************************/

typedef struct 
{ 
  int rank;
  volatile uint32 *data;
  process_axis_type* axis;
} banks_type;

static banks_type *banks = 0;

static uint32 bankmap_len = 0;
static volatile uint32 *bankmap_array = 0;


/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_tof_init_daq(void);
void process_tof(packet_type *p);
void process_tof_mb(packet_type *p);
void process_tof_destruct(void);

void process_gummi_monitor(packet_type *p);


/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_tof_construct
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

int process_tof_construct(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  volatile uint32           *bmap_arraydesc_ptr;

  int status;
  int axis, bank;

  histo_descr_ptr = getShmHistoPtr();

  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }

  histoDataPtr=getHistDataPtr();
  if(!histoDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not get histo data pointer\n");
    return SINQHM_ERR_NO_HISTO_DATA_PTR;
  }

  process_init_daq_fcn = process_tof_init_daq;
  process_destruct_fcn = process_tof_destruct;
  nbank = histo_descr_ptr->nBank;

  if (nbank == 1)
  {
    process_packet_fcn = process_tof;

    bank_descr_ptr = getBankDescription(0);
    if(!bank_descr_ptr)
    {
      dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not get bank description pointer for bank 0\n");
      return SINQHM_ERR_NO_BANK_DESCR_PTR;
    }

    b0_rank = bank_descr_ptr->rank;

    if ((b0_rank != 1) && (b0_rank != 2))
    {
      dbg_printf(DBGMSG_ERROR, "process_tof_construct(): naxis must be 1 or 2 for this filler type\n");
      return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
    }

    // bank 0, axis 0 is x - axis
    axis_descr_ptr = getAxisDescription(0, 0);
    status = SetAxisMapping(axis_descr_ptr, &axis_0, DO_RANGE_CHECK);
    if (status<0) return status;

    if (b0_rank > 1)
    {
      // bank 0, axis 1 is tof - axis
      axis_descr_ptr = getAxisDescription(0, 1);
      status = SetAxisMapping(axis_descr_ptr, &axis_1, DO_RANGE_CHECK);
      if (status<0) return status;
    }
  }
  else if (nbank > 1)
  {

    if (!histo_descr_ptr->bank_mapping_array.offs)
    {
      dbg_printf(DBGMSG_ERROR, "process_tof_construct(): no valid bank mapping array\n");
      return SINQHM_ERR_NO_BANK_MAPPING_ARRAY;
    }

    bmap_arraydesc_ptr = (uint32*)dataShmOffsToPtr(histo_descr_ptr->bank_mapping_array.offs);

    if(bmap_arraydesc_ptr[0] != 1)
    {
      dbg_printf(DBGMSG_ERROR, "process_tof_construct(): rank of bank mapping array must be 1\n");
      return SINQHM_ERR_NO_BANK_MAPPING_ARRAY;
    }    

    bankmap_len   = bmap_arraydesc_ptr[1];  
    bankmap_array = &bmap_arraydesc_ptr[2];
      
    process_packet_fcn = process_tof_mb;

    banks = (banks_type *)rt_malloc((nbank * sizeof(banks_type)));
    if(!banks)
    {
      dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not allocate memory for banks array\n");
      return SINQHM_ERR_FIL_MALLOC_FAILED;
    }    

    memset((void*)banks,0,(nbank * sizeof(banks_type)) );

    for (bank=0; bank<nbank; bank++)
    {
        
      bank_descr_ptr = getBankDescription(bank);
      if(!bank_descr_ptr)
      {
        dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not get bank description pointer for bank %d\n",bank);
        process_tof_destruct();
        return SINQHM_ERR_NO_BANK_DESCR_PTR;
      }

      if ((bank_descr_ptr->rank != 1) && (bank_descr_ptr->rank != 2))
      {
        dbg_printf(DBGMSG_ERROR, "process_tof_construct(): naxis must be 1 or 2 for this filler type\n");
        process_tof_destruct();
        return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
      }

      banks[bank].rank = bank_descr_ptr->rank;
      banks[bank].data = getBankData(bank);
      if(!banks[bank].data)
      {
        dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not get bank data pointer\n");
        return SINQHM_ERR_NO_BANK_DATA_PTR;
      }

      banks[bank].axis = (process_axis_type *)rt_malloc((bank_descr_ptr->rank * sizeof(process_axis_type)));
      if(!banks[bank].axis)
      {
        dbg_printf(DBGMSG_ERROR, "process_tof_construct(): can not allocate memory for axis array\n");
        return -108;
      }   

      memset((void*)banks[bank].axis, 0, (bank_descr_ptr->rank * sizeof(process_axis_type)) );

      for (axis=0; axis<banks[bank].rank ;axis++)
      {
        axis_descr_ptr = getAxisDescription(bank, axis);
        status = SetAxisMapping(axis_descr_ptr, &(banks[bank].axis[axis]), DO_RANGE_CHECK);
        if (status<0) 
        {
          process_tof_destruct();
          return status;
        }
      }

    }
  }

  return SINQHM_OK;
}

/******************************************************************************/

void process_tof_init_daq(void)
{
  int bank,axis;

  if (nbank==1)
  {
    *(axis_0.cnt_low_ptr)  = 0;
    *(axis_0.cnt_high_ptr) = 0;

    if(b0_rank>1)
    {
      *(axis_1.cnt_low_ptr)  = 0;
      *(axis_1.cnt_high_ptr) = 0;
    }
  }
  else if (nbank > 1)
  {
    if (banks)
    {
      for (bank=0; bank<nbank; bank++)
      { 
        for (axis=0; axis<banks[bank].rank; axis++)
        {
          *(banks[bank].axis[axis].cnt_high_ptr) = 0;
          *(banks[bank].axis[axis].cnt_low_ptr) = 0;
        }
      }
    }
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_tof_destruct
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

void process_tof_destruct(void)
{
  int bank;

  histoDataPtr = 0;

  // free memory
  if ((nbank > 1) && banks)
  {
    for (bank=0; bank<nbank; bank++)
    {
      if (banks[bank].axis)
      {
        rt_free((void*)banks[bank].axis);
      }
    }
    rt_free((void*)banks);
    banks = 0;
  }  

}

/*******************************************************************************
 *
 * FUNCTION
 *   process_packet_tof
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

void process_tof(packet_type *p)
{
  uint32 header_type;
  uint32 timestamp,binpos;;
  int32  channel,tofpos;

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if ((header_type >= LWL_TOF_C1) && (header_type <= LWL_TOF_C9))
  {
    if ((p->data[0] & hdr_daq_mask) != hdr_daq_active)
    {
      shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]++;
    }
    else  
    {
      // process
      shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++;

      channel = axis_0.fcn(p->data[1], &axis_0);
      if (channel>=0)
      {
        if (b0_rank == 1)
        {
          // no TOF needed
          UINT32_INC_CEIL_CNT_OVL(histoDataPtr[channel], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        }
        else
        {
          timestamp = p->data[0] & LWL_HDR_TS_MASK;
          dbg_printf(DBGMSG_INFO7,"timestamp=%d --",timestamp);
          tofpos = axis_1.fcn(timestamp, &axis_1);
          if (tofpos>=0)
          { 
            dbg_printf(DBGMSG_INFO7,"channel=%d tofpos=%d\n",channel,tofpos);
            binpos = (channel * axis_1.len) + tofpos;
            UINT32_INC_CEIL_CNT_OVL(histoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
          }
        }
      }
    }
   
  }
  else if (header_type == LWL_GUMMI_MON)
  {
    process_gummi_monitor(p);
  }
  else if (!process_tsi(p))
  {
    // unknown packet
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_PKG_UNKNOWN]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PKG_UNKNOWN]);
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   process_tof_mb
 *
 * DESCRIPTION
 *   process function for multiple banks.
 *
 * PARAMETERS
 *   
 *
 * RETURNS
 *   
 *
 ******************************************************************************/

void process_tof_mb(packet_type *p)
{
  uint32 header_type;
  uint32 timestamp,binpos;
  int32  channel,tofpos;
  int32  val, lookup, bank, bankch;

  print_packet(p);

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if ((header_type >= LWL_TOF_C1) && (header_type <= LWL_TOF_C9))
  {
    if ((p->data[0] & hdr_daq_mask) != hdr_daq_active)
    {
      // skip
      shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]++;
    }
    else  
    {
      // process
      shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++;

      //  get bank number
      val = p->data[1];
      dbg_printf(DBGMSG_INFO7,"p->data[1]=%d  \n",p->data[1]);

      if (val < 0)
      {
        // val low
        UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_LOW]);
        return;
      }
      else if (val >= bankmap_len)
      {
        // val high
        UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_HIGH]);
        return;
      }

      lookup = bankmap_array[val];

      bank   = lookup >> 16;
      bankch = lookup & 0xffff;
      dbg_printf(DBGMSG_INFO7,"bank=%d  bankch=%d \n",bank,bankch);

      if (bank>=nbank)
      {
        UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_HIGH]);
        return;
      }
    
      channel = banks[bank].axis[0].fcn(bankch, &banks[bank].axis[0]);
      dbg_printf(DBGMSG_INFO7,"axislookup:  channel=%d \n",channel);

      if (channel>=0)
      {
        if (banks[bank].rank == 1)
        {
          // no TOF needed
          UINT32_INC_CEIL_CNT_OVL(banks[bank].data[channel], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        }
        else
        {
          timestamp = p->data[0] & LWL_HDR_TS_MASK;
          dbg_printf(DBGMSG_INFO7,"timestamp=%d --",timestamp);
          tofpos = banks[bank].axis[1].fcn(timestamp, &banks[bank].axis[1]);
          if (tofpos>=0)
          { 
            dbg_printf(DBGMSG_INFO7,"channel=%d tofpos=%d\n",channel,tofpos);
            binpos = (channel * banks[bank].axis[1].len) + tofpos;
            UINT32_INC_CEIL_CNT_OVL(banks[bank].data[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
          }
        }
      }
    }
   
  }
  else if (!process_tsi(p))
  {
    // unknown packet
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_PKG_UNKNOWN]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PKG_UNKNOWN]);
  }

}


/*******************************************************************************/

void process_gummi_monitor(packet_type *p)
{
  uint32 mon[3];
  int32  channel,tofpos;
  int i;
  uint32 timestamp, binpos;

  UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_PKG_GUMMI_MONITOR]);
  UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PKG_GUMMI_MONITOR]);

/*
  put them together again
*/
  for(i = 0; i < 3; i++)
  {
    mon[i] = (p->data[2*i+1] << 16) | (p->data[2*i+2]);
  }

  timestamp = p->data[0] & LWL_HDR_TS_MASK;

  dbg_printf(DBGMSG_INFO7, "Gummi monitors: %d, %d, %d registered for timestamp=%d\n",
      mon[0],mon[1],mon[2],timestamp);

  tofpos = axis_1.fcn(timestamp, &axis_1);
  if (tofpos>=0)
  { 
    for(i = 0; i < 3; i++)
    {
      mon[i] = (p->data[2*i+1] << 16) | (p->data[2*i+2]);
      channel = axis_0.len - 3 + i;
      dbg_printf(DBGMSG_INFO7,"channel=%d tofpos=%d\n",channel,tofpos);
      binpos = (channel * axis_1.len) + tofpos;
      UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], mon[i], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
    }
  }
  
}
