/*******************************************************************************
  process_psd.c
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

static volatile uint32 *bank0_HistoDataPtr = 0;
static volatile uint32 *bank1_HistoDataPtr = 0;

// bank 0
// axis 0 is X axis
static process_axis_type b0_axis_0;

// axis 1 is Y axis
static process_axis_type b0_axis_1;

// axis 2 is Time Of Flight axis
static process_axis_type b0_axis_2;

// bank 1
// axis 0 is X axis
static process_axis_type b1_axis_0;

// axis 1 is Time Of Flight axis
static process_axis_type b1_axis_1;

unsigned int  ed_mode ;
unsigned int  b0_tof_mode;
unsigned int  b1_tof_mode;

/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_psd_init_daq(void);
void process_psd(packet_type *p);
void process_psd_destruct(void);

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_psd_construct
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

int process_psd_construct(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  int status;

  histo_descr_ptr = getShmHistoPtr();

  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_psd_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }

  if ((histo_descr_ptr->nBank != 1) && (histo_descr_ptr->nBank != 2))
  {
    dbg_printf(DBGMSG_ERROR, "process_psd_construct(): nbank must be 1 or 2 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_BANKS;
  }

  bank_descr_ptr = getBankDescription(0);
  if(!bank_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_psd_construct(): can not get bank description pointer\n");
    return SINQHM_ERR_NO_BANK_DESCR_PTR;
  }

  if ((bank_descr_ptr->rank != 2) && (bank_descr_ptr->rank != 3))
  {
    dbg_printf(DBGMSG_ERROR, "process_psd_construct(): naxis must be 2 or 3 for bank 0\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
  }

  if (bank_descr_ptr->rank == 3)
  {
    b0_tof_mode=1;
  }
  else
  {
    b0_tof_mode=0;
  }

  process_packet_fcn   = process_psd;
  process_init_daq_fcn = process_psd_init_daq;
  process_destruct_fcn = process_psd_destruct;

  // bank 0, axis 0 is x - axis
  axis_descr_ptr = getAxisDescription(0, 0);
  status = SetAxisMapping(axis_descr_ptr, &b0_axis_0, DO_RANGE_CHECK);
  if (status<0) return status;

  // bank 0, axis 1 is y - axis
  axis_descr_ptr = getAxisDescription(0, 1);
  status = SetAxisMapping(axis_descr_ptr, &b0_axis_1, DO_RANGE_CHECK);
  if (status<0) return status;



  if (b0_tof_mode)
  {
    // bank 0, axis 2 is tof - axis
    axis_descr_ptr = getAxisDescription(0, 2);
    status = SetAxisMapping(axis_descr_ptr, &b0_axis_2, DO_RANGE_CHECK);
    if (status<0) return status;
  }

  bank0_HistoDataPtr = getBankData(0);
  if(!bank0_HistoDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_psd_construct(): can not get bank data pointer\n");
    return SINQHM_ERR_NO_BANK_DATA_PTR;
  }

  if (histo_descr_ptr->nBank == 2)
  {
    ed_mode = 1;

    bank_descr_ptr = getBankDescription(1);
    if(!bank_descr_ptr)
    {
      dbg_printf(DBGMSG_ERROR, "process_psd_construct(): can not get bank description pointer\n");
      return SINQHM_ERR_NO_BANK_DESCR_PTR;
    }

	  if ((bank_descr_ptr->rank != 1) && (bank_descr_ptr->rank != 2))
	  {
  	    dbg_printf(DBGMSG_ERROR, "process_psd_construct(): naxis must be 1 or 2 for bank 1\n");
	    return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
	  }

	  if (bank_descr_ptr->rank == 2)
	  {
	    b1_tof_mode=1;
	  }
	  else
	  {
	    b1_tof_mode=0;
	  }


    // bank 1, axis 0 is x - axis
    axis_descr_ptr = getAxisDescription(1, 0);
    status = SetAxisMapping(axis_descr_ptr, &b1_axis_0, DO_RANGE_CHECK);
    if (status<0) return status;


    if (b1_tof_mode)
    {
      // bank 1, axis 1 is tof - axis
      axis_descr_ptr = getAxisDescription(1, 1);
      status = SetAxisMapping(axis_descr_ptr, &b1_axis_1, DO_RANGE_CHECK);
      if (status<0) return status;
    }

    bank1_HistoDataPtr = getBankData(1);
    if(!bank1_HistoDataPtr)
    {
      dbg_printf(DBGMSG_ERROR, "process_psd_construct(): can not get bank data pointer\n");
      return SINQHM_ERR_NO_BANK_DATA_PTR;
    }

  }
  else
  {
    ed_mode = 0;
  }

  return SINQHM_OK;
}

/******************************************************************************/

