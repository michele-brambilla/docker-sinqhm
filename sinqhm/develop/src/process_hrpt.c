/*******************************************************************************
  process_hrpt.c
*******************************************************************************/

#define FRAME_WAIT     0
#define FRAME_STARTED  1


/*******************************************************************************
  includes
*******************************************************************************/

#ifdef FILLER_RTAI
#include <rtai_malloc.h>
#else

#include <stdlib.h>
#define rt_malloc malloc
#define rt_free   free
//#include <string.h>

#endif


#include "controlshm.h"
#include "debugshm.h"
#include "datashm.h"
#include "needfullthings.h"
#include "process_common.h"
#include "sinqhm_errors.h"

/*******************************************************************************
  defines 
*******************************************************************************/

#define HRPT_NUM_WIRES   1600
// Monitor count directly after wires
#define HRPT_MONITOR_BIN (HRPT_NUM_WIRES + 0)


/*******************************************************************************
  file wide global vars 
*******************************************************************************/

extern volatile unsigned int *shm_cfg_ptr ;

static volatile uint32 *histoDataPtr = 0;
static volatile uint32 *frameDataPtr = 0;

static volatile uint32 *cnt_low_ptr  = 0;
static volatile uint32 *cnt_high_ptr = 0;
static uint32 axislength = 0;
static uint32 last_valid_processed_frame_num;
static uint32 mon_count_low_frame_num;
static uint32 frame_num;
static int last_valid_processed_strobopos;
static uint32 skip_frame;
static uint32 state = FRAME_WAIT;
static uint32 channel_count;
static uint32 picture_length;
static uint32 monitor_count;
static int Hi_bin;
static int Lo_bin;

static unsigned int  strobo_mode;

// axis 0 is X axis
// static process_axis_type axis_0;

// axis 1 is strobo axis
static process_axis_type axis_1;



/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_hrpt_init_daq(void);
void process_hrpt(packet_type *p);
void process_hrpt_destruct(void);


/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_hrpt_construct
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

int process_hrpt_construct(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  int status;
  
  histo_descr_ptr = getShmHistoPtr();

  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }

  if (histo_descr_ptr->nBank != 1)
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): nbank must be 1 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_BANKS;
  }

  bank_descr_ptr = getBankDescription(0);
  if(!bank_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): can not get bank 0 description pointer\n");
    return SINQHM_ERR_NO_BANK_DESCR_PTR;
  }

  if ((bank_descr_ptr->rank != 1) && (bank_descr_ptr->rank != 2))
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): naxis must be 1 or 2 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
  }

  if (bank_descr_ptr->rank == 2)
  {
    strobo_mode=1;
  }
  else
  {
    strobo_mode=0;
  }

  process_packet_fcn   = process_hrpt;
  process_init_daq_fcn = process_hrpt_init_daq;
  process_destruct_fcn = process_hrpt_destruct;

  // bank 0, axis 0 is x - axis
  axis_descr_ptr = getAxisDescription(0, 0);
  axislength = axis_descr_ptr->length;
  cnt_low_ptr  =&(axis_descr_ptr->cnt_low);
  cnt_high_ptr =&(axis_descr_ptr->cnt_high);

//  status = SetAxisMapping(axis_descr_ptr, &axis_0, DO_RANGE_CHECK);
  if (axis_descr_ptr->type != AXDIRECT)
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): mapping for axis 0 must be direct\n");    
    return SINQHM_ERR_WRONG_AXIS_MAPPING;
  }
    

  if (strobo_mode)
  {
    // bank 0, axis 2 is tof - axis
    axis_descr_ptr = getAxisDescription(0, 1);
    status = SetAxisMapping(axis_descr_ptr, &axis_1, DO_RANGE_CHECK);
    if (status<0) return status;
  }

  histoDataPtr=getHistDataPtr();
  if(!histoDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): can not get histo data pointer\n");
    return SINQHM_ERR_NO_HISTO_DATA_PTR;
  }
   
  frameDataPtr = (volatile uint32 *)rt_malloc( (axislength * sizeof(int)));
  if(!frameDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_hrpt_construct(): can not allocate Frame Data\n");
    return SINQHM_ERR_FIL_MALLOC_FAILED;
  }

  memset((void*)frameDataPtr,0,(axislength * sizeof(int)) );

  state = FRAME_WAIT;

  Lo_bin=0;
  if (axislength > HRPT_NUM_WIRES)
  {
    Hi_bin=HRPT_NUM_WIRES - 1;
  }
  else
  {
    Hi_bin=axislength - 1;
  }
  

  return SINQHM_OK;
}

/******************************************************************************/

