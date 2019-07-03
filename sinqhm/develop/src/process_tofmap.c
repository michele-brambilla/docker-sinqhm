/*******************************************************************************
  process_tofmap.c
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

// axis 0 is tube number
static process_axis_type axis_0;

// axis 1 is position in tube
static process_axis_type axis_1;

// axis 2 is Time Of Flight 
static process_axis_type axis_2;

static int whole_inc = 1;
static int b0_rank;
static int axis_1_lookup = 0;

/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_tofmap_init_daq(void);
void process_tofmap_destruct(void);
void process_tofmap(packet_type *p);


/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_tofmap_construct
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

int process_tofmap_construct(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  int status;

  histo_descr_ptr = getShmHistoPtr();


  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_tofmap_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }

  if (histo_descr_ptr->nBank != 1)
  {
    dbg_printf(DBGMSG_ERROR, "process_tofmap_construct(): nbank must be 1 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_BANKS;
  }

  bank_descr_ptr = getBankDescription(0);
  if(!bank_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_tofmap_construct(): can not get bank description pointer\n");
    return SINQHM_ERR_NO_BANK_DESCR_PTR;
  }

  b0_rank = bank_descr_ptr->rank;
  if ((b0_rank != 2) && (b0_rank != 3))
  {
    dbg_printf(DBGMSG_ERROR, "process_tofmap_construct(): naxis must be 2 or 3 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
  }

  whole_inc = histo_descr_ptr->increment;

  process_packet_fcn   = process_tofmap;
  process_init_daq_fcn = process_tofmap_init_daq;
  process_destruct_fcn = process_tofmap_destruct;

  // bank 0, axis 0 is tube - axis
  axis_descr_ptr = getAxisDescription(0, 0);
  status = SetAxisMapping(axis_descr_ptr, &axis_0, DO_RANGE_CHECK);
  if (status<0) return status;

  // bank 0, axis 1 is position inside a tube
  axis_descr_ptr = getAxisDescription(0, 1);
  // Mapping type for axis 1 must be AXLOOKUP
  if (axis_descr_ptr->type == AXLOOKUP)
  {
    axis_1_lookup = 1;
    status = SetAxisMapping(axis_descr_ptr, &axis_1, NO_CHECK);
    if (status<0) return status;
  }
  else
  {
    axis_1_lookup = 0;
    status = SetAxisMapping(axis_descr_ptr, &axis_1, DO_RANGE_CHECK);
    if (status<0) return status;
  }

  if (b0_rank > 2) 
  {
    // bank 0, axis 2 is tof - axis
    axis_descr_ptr = getAxisDescription(0, 2);
    status = SetAxisMapping(axis_descr_ptr, &axis_2, DO_RANGE_CHECK);
    if (status<0) return status;
  }

  histoDataPtr=getHistDataPtr();
  if(!histoDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_tofmap_construct(): can not get histo data pointer\n");
    return SINQHM_ERR_NO_HISTO_DATA_PTR;
  }

  return SINQHM_OK;
}

/******************************************************************************/