void process_psd_init_daq(void)
{
  *(b0_axis_0.cnt_low_ptr)  = 0;
  *(b0_axis_0.cnt_high_ptr) = 0;

  *(b0_axis_1.cnt_low_ptr)  = 0;
  *(b0_axis_1.cnt_high_ptr) = 0;

  if (b0_tof_mode)
  {
    *(b0_axis_2.cnt_low_ptr)  = 0;
    *(b0_axis_2.cnt_high_ptr) = 0;
  }

  if (ed_mode) 
  {
    *(b1_axis_0.cnt_low_ptr)  = 0;
    *(b1_axis_0.cnt_high_ptr) = 0;

    if (b1_tof_mode) 
    {
      *(b1_axis_1.cnt_low_ptr)  = 0;
      *(b1_axis_1.cnt_high_ptr) = 0;
    }
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_psd_destruct
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

void process_psd_destruct(void)
{
  bank0_HistoDataPtr = 0;
  bank1_HistoDataPtr = 0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_packet_psd
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

void process_psd(packet_type *p)
{
  uint32 header_type;
  uint32 timestamp,binpos;;
  int32  xPos,yPos,tofpos,edNum;

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if (header_type == LWL_PSD_DATA)
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

//      xPos = p->data[1];
//      xPos = (xPos - 2048)/psdXFactor;
      xPos = b0_axis_0.fcn(p->data[1], &b0_axis_0);


      if(xPos >= 0 )
      {
//      yPos = p->data[2];
//      yPos = (yPos - 2048)/psdYFactor;
        yPos = b0_axis_1.fcn(p->data[2], &b0_axis_1);

        if(yPos >= 0 )
        {
          if(!b0_tof_mode)
          {
			// no Time Of Flight
            binpos = (xPos * b0_axis_1.len) + yPos ;
            UINT32_INC_CEIL_CNT_OVL(bank0_HistoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
          }
          else
          {
            timestamp = p->data[0] & LWL_HDR_TS_MASK;
    //        dbg_printf("timestamp=%d --",timestamp);

            tofpos = b0_axis_2.fcn(timestamp, &b0_axis_2);
            if (tofpos>=0)
            {
    //        dbg_printf("xPos=%d yPos=%d tofpos=%d\n",xPos,yPos,tofpos);
              binpos = (xPos * b0_axis_2.len * b0_axis_1.len) + (yPos * b0_axis_2.len) + tofpos ;
              UINT32_INC_CEIL_CNT_OVL(bank0_HistoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
            }
          }
        }
      }
    }

  }
  else if (ed_mode && (header_type >= LWL_TOF_C1) && (header_type <= LWL_TOF_C3))
  {
    /* we have located a single detector packet from AMOR. */
    if ((p->data[0] & hdr_daq_mask) != hdr_daq_active)
    {
      // skip
      shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]++;
    }
    else
    {
      // process
      shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++;
/*
  We have a valid single detector packet.
Subtract one from the number because Urs Greuter starts
counting at 1, and ANSI-C at 0
*/
      edNum = b1_axis_0.fcn(p->data[1], &b1_axis_0);

      if(edNum >= 0)
      {

	    if (!b1_tof_mode)
        {
	      // no Time Of Flight
          UINT32_INC_CEIL_CNT_OVL(bank1_HistoDataPtr[edNum], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
	    }
	    else
	    {

          timestamp = p->data[0] & LWL_HDR_TS_MASK;
//        dbg_printf("timestamp=%d --",timestamp);

          tofpos = b1_axis_1.fcn(timestamp, &b1_axis_1);
          if (tofpos>=0)
          {
//          dbg_printf("xPos=%d yPos=%d tofpos=%d\n",xPos,yPos,tofpos);
            binpos = (edNum * b1_axis_1.len) + tofpos  ;
            UINT32_INC_CEIL_CNT_OVL(bank1_HistoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
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
