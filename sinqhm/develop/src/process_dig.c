/*******************************************************************************
  process_dig.c
*******************************************************************************/


/*******************************************************************************
  includes
*******************************************************************************/

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

// axis 0 is channel axis
static process_axis_type axis_0;

/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_dig_init_daq(void);
void process_dig(packet_type *p);
void process_dig_destruct(void);


/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_dig_construct
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

int process_dig_construct(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  int status;

  histo_descr_ptr = getShmHistoPtr();

  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_dig_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }

  if (histo_descr_ptr->nBank != 1)
  {
    dbg_printf(DBGMSG_ERROR, "process_dig_construct(): nbank must be 1 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_BANKS;
  }

  bank_descr_ptr = getBankDescription(0);
  if(!bank_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_dig_construct(): can not get bank 0 description pointer\n");
    return SINQHM_ERR_NO_BANK_DESCR_PTR;
  }

  if (bank_descr_ptr->rank != 1)
  {
    dbg_printf(DBGMSG_ERROR, "process_dig_construct(): naxis must be 1 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
  }

  process_packet_fcn   = process_dig;
  process_init_daq_fcn = process_dig_init_daq;
  process_destruct_fcn = process_dig_destruct;

  // bank 0, axis 0 is x - axis
  axis_descr_ptr = getAxisDescription(0, 0);
  status = SetAxisMapping(axis_descr_ptr, &axis_0, DO_RANGE_CHECK);

  if (status<0) return status;

  histoDataPtr=getHistDataPtr();
  if(!histoDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_dig_construct(): can not get histo data pointer\n");
    return SINQHM_ERR_NO_HISTO_DATA_PTR;
  }

  return SINQHM_OK;
}

/******************************************************************************/

void process_dig_init_daq(void)
{
  *(axis_0.cnt_low_ptr)  = 0;
  *(axis_0.cnt_high_ptr) = 0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_dig_destruct
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

void process_dig_destruct(void)
{
  histoDataPtr = 0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_packet_dig
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

void process_dig(packet_type *p)
{
  uint32 header_type;
  int32  channel;

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if (header_type == LWL_HM_NC_C1)
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
      channel = axis_0.fcn(p->data[1], &axis_0);

      if (channel >= 0)
      {
        UINT32_INC_CEIL_CNT_OVL(histoDataPtr[channel], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
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
