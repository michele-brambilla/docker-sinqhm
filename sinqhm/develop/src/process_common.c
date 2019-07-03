/*******************************************************************************
  process_common.c
*******************************************************************************/


/*******************************************************************************
  includes
*******************************************************************************/

#include "controlshm.h"
#include "debugshm.h"
#include "datashm.h"
#include "rawdata.h"
#include "needfullthings.h"
#include "process_common.h"
#include "sinqhm_errors.h"


/*******************************************************************************
  global vars
*******************************************************************************/

extern  volatile unsigned int * shm_cfg_ptr ;
extern  volatile histo_descr_type *shm_histo_ptr;

/* function ptr for packet process function */
void (*process_packet_fcn)(packet_type*) = 0;
int  (*process_construct_fcn)(void) = 0;
void (*process_destruct_fcn)(void) = 0;
void (*process_init_daq_fcn)(void) = 0;

uint32  hdr_daq_mask   = 0;
uint32  hdr_daq_active = 0;


/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   searchBinBoundary
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

__INLINE int searchBinBoundary(unsigned int val, unsigned int *array, unsigned int arraySize)
{
  int diff,left,middle,right;

  left   = 0;
  right  = arraySize-1;
  middle = (left + right)/2;
  diff = right - left;

  while(diff)
  {
    if (diff==1)
    {
      middle = (val >= array[right]) ? right : left;
      return middle;
    }
    else
    {
      middle = (left + right)/2;
      if (val == array[middle])
      {
        return middle;
      }
      else if (val > array[middle])
      {
        left = middle;
      }
      else
      {
        right = middle;
      }
    }
    diff = right - left;
  }
  return middle;
}


/*******************************************************************************
 *
 * FUNCTION
 *   axismap_boundary
 *
 ******************************************************************************/

int axismap_boundary(int val, process_axis_type *axis)
{
  int tofpos;

  /* The AMOR fix */
  if(axis->threshold > 0 && val > axis->threshold){
    val -= axis->offset;
  }

  if (axis->rangecheck)
  {
    if (val < axis->data[0])
    {
      UINT32_INC_CEIL(*(axis->cnt_low_ptr));
      return SINQHM_ERR_VAL_LOW;
    }
    else if (val >= axis->data[axis->datalen-1])
    {
      UINT32_INC_CEIL(*(axis->cnt_high_ptr));
      return SINQHM_ERR_VAL_HIGH;
    }
  }



  tofpos = searchBinBoundary(val, axis->data, axis->datalen);

  if (axis->rangecheck && tofpos >= axis->len)
  {
    UINT32_INC_CEIL(*(axis->cnt_high_ptr));
    return SINQHM_ERR_VAL_HIGH;
  }

  return tofpos;
}


/*******************************************************************************
 *
 * FUNCTION
 *   axismap_direct, axismap_direct_rangecheck
 *
 ******************************************************************************/