void process_tofmap_init_daq(void)
{
  *(axis_0.cnt_low_ptr)  = 0;
  *(axis_0.cnt_high_ptr) = 0;

  *(axis_1.cnt_low_ptr)  = 0;
  *(axis_1.cnt_high_ptr) = 0;

  if (b0_rank > 2) 
  {
    *(axis_2.cnt_low_ptr)  = 0;
    *(axis_2.cnt_high_ptr) = 0;
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   process_tofmap_destruct
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

void process_tofmap_destruct(void)
{
  histoDataPtr = 0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_tofmap
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

void process_tofmap(packet_type *p)
{
  uint32  header_type;
  uint32  timestamp,binpos;
  int32   tube, channel, tofpos, pos;
  uint32  data, lookupidx, pos_inc, inc;
   
//  print_packet(p);

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if ((header_type >= LWL_TOF_C1) && (header_type <= LWL_TOF_C10))
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

      tube = (p->data[1] & LWL_TUBE_MASK) >> LWL_TUBE_SHIFT;
      data = (p->data[1] & LWL_DATA_MASK) >> LWL_DATA_SHIFT;

      if (tube != LWL_TUBE_ERROR)
      {

      }


#if 0
      {
        unsigned int adr, bits_set, bit_pos, test_pos, ds;

        data = p->data[1] & 0x1ff;

        adr = p->data[1] >> 9;
        ds  = (p->data[0] >> 24) & 0xf;

        dbg_printf(DBGMSG_INFO5,"ds=%d  adr=%d (0x%02x)\n",ds,adr,adr);

        bits_set = 0;
        bit_pos  = 0;
        test_pos = 1;

        while(adr)
        {
          if (adr & 0x01)
          {
            bits_set++;
            bit_pos  = test_pos;
          }
          adr = adr >> 1;
          test_pos ++;
        }

        if (bits_set > 1)        
        {
          // more than one bits set
          shm_cfg_ptr[CFG_FIL_TEST_1]++;
          dbg_printf(DBGMSG_INFO5,"More than one bit set\n");
          return;
        }

        tube = bit_pos*10 + ds - 1;
      }
#endif


      dbg_printf(DBGMSG_INFO5,"tube=%d data=%d\n",tube,data);

      channel = axis_0.fcn(tube, &axis_0);
//      dbg_printf(DBGMSG_INFO5,"channel=%d \n",channel);

      if (channel>=0)
      {
        if(axis_1_lookup)
        {
          lookupidx = tube*axis_1.array[2] + data;
  //      dbg_printf(DBGMSG_INFO5,"lookupidx=%d \n",lookupidx);

          pos_inc = axis_1.fcn(lookupidx, &axis_1);
  //      dbg_printf(DBGMSG_INFO5,"pos_inc=%d \n",pos_inc);

          pos = pos_inc >> 16;
          inc = pos_inc & 0x0ffff;
        }
        else
        {
          inc = whole_inc;
          pos = axis_1.fcn(data, &axis_1);
          if (pos<0) return;
        }


//        dbg_printf(DBGMSG_INFO5,"pos=%d inc=%d\n",pos,inc);
        if (pos>=axis_1.len)
        {
//            dbg_printf(DBGMSG_INFO5,"process_tofmap_construct(): pos=%d\n",pos);
            (*(axis_1.cnt_high_ptr))++;
        }
        else
        { 
          if (b0_rank<3)
          {
            // No Tof needed;
            binpos = (channel * axis_1.len) + pos;

            UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], inc, shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
//            dbg_printf(DBGMSG_INFO5,"histoDataPtr[%d]=%d\n",binpos,histoDataPtr[binpos]);
            if ((inc<whole_inc) && ((channel+1)<axis_0.len))
            {
              UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos+(axis_1.len)], (whole_inc-inc), shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
            }
          }
          else
          {
            timestamp = p->data[0] & LWL_HDR_TS_MASK;
  //          dbg_printf(DBGMSG_INFO5,"timestamp=%d --",timestamp);
            tofpos = axis_2.fcn(timestamp, &axis_2);
  //            dbg_printf(DBGMSG_INFO5,"tofpos=%d\n",tofpos);
            if (tofpos>=0)
            {
//              dbg_printf(DBGMSG_INFO5,"channel=%d tofpos=%d\n",channel,tofpos);
              binpos = (channel * axis_2.len * axis_1.len) + (pos * axis_2.len) + tofpos ;
              UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos], inc, shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
//              dbg_printf(DBGMSG_INFO5,"histoDataPtr[%d]=%d\n",binpos,histoDataPtr[binpos]);
              if ((inc<whole_inc) && ((channel+1)<axis_0.len))
              {
                UINT32_INC_X_CEIL_CNT_OVL(histoDataPtr[binpos+(axis_2.len * axis_1.len)], (whole_inc-inc), shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
              }
            }

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
