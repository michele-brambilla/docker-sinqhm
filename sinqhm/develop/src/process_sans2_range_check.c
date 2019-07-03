/*******************************************************************************
  process_sans2.c
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
static volatile uint32 *bank2_HistoDataPtr = 0;

static int num_banks = 0;
// bank 0
// axis 0 is X axis
static process_axis_type b0_axis_0;

// axis 1 is Y axis
static process_axis_type b0_axis_1;


// bank 1
// axis 0 is X axis
static process_axis_type b1_axis_0;

// axis 1 is Y axis
static process_axis_type b1_axis_1;


// bank 2
// axis 0 is X axis
static process_axis_type b2_axis_0;

// axis 1 is Y axis
static process_axis_type b2_axis_1;





/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_sans2_init_daq(void);
void process_sans2(packet_type *p);
void process_sans2_destruct(void);

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_sans2_construct
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

int process_sans2_construct(void)
{
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  int status;

  histo_descr_ptr = getShmHistoPtr();

  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }


  num_banks = histo_descr_ptr->nBank;

  if ((num_banks < 1) || (num_banks > 3))
  {
    dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): nbank must be 1, 2 or 3 for this filler type\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_BANKS;
  }

  process_packet_fcn   = process_sans2;
  process_init_daq_fcn = process_sans2_init_daq;
  process_destruct_fcn = process_sans2_destruct;

/*  Bank 0 */
  bank_descr_ptr = getBankDescription(0);
  if(!bank_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get bank 0 description pointer\n");
    return SINQHM_ERR_NO_BANK_DESCR_PTR;
  }

  if (bank_descr_ptr->rank != 2)
  {
    dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): naxis must be 2 for bank 0\n");
    return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
  }

  // bank 0, axis 0 is x - axis
  axis_descr_ptr = getAxisDescription(0, 0);
  status = SetAxisMapping(axis_descr_ptr, &b0_axis_0, DO_RANGE_CHECK);
  if (status<0) return status;

  // bank 0, axis 1 is y - axis
  axis_descr_ptr = getAxisDescription(0, 1);
  status = SetAxisMapping(axis_descr_ptr, &b0_axis_1, DO_RANGE_CHECK);
  if (status<0) return status;

  bank0_HistoDataPtr = getBankData(0);
  if(!bank0_HistoDataPtr)
  {
    dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get bank 0 data pointer\n");
    return SINQHM_ERR_NO_BANK_DATA_PTR;
  }


/*  Bank 1 */
  if (num_banks >= 2)
  {
      bank_descr_ptr = getBankDescription(1);
      if(!bank_descr_ptr)
      {
        dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get bank 1 description pointer\n");
        return SINQHM_ERR_NO_BANK_DESCR_PTR;
      }

      if (bank_descr_ptr->rank != 2)
      {
        dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): naxis must be 2 for bank 1\n");
        return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
      }

      // bank 1, axis 0 is x - axis
      axis_descr_ptr = getAxisDescription(1, 0);
      status = SetAxisMapping(axis_descr_ptr, &b1_axis_0, DO_RANGE_CHECK);
      if (status<0) return status;

      // bank 1, axis 1 is y - axis
      axis_descr_ptr = getAxisDescription(1, 1);
      status = SetAxisMapping(axis_descr_ptr, &b1_axis_1, DO_RANGE_CHECK);
      if (status<0) return status;

      bank1_HistoDataPtr = getBankData(1);
      if(!bank1_HistoDataPtr)
      {
        dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get bank 1 data pointer\n");
        return SINQHM_ERR_NO_BANK_DATA_PTR;
      }

  }


/*  Bank 2 */
  if (num_banks >= 3)
  {
      bank_descr_ptr = getBankDescription(2);
      if(!bank_descr_ptr)
      {
        dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get bank 2 description pointer\n");
        return SINQHM_ERR_NO_BANK_DESCR_PTR;
      }

      if (bank_descr_ptr->rank != 2)
      {
        dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): naxis must be 2 for bank 2\n");
        return SINQHM_ERR_WRONG_NUMBER_OF_AXIS;
      }

      // bank 0, axis 0 is x - axis
      axis_descr_ptr = getAxisDescription(2, 0);
      status = SetAxisMapping(axis_descr_ptr, &b2_axis_0, DO_RANGE_CHECK);
      if (status<0) return status;

      // bank 0, axis 1 is y - axis
      axis_descr_ptr = getAxisDescription(2, 1);
      status = SetAxisMapping(axis_descr_ptr, &b2_axis_1, DO_RANGE_CHECK);
      if (status<0) return status;

      bank2_HistoDataPtr = getBankData(2);
      if(!bank2_HistoDataPtr)
      {
        dbg_printf(DBGMSG_ERROR, "process_sans2_construct(): can not get bank 2 data pointer\n");
        return SINQHM_ERR_NO_BANK_DATA_PTR;
      }
  }



  return SINQHM_OK;
}

/******************************************************************************/