int axismap_direct(int val, process_axis_type *axis)
{
  if (axis->rangecheck)
  {
    if (val < 0)
    {
      UINT32_INC_CEIL(*(axis->cnt_low_ptr));
      return SINQHM_ERR_VAL_LOW;
    }
    else if (val >= axis->len)
    {
      UINT32_INC_CEIL(*(axis->cnt_high_ptr));
      return SINQHM_ERR_VAL_HIGH;
    }
  }

  return val;
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:57:13 2017
int axismap_direct_rangecheck(int val, process_axis_type *axis) {

  if( (val >= 0) && (val < axis->len) ) {
    return val;
  }
  else {
    if ( val > 0 ) {
      UINT32_INC_CEIL(*(axis->cnt_high_ptr));
      return SINQHM_ERR_VAL_HIGH;
    }
  }
  UINT32_INC_CEIL(*(axis->cnt_low_ptr));
  return SINQHM_ERR_VAL_LOW;
}


/*******************************************************************************
 *
 * FUNCTION
 *   axismap_calc
 *
 ******************************************************************************/

int axismap_calc(int val, process_axis_type *axis)
{
  int temp;

  //axis->array[0] : Multiplier
  if (axis->array[0])
  {
    temp = val * (int)axis->array[0];
  }
  else
  {
    temp = val;
  }

  //axis->array[1] : PreOffset
  temp = temp + (int)axis->array[1];

  //axis->array[2] : Divisor
  if (axis->array[2])
  {
    temp = temp / (int)axis->array[2];
  }

  //axis->array[3] : PostOffset
  temp = temp + (int)axis->array[3];

  if (axis->rangecheck)
  {
    if (temp < 0)
    {
      UINT32_INC_CEIL(*(axis->cnt_low_ptr));
      return SINQHM_ERR_VAL_LOW;
    }
    else if (temp >= axis->len)
    {
      UINT32_INC_CEIL(*(axis->cnt_high_ptr));
      return SINQHM_ERR_VAL_HIGH;
    }
  }

  return temp;
}


/*******************************************************************************
 *
 * FUNCTION
 *   axismap_lookup
 *
 ******************************************************************************/

int axismap_lookup(int val, process_axis_type *axis)
{
  int lookup;

  if (val < 0)
  {
    UINT32_INC_CEIL(*(axis->cnt_low_ptr));
    return SINQHM_ERR_VAL_LOW;
  }
  else if (val >= axis->datalen)
  {
    UINT32_INC_CEIL(*(axis->cnt_high_ptr));
    return SINQHM_ERR_VAL_HIGH;
  }

  lookup = axis->data[val];

  if (axis->rangecheck)
  {
    if (lookup < 0)
    {
      UINT32_INC_CEIL(*(axis->cnt_low_ptr));
      return SINQHM_ERR_VAL_LOW;
    }
    else if (lookup >= axis->len)
    {
      UINT32_INC_CEIL(*(axis->cnt_high_ptr));
      return SINQHM_ERR_VAL_HIGH;
    }
  }

  return lookup;
}


//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:59:53 2017
/*******************************************************************************
 *
 * FUNCTION
 *   axismap_map
 *
 ******************************************************************************/
int axismap_map(int val, process_axis_type *axis)
{
  int pos;

  if (val < 0)
  {
    UINT32_INC_CEIL(*(axis->cnt_low_ptr));
    return SINQHM_ERR_VAL_LOW;
  }
  else if (val >= axis->datalen)
  {
    UINT32_INC_CEIL(*(axis->cnt_high_ptr));
    return SINQHM_ERR_VAL_HIGH;
  }

  pos = axis->data[val];

  if (axis->rangecheck)
  {
    if (pos < 0)
    {
      UINT32_INC_CEIL(*(axis->cnt_low_ptr));
      return SINQHM_ERR_VAL_LOW;
    }
    else if (pos >= axis->len)
    {
      UINT32_INC_CEIL(*(axis->cnt_high_ptr));
      return SINQHM_ERR_VAL_HIGH;
    }
  }
  return pos;
}


/*******************************************************************************
 *
 * FUNCTION
 *   SetAxisMapping
 *
 ******************************************************************************/

int SetAxisMapping(volatile axis_descr_type  *axis_descr_ptr, process_axis_type *axis, unsigned int rangecheck)
{
  uint32 mapping;
  int i, datalen;

  if (!axis_descr_ptr)
  {
    axis->cnt_low_ptr  = 0;
    axis->cnt_high_ptr = 0;
    return SINQHM_ERR_NO_AXIS_DESCR_PTR;
  }

  axis->rangecheck   = rangecheck;
  axis->cnt_low_ptr  = &(axis_descr_ptr->cnt_low);
  axis->cnt_high_ptr = &(axis_descr_ptr->cnt_high);

  *(axis->cnt_low_ptr)  = 0;
  *(axis->cnt_high_ptr) = 0;

  mapping = axis_descr_ptr->type;
  switch(mapping)
  {
    case AXDIRECT:
      axis->fcn      = axismap_direct_rangecheck;
      axis->len      = axis_descr_ptr->length;
      axis->array    = 0;
      axis->arraylen = 0;
      axis->data     = 0;
      axis->datalen  = 0;
      break;

    case AXCALC:
      axis->fcn      = axismap_calc;
      axis->len      = axis_descr_ptr->length;
      axis->array    = (uint32*)getAxisData(axis_descr_ptr);
      axis->arraylen = 4;
      axis->data     = (uint32*)getAxisData(axis_descr_ptr);
      axis->datalen  = 4;
      break;

    case AXBOUNDARY:
      axis->fcn       = axismap_boundary;
      axis->len       = axis_descr_ptr->length;
      axis->threshold = axis_descr_ptr->threshold;
      axis->offset    = axis_descr_ptr->offset;
      axis->array     = (uint32*)getAxisData(axis_descr_ptr);

      datalen = axis->array[1];
      for(i = 1; i < axis->array[0]; i++)
      {
        datalen *= axis->array[i+1];
      }
      axis->arraylen = datalen + 1 + axis->array[0];
      axis->data     = axis->array + 1 + axis->array[0];;
      axis->datalen  = datalen;
      break;

    case AXLOOKUP:
      axis->fcn      = axismap_lookup;
      axis->len      = axis_descr_ptr->length;
      axis->array    = (uint32*)getAxisData(axis_descr_ptr);

      datalen = axis->array[1];
      for(i = 1; i < axis->array[0]; i++)
      {
        datalen *= axis->array[i+1];
      }
      axis->arraylen = datalen + 1 + axis->array[0];
      axis->data     = axis->array + 1 + axis->array[0];;
      axis->datalen  = datalen;
      break;

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 12:01:08 2017
  case AXMAP:

    axis->fcn      = axismap_map;
    axis->len      = axis_descr_ptr->length;
    axis->len      = axis_descr_ptr->length;
    axis->array    = (uint*)getAxisData(axis_descr_ptr);

    datalen = axis->array[1];
    for(i = 1; i < axis->array[0]; i++) {
      datalen *= axis->array[i+1];
    }
    axis->arraylen = datalen + 1 + axis->array[0];
    axis->data     = axis->array + 1 + axis->array[0];;
    axis->datalen  = datalen;

    break;

    default:
      dbg_printf(DBGMSG_ERROR, "SetAxisMapping(): Unknow Mapping\n");
      return SINQHM_ERR_UNKNOWN_AXIS_MAPPING;
      break;

  }
  return SINQHM_OK;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_construct
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

int process_construct(void)
{
  int status;
  print_config();

  shm_histo_ptr->filler_valid = 0;
//    dbg_printf(DBGMSG_ERROR, "process_construct(): TEST BLA\n");

  if (shm_histo_ptr->server_valid != DATASHM_CFG_SRV_VALID)
  {
    dbg_printf(DBGMSG_ERROR, "process_construct(): No valid Histo Configuration\n");
    return SINQHM_ERR_NO_VALID_CONFIG;
  }

  hdr_daq_mask   =  shm_cfg_ptr[CFG_SRV_HDR_DAQ_MASK];
  hdr_daq_active =  shm_cfg_ptr[CFG_SRV_HDR_DAQ_ACTIVE];

  dbg_printf(DBGMSG_INFO2, "process_construct(): hdr_daq_mask=0x%08x\n",hdr_daq_mask);
  dbg_printf(DBGMSG_INFO2, "process_construct(): hdr_daq_active=0x%08x\n",hdr_daq_active);

  if (!hdr_daq_mask)
  {
    dbg_printf(DBGMSG_WARNING, "process_construct(): Header DAQ Mask is not set!\n");
  }


  switch(shm_histo_ptr->histo_type)
  {
    case FILLERTOF:
        status = process_tof_construct();
      break;

    case FILLERTOFMAP:
        status = process_tofmap_construct();
      break;

    case FILLERPSD:
        status = process_psd_construct();
      break;

    case FILLERDIG:
        status = process_dig_construct();
      break;

    case FILLERHRPT:
        status = process_hrpt_construct();
      break;

    case FILLERSANS2:
        status = process_sans2_construct();
      break;

    case FILLERRITA2:
      status = process_rita2_construct();
      status = SINQHM_OK;  // fake
      break;

    case FILLERCAMEA:
      status = process_camea_construct();
      status = SINQHM_OK;  // fake
      break;

    default:
      dbg_printf(DBGMSG_ERROR, "process_construct(): Unknown Filler Type\n");
      return SINQHM_ERR_NO_VALID_CONFIG;
      break;

  }

  if (status>=0)
  {
    shm_histo_ptr->filler_valid = DATASHM_CFG_FIL_VALID;
  }

  return status;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_init_daq
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

void process_init_daq(void)
{
  RawDataInit();

  if (process_init_daq_fcn)
  {
    process_init_daq_fcn();
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   process_leave_daq
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

void process_leave_daq(void)
{
  RawDataStop();
}

/*******************************************************************************
 *
 * FUNCTION
 *   process_destruct
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

void process_destruct(void)
{

  shm_histo_ptr->filler_valid = 0;

  if (process_destruct_fcn)
  {
    process_destruct_fcn();
  }

  process_packet_fcn   = 0;
  process_init_daq_fcn = 0;
  process_destruct_fcn = 0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   process_tsi
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

int process_tsi(packet_type *p)
{
  uint32 header_type;
  uint32 hdr;


  hdr = p->data[0];
  header_type = hdr & LWL_HDR_TYPE_MASK;

  if (    (header_type != LWL_HEADER_TSI_L2)
       && (header_type != LWL_HEADER_TSI_L4)
       && (header_type != LWL_HEADER_TSI_SANS2) )
  {
    return 0;
  }

  // Rate Information in SANS2 TSI
  // set rate information independent from DAQ state
  if (header_type == LWL_HEADER_TSI_SANS2)
  {
    shm_cfg_ptr[CFG_FIL_COINC_TIME]  =   p->data[0] & LWL_HDR_TS_MASK;
    shm_cfg_ptr[CFG_FIL_RATE_VALID]  =  (p->data[2] | ((p->data[1]&0x0f0)<<12) );
    shm_cfg_ptr[CFG_FIL_RATE_AN]     =  (p->data[3] | ((p->data[1]&0x00f)<<16) );
    shm_cfg_ptr[CFG_FIL_RATE_KV]     =   p->data[4];
    shm_cfg_ptr[CFG_FIL_RATE_KH]     =   p->data[5];
  }

  UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_COUNT]);
  UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_COUNT]);

  shm_cfg_ptr[CFG_FIL_TSI_HEADER] = hdr;
  shm_cfg_ptr[CFG_FIL_TSI_DATA]   = p->data[1];


  if (shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] )        // DAQ STARTED
  {
    if (!shm_cfg_ptr[CFG_SRV_DAQ_PAUSE_CMD])   // not Paused
    {
      UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_STARTED]);
      UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_DAQ_STARTED]);

      if ((hdr & hdr_daq_mask) == hdr_daq_active) UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_ACTIVE]);

      if (hdr & LWL_HDR_SYNC0_MASK) UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC0]);
      if (hdr & LWL_HDR_SYNC1_MASK) UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC1]);
      if (hdr & LWL_HDR_SYNC2_MASK) UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC2]);
      if (hdr & LWL_HDR_SYNC3_MASK) UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC3]);

      // Information from TDC
      if (header_type == LWL_HEADER_TSI_L4)
      {
        UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_PSD_HITS],   p->data[2]                       );
        UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_PSD_FLASH], (p->data[3] & LWL_PSD_FLASH_MASK) );
      }

    }
    else  // Paused
    {
      UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_PAUSED]);
      UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_DAQ_PAUSED]);
    }
  }
  else
  {
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_DAQ_STOPPED]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_DAQ_STOPPED]);
  }

  if (hdr & LWL_HDR_SWC_MASK)
  {
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_STATUS_WORD_CHANGED]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_STATUS_WORD_CHANGED]);
  }

  // Information from TDC
  if (header_type == LWL_HEADER_TSI_L4)
  {
    UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PSD_HITS],   p->data[2]                       );
    UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PSD_FLASH], (p->data[3] & LWL_PSD_FLASH_MASK) );
  }


#if 0
  if (hdr & LWL_HDR_NRL_MASK)
  {
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_RATE_LOW]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_RATE_LOW]);
  }

  if (hdr & LWL_HDR_PF_MASK)
  {
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_TSI_POWER_FAIL]);
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_SUM_TSI_POWER_FAIL]);
  }
#endif

  return 1;
}


/*******************************************************************************
 *
 * FUNCTION
 *   mask_shift
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
uint mask_shift(uint value, uint mask, uint shift) {
  return (value & mask) >> shift;
}
/******************************************************************************/