void process_hrpt_init_daq(void)
{
  *cnt_low_ptr  = 0;
  *cnt_high_ptr = 0;

//  *(axis_0.cnt_low_ptr)  = 0;
//  *(axis_0.cnt_high_ptr) = 0;

  if (strobo_mode)
  {
    *(axis_1.cnt_low_ptr)  = 0;
    *(axis_1.cnt_high_ptr) = 0;
  }

  state = FRAME_WAIT;
  picture_length = 0;
  channel_count = 0;
  last_valid_processed_frame_num = 0xffffffff; 
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_hrpt_destruct
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

void process_hrpt_destruct(void)
{
  histoDataPtr = 0;

  if (frameDataPtr)	
  {
  	rt_free((void*)frameDataPtr);
    frameDataPtr=0;
  }	
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_packet_hrpt
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

void process_hrpt(packet_type *p)
{
  uint32 header_type;
  int  i,wire, binpos;
  uint32 timestamp;
  int strobopos;
  unsigned int error_flag;
  
  // print_packet(p);

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if (header_type == LWL_HM_NC_C3)
  {
    wire        = ((p->data[1]) & 0x0fff) >> 1;  
    error_flag  = !(p->data[1] & 0x01);   // Note: Error Bit  0: error occured  1: no error


    if (wire < Lo_bin)                           /* Out-of-range? */
    {
      UINT32_INC_CEIL(*cnt_low_ptr);
    }        
    else if (wire <= Hi_bin)
    {                                           
      if (wire == Lo_bin)                    /* Start of new frame? */
      {
        if (state != FRAME_WAIT)
        {
          dbg_printf(DBGMSG_ERROR, "process_hrpt(): incomplete Frame\n"); 
          shm_cfg_ptr[CFG_FIL_EVT_ERROR]++;
        }
  
        memset((void*)frameDataPtr,0,(axislength * sizeof(int)) );
        state         = FRAME_STARTED;                      /* Yes. Note our new state */
        frame_num     = p->data[3];
        skip_frame    = 0; 
        mon_count_low_frame_num = 0xffffffff;
        channel_count = 1;

      }   
      else 
      {
        if (state == FRAME_STARTED)                 /* Yes. Check state */
        {
          if (wire != channel_count)                /* And check frame # */
          {
            dbg_printf (DBGMSG_ERROR, "process_hrpt(): Wire sequence error: expected %d but got %d\n",channel_count,wire);
            state = FRAME_WAIT;
            shm_cfg_ptr[CFG_FIL_EVT_ERROR]++;
          }
          if (p->data[3] != frame_num)                /* And check frame # */
          {
            dbg_printf (DBGMSG_ERROR, "process_hrpt(): Frame sequence error: expected %d but got %d\n",frame_num,p->data[3]);
            state = FRAME_WAIT;
            shm_cfg_ptr[CFG_FIL_EVT_ERROR]++;
          }
          else
          {
            channel_count++;
          }
        }
      }
      
      frameDataPtr[wire] = (p->data[2]) & 0x03ff;
      skip_frame = skip_frame || ((p->data[0] & hdr_daq_mask) != hdr_daq_active) || error_flag;

      if ((wire == Hi_bin) && (state == FRAME_STARTED))   /* End of frame? */
      {
        if(channel_count != (Hi_bin+1))
        {
          dbg_printf(DBGMSG_ERROR, "process_hrpt(): incomplete Frame\n"); 
          shm_cfg_ptr[CFG_FIL_EVT_ERROR]++;
        } 
        else if (skip_frame)
        {
          // skip
          shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]++;
        }
        else
        {
          shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++;
          last_valid_processed_frame_num = frame_num;
          if(strobo_mode)
          {
            timestamp = p->data[0] & LWL_HDR_TS_MASK;
            strobopos = axis_1.fcn(timestamp, &axis_1);
            last_valid_processed_strobopos = strobopos;
            if (strobopos>=0)
            {
              for (i = 0; i <= Hi_bin; i++)
              {
                binpos = ((Hi_bin - i) * axis_1.len) + strobopos;   /* Reflect */  
                UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], frameDataPtr[i], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
              }
            }
            
          }
          else
          {
            for (i = 0; i <= Hi_bin; i++)
            {
              binpos = Hi_bin - i;     /* Reflect */
              UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], frameDataPtr[i], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
            }
          }


        }
        state = FRAME_WAIT;
      }

    }
    else if (wire == (HRPT_NUM_WIRES + 0))
    {
      // info currently not used
      picture_length = p->data[2];
    }                                                   
    else if (wire == (HRPT_NUM_WIRES + 1))
    {
      picture_length |= p->data[2] << 16;
      shm_cfg_ptr[CFG_FIL_PIC_LENGTH] = picture_length;
    }
    else if (wire == (HRPT_NUM_WIRES + 2))
    {
      monitor_count = p->data[2];  // lower 16 bit of monitor count
      mon_count_low_frame_num = p->data[3];
    }                                                   
    else if (wire == (HRPT_NUM_WIRES + 3))
    {
      frame_num     = p->data[3];      
      monitor_count |= p->data[2] << 16;
      if ( (!skip_frame)                           && 
           (axislength > HRPT_MONITOR_BIN)         &&
           (frame_num == mon_count_low_frame_num)  &&
           (frame_num == last_valid_processed_frame_num)
         )
      {
         if(strobo_mode)
          {
            if (last_valid_processed_strobopos>=0)
            {
                binpos = (HRPT_MONITOR_BIN * axis_1.len) + last_valid_processed_strobopos;  
                UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], monitor_count, shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
            }
          }
          else
          {
              binpos = HRPT_MONITOR_BIN; 
              UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], monitor_count, shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
          }
      }
    }    
    else // if (wire > Hi_bin)
    {
      UINT32_INC_CEIL(*cnt_high_ptr);
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