void process_sans2_init_daq(void)
{
  *(b0_axis_0.cnt_low_ptr)  = 0;
  *(b0_axis_0.cnt_high_ptr) = 0;

  *(b0_axis_1.cnt_low_ptr)  = 0;
  *(b0_axis_1.cnt_high_ptr) = 0;

  if (num_banks > 1)
  {
    *(b1_axis_0.cnt_low_ptr)  = 0;
    *(b1_axis_0.cnt_high_ptr) = 0;

    *(b1_axis_1.cnt_low_ptr)  = 0;
    *(b1_axis_1.cnt_high_ptr) = 0;
  }

  if (num_banks > 2)
  {
    *(b2_axis_0.cnt_low_ptr)  = 0;
    *(b2_axis_0.cnt_high_ptr) = 0;

    *(b2_axis_1.cnt_low_ptr)  = 0;
    *(b2_axis_1.cnt_high_ptr) = 0;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_sans2_destruct
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

void process_sans2_destruct(void)
{
  bank0_HistoDataPtr = 0;
  bank1_HistoDataPtr = 0;
  bank2_HistoDataPtr = 0;

}


/*******************************************************************************
 *
 * FUNCTION
 *   process_packet_sans2
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

void process_sans2(packet_type *p)
{
  uint32 header_type;
  uint32 timestamp,binpos;
  int32  xPos, yPos, pos_kv, pos_kh, pos_an, kv_valid, kh_valid, an_valid, pos_k_avg, k_diff;
  int32  b0_valid,b1_valid,b2_valid;

  header_type = p->data[0] & LWL_HDR_TYPE_MASK;

  if (header_type == LWL_SANS2_DATA)
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

      pos_an = p->data[0] & 0x01ff;
      pos_kv = p->data[1] & 0x01ff;
      pos_kh = p->data[2] & 0x01ff;

//      kv_valid = ( (p->data[1] & 0x8000) && (pos_kv > 135) && (pos_kv < 385) );
//      kh_valid = ( (p->data[2] & 0x8000) && (pos_kh > 135) && (pos_kh < 385) );
//      an_valid = ( (pos_an > 100) && (pos_an < 420) );
      kv_valid = (p->data[1] & 0x8000);
      kh_valid = (p->data[2] & 0x8000);
      an_valid = 1;

// bank 0
      k_diff = 9999;
      b0_valid=0;
      b1_valid=0;
      b2_valid=0;

      if (an_valid && kv_valid && kh_valid)
      {
        k_diff = pos_kv - pos_kh;

        if ( (k_diff > -20) && (k_diff < 20) ) 
        {
          xPos = b0_axis_0.fcn(pos_an, &b0_axis_0);
          if(xPos >= 0 )
          {
            pos_k_avg = (pos_kv + pos_kh) >> 1; 
            yPos = b0_axis_1.fcn(pos_k_avg, &b0_axis_1);

            if(yPos >= 0 )
            {
              binpos = (xPos * b0_axis_1.len) + yPos ;
              UINT32_INC_CEIL_CNT_OVL(bank0_HistoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
              b0_valid=1;
            }
          }

        }
      }


      if ((num_banks > 1) && (k_diff != 9999))
      {
        int xd, yd, p;

        UINT32_INC_CEIL_CNT_OVL(bank1_HistoDataPtr[512+256+k_diff], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if ((b0_valid) && (yPos>150) && (yPos<360) && (xPos>110) && (xPos<410))
        {
          xd = (xPos-110) / 60;
          yd = (yPos-150) / 42;

          p = 1 + 5*yd + xd;
          UINT32_INC_CEIL_CNT_OVL(bank1_HistoDataPtr[(p*2*512)+512+256+k_diff], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        }
      }




// bank 1
      if ((num_banks > 1) && (an_valid && kv_valid))
      {
        xPos = b1_axis_0.fcn(pos_an, &b1_axis_0);
        if(xPos >= 60 )   // leave space for kdiff statistics
        {
          yPos = b1_axis_1.fcn(pos_kv, &b1_axis_1);

          if(yPos >= 0 )
          {
            binpos = (xPos * b1_axis_1.len) + yPos ;
            UINT32_INC_CEIL_CNT_OVL(bank1_HistoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
            b1_valid=1;
          }
        }
      }

// bank 2
      if ((num_banks > 2) && (an_valid && kh_valid))
      {
        xPos = b2_axis_0.fcn(pos_an, &b2_axis_0);
        if(xPos >= 1 )  // leave space for statistics
        {
          yPos = b2_axis_1.fcn(pos_kh, &b2_axis_1);

          if(yPos >= 0 )
          {
            binpos = (xPos * b2_axis_1.len) + yPos ;
            UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[binpos], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
            b2_valid=1;
          }
        }
      }




      if (num_banks > 2) 
      {
        int tmp;
        // total 
        UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[0], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        // valid
        if (b0_valid) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[1], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (b1_valid) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[2], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (b2_valid) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[3], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        // Error Counts
        if ( kv_valid && !kh_valid) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[4], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (!kv_valid &&  kh_valid) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[5], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (!an_valid ) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[6], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (!kv_valid ) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[7], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (!kh_valid ) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[8], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (pos_an > 100) && (pos_an < 420) );
        if (pos_an == 0)     UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[ 9], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
        if (pos_an == 0x1ff) UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[10], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (p->data[1] & 0x8000) );
        if (tmp)             UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[11], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (p->data[1] & 0x8000) && (pos_kv == 0) );
        if (tmp)             UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[12], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (p->data[1] & 0x8000) && (pos_kv == 0x1ff) );
        if (tmp)             UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[13], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (p->data[2] & 0x8000) );
        if (tmp)             UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[14], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (p->data[2] & 0x8000) && (pos_kh == 0) );
        if (tmp)             UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[15], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

        tmp = ( (p->data[2] & 0x8000) && (pos_kh == 0x1ff) );
        if (tmp)             UINT32_INC_CEIL_CNT_OVL(bank2_HistoDataPtr[16], shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
      }



    } // daq active

  }
  else if (!process_tsi(p))
  {
    // unknown packet
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_PKG_UNKNOWN]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PKG_UNKNOWN]);
  }

}
