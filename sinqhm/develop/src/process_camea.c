/*******************************************************************************
  includes
*******************************************************************************/

#ifdef FILLER_RTAI
#include <rtai_malloc.h>
#else
#include <stdlib.h>
#define rt_malloc malloc
#define rt_free free
#endif

#include <stdio.h>
#include <unistd.h>

#include "zeromq.h"

#include "controlshm.h"
#include "datashm.h"
#include "debugshm.h"
#include "needfullthings.h"
#include "process_common.h"
#include "sinqhm_errors.h"

#include "sinqhmlog.h"

/*******************************************************************************
  global vars
*******************************************************************************/

extern volatile unsigned int *shm_cfg_ptr;

static volatile uint *histoDataPtr = 0;
static uint nbank = 0;

char message[1024];
/******************************
 vars for process_camea
*****************************/

typedef struct {
  int rank;
  volatile uint *data;
  process_axis_type *axis;
} banks_type;

static banks_type *banks = 0;

/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_camea_init_daq(void);
void process_camea(packet_type *p);
void process_camea_destruct(void);

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_camea_construct
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

int process_camea_construct(void) {
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type *bank_descr_ptr;
  volatile axis_descr_type *axis_descr_ptr;

  int status;
  uint axis, bank;

  histo_descr_ptr = getShmHistoPtr();
  if (!histo_descr_ptr) {
    dbg_printf(DBGMSG_ERROR,
               "process_camea_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }
  nbank = histo_descr_ptr->nBank;

  // Make sure that only one bank is present
  assert(nbank == 1);

  histoDataPtr = getHistDataPtr();
  if (!histoDataPtr) {
    dbg_printf(DBGMSG_ERROR,
               "process_camea_construct(): can not get histo data pointer\n");
    return SINQHM_ERR_NO_HISTO_DATA_PTR;
  }

  process_init_daq_fcn = process_camea_init_daq;
  process_destruct_fcn = process_camea_destruct;
  process_packet_fcn = process_camea;

  banks = (banks_type *)rt_malloc((nbank * sizeof(banks_type)));
  if (!banks) {
    dbg_printf(
        DBGMSG_ERROR,
        "process_camea_construct(): can not allocate memory for banks array\n");
    return SINQHM_ERR_FIL_MALLOC_FAILED;
  }
  memset((void *)banks, 0, (nbank * sizeof(banks_type)));

  for (bank = 0; bank < nbank; bank++) {
    bank_descr_ptr = getBankDescription(bank);
    if (!bank_descr_ptr) {
      dbg_printf(DBGMSG_ERROR,
                 "process_camea_construct(): can not get bank description "
                 "pointer for bank %d\n",
                 bank);
      process_camea_destruct();
      return SINQHM_ERR_NO_BANK_DESCR_PTR;
    }

    banks[bank].rank = bank_descr_ptr->rank;
    banks[bank].data = getBankData(bank);
    if (!banks[bank].data) {
      dbg_printf(DBGMSG_ERROR,
                 "process_camea_construct(): can not get bank data pointer\n");
      return SINQHM_ERR_NO_BANK_DATA_PTR;
    }

    banks[bank].axis = (process_axis_type *)rt_malloc(
        (bank_descr_ptr->rank * sizeof(process_axis_type)));
    if (!banks[bank].axis) {
      dbg_printf(
          DBGMSG_ERROR,
          "process_camea_construct(): can not allocate memory for axis\n");
      return -108;
    }
    memset((void *)banks[bank].axis, 0,
           (bank_descr_ptr->rank * sizeof(process_axis_type)));

    for (axis = 0; axis < banks[bank].rank; axis++) {
      axis_descr_ptr = getAxisDescription(bank, axis);
      status = SetAxisMapping(axis_descr_ptr, &(banks[bank].axis[axis]),
                              DO_RANGE_CHECK);
      if (status < 0) {
        process_camea_destruct();
        return status;
      }
    }
  }

  return SINQHM_OK;
}

/******************************************************************************/

void process_camea_init_daq(void) {
  int bank, axis;

  assert(nbank > 0);
  if (banks) {
    for (bank = 0; bank < nbank; bank++) {
      for (axis = 0; axis < banks[bank].rank; axis++) {
        *(banks[bank].axis[axis].cnt_high_ptr) = 0;
        *(banks[bank].axis[axis].cnt_low_ptr) = 0;
      }
    }
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   process_camea_destruct
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

void process_camea_destruct(void) {
  uint bank;

  histoDataPtr = 0;

  if ((nbank > 1) && banks) {
    for (bank = 0; bank < nbank; bank++) {
      if (banks[bank].axis) {
        rt_free((void *)banks[bank].axis);
      }
    }
    rt_free((void *)banks);
    banks = 0;
  }
}

  /*******************************************************************************
   *
   * FUNCTION
   *   process_packet_camea
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

#define MAX_NO_AXIS 5

int verify_timestamp(uint32_t timestamp) {

  if (timestamp < 0) {
    dbg_printf(DBGMSG_ERROR, "Negative timestamp: event skipped\n");
    mongohm_log(DBGMSG_ERROR, "sys", "Event with negative timestamp");
    return -1;
  } else {
    if (timestamp > MAX_TIMESTAMP) {
      dbg_printf(DBGMSG_ERROR,
                 "Timestamp out of range: event skipped (%ld,%ld)\n", timestamp,
                 MAX_TIMESTAMP);
      mongohm_log(DBGMSG_ERROR, "sys", "Event with timestamp out of range");
      return -1;
    }
  }
  return 0;
}

int position_valid(int *position, uint *axis_length) {
  if ((*position) < 0) {
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_LOW]);
    mongohm_log(DBGMSG_ERROR, "sys", "position lower than threshold");
    return -1;
  }
  if ((*position) >= (*axis_length)) {
    mongohm_log(DBGMSG_ERROR, "sys", "position greater than threshold");
    UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_HIGH]);
    return -1;
  }
  return 0;
}

void process_camea(packet_type *p) {
  uint event;
  uint32_t timestamp, multiplier;
  uint binpos, tpos, axlen[MAX_NO_AXIS];
  int val[2], pos, bank, axis, err;

  bank = 0;
  for (axis = 0; axis < banks[bank].rank; ++axis) {
    axlen[axis] = getAxisDescription(0, axis)->length;
  }

  for (event = 0; event < 2 * (p->length); event++) {
    timestamp = p->data[event];
    if (verify_timestamp(timestamp) == -1) {
      continue;
    }

    event++;
    shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++;
    val[0] = mask_shift(p->data[event], 0x0fff0000, 16);
    val[1] = mask_shift(p->data[event], 0x0000ffff, 0);
    printf("tub: %u\tpos: %u\n", val[1], val[0]);
    val[1] /= 64;

    binpos = 0;
    multiplier = 1;
    for (axis = 0; axis < banks[bank].rank - 1; ++axis) { // skip timestamp
      pos = banks[bank].axis[axis].fcn(val[axis], &banks[bank].axis[axis]);
      err = position_valid(&pos, &axlen[axis]);
      if (err != 0) {
        break;
      }

      binpos += multiplier * pos;
      multiplier *= axlen[axis];
      printf("\taxis: %d\tmult: %d\tpos: %d\tbinpos: %d\n", axis, multiplier,
             pos, binpos);
    }
    printf("\tbin: %u\t-> %u\n\n", binpos, val[1] * 1024 + val[0]);
    if (err != 0) {
      break;
    }

    /* timestamp */
    axis = banks[bank].rank - 1;
    tpos = banks[bank].axis[axis].fcn(timestamp, &banks[bank].axis[axis]);

    err = position_valid(&tpos, &axlen[axis]);
    if (err != 0)
      break;

    /* Only if bot x,y and timestamp makes sense data is stored */
    UINT32_INC_CEIL_CNT_OVL(banks[bank].data[binpos],
                            shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
    UINT32_INC_CEIL_CNT_OVL(banks[bank].data[multiplier + tpos],
                            shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
  }
}
