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

#include "process_rita2.h"
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
 vars for process_rita2
*****************************/

/******************************
 vars for process_rita2_mb
*****************************/

typedef struct {
  int rank;
  volatile uint *data;
  process_axis_type *axis;
} banks_type;

static banks_type *banks = 0;
/* static uint bankmap_len = 0; */
/* static volatile uint *bankmap_array = 0; */

/* static uint map_ndim = 0; */
/* static uint* map_dims; */

/*******************************************************************************
  function prototypes;
*******************************************************************************/

void process_rita2_init_daq(void);
void process_rita2(packet_type *p);
void process_rita2_mb(packet_type *p);
void process_rita2_destruct(void);

// void zmqReceive();

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   process_rita2_construct
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

int process_rita2_construct(void) {
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type *bank_descr_ptr;
  volatile axis_descr_type *axis_descr_ptr;
  //  volatile uint         *bmap_arraydesc_ptr;

  int status;
  uint axis, bank;

  histo_descr_ptr = getShmHistoPtr();
  if (!histo_descr_ptr) {
    dbg_printf(DBGMSG_ERROR,
               "process_rita2_construct(): can not get data pointer\n");
    return SINQHM_ERR_NO_HISTO_DESCR_PTR;
  }
  nbank = histo_descr_ptr->nBank;
  dbg_printf(DBGMSG_ERROR, "process_rita2_construct(): nbank = %d\n", nbank);

  histoDataPtr = getHistDataPtr();
  if (!histoDataPtr) {
    dbg_printf(DBGMSG_ERROR,
               "process_rita2_construct(): can not get histo data pointer\n");
    return SINQHM_ERR_NO_HISTO_DATA_PTR;
  }

  process_init_daq_fcn = process_rita2_init_daq;
  process_destruct_fcn = process_rita2_destruct;
  if (nbank == 1) {
    process_packet_fcn = process_rita2;
  } else {
    process_packet_fcn = process_rita2_mb;
  }

  banks = (banks_type *)rt_malloc((nbank * sizeof(banks_type)));
  if (!banks) {
    dbg_printf(
        DBGMSG_ERROR,
        "process_rita2_construct(): can not allocate memory for banks array\n");
    return SINQHM_ERR_FIL_MALLOC_FAILED;
  }
  memset((void *)banks, 0, (nbank * sizeof(banks_type)));

  for (bank = 0; bank < nbank; bank++) {

    bank_descr_ptr = getBankDescription(bank);
    if (!bank_descr_ptr) {
      dbg_printf(DBGMSG_ERROR,
                 "process_rita2_construct(): can not get bank description "
                 "pointer for bank %d\n",
                 bank);
      process_rita2_destruct();
      return SINQHM_ERR_NO_BANK_DESCR_PTR;
    }

    banks[bank].rank = bank_descr_ptr->rank;
    banks[bank].data = getBankData(bank);
    if (!banks[bank].data) {
      dbg_printf(DBGMSG_ERROR,
                 "process_rita2_construct(): can not get bank data pointer\n");
      return SINQHM_ERR_NO_BANK_DATA_PTR;
    }

    banks[bank].axis = (process_axis_type *)rt_malloc(
        (bank_descr_ptr->rank * sizeof(process_axis_type)));
    if (!banks[bank].axis) {
      dbg_printf(
          DBGMSG_ERROR,
          "process_rita2_construct(): can not allocate memory for axis\n");
      return -108;
    }
    memset((void *)banks[bank].axis, 0,
           (bank_descr_ptr->rank * sizeof(process_axis_type)));

    for (axis = 0; axis < banks[bank].rank; axis++) {
      axis_descr_ptr = getAxisDescription(bank, axis);

      /* if (axis_descr_ptr->type == AXMAP && axis == 0 ) { */
      /*   map_ndim++; */
      /*   banks[bank].axis[axis].array = (uint*)malloc(2*sizeof(uint)); */
      /*   banks[bank].axis[axis].array[0] = axis_descr_ptr->length; */
      /*   banks[bank].axis[axis].array[1] = 1; */
      /* } */
      /* if (axis_descr_ptr->type == AXMAP && axis > 0 ) { */
      /*   map_ndim++; */
      /*   banks[bank].axis[axis].array = (uint*)malloc(2*sizeof(uint)); */
      /*   banks[bank].axis[axis].array[0] = axis_descr_ptr->length; */
      /*   if ( getAxisDescription(bank, axis-1)->type == AXMAP ) { */
      /*     banks[bank].axis[axis].array[1] =
       * banks[bank].axis[axis-1].array[0]*banks[bank].axis[axis-1].array[1]; */
      /*   } */
      /*   else { */
      /*     banks[bank].axis[axis].array[1] = 1; */
      /*   } */

      /* } */

      status = SetAxisMapping(axis_descr_ptr, &(banks[bank].axis[axis]),
                              DO_RANGE_CHECK);
      if (status < 0) {
        process_rita2_destruct();
        return status;
      }
    }
  }

  return SINQHM_OK;
}

/******************************************************************************/

void process_rita2_init_daq(void) {
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
 *   process_rita2_destruct
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

void process_rita2_destruct(void) {
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
   *   process_packet_0mq
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

#define GOOD_EVENT 0
#define MAX_NO_AXIS 5

// input \in [1,4096]
// output \in [1,128]
int correct_x(const int x, const int y) {

  /* if (correction_dx > 0) { */

  double p1 = (correction_a_0 * y + correction_b_0) * y + correction_c_0;
  double p2 = (correction_a_1 * y + correction_b_1) * y + correction_c_1;
  if (p2 == 0) {
    dbg_printf(DBGMSG_WARNING, "Distortion correction failure: divide by 0\n");
    return -1;
  }
  double r = (x - p1) / p2;
  double w = r - correction_x1;
  double z = Nx - correction_deltax * 2;
  int t = round(w * z / correction_dx + correction_deltax);
  int max = 1;
  if (max < t) {
    max = t;
  }
  if (max < Nx) {
    return max;
  }
  return Nx;
  /* } */
  /* else { */
  /*   dbg_printf(DBGMSG_WARNING,"Unable to correct distrosion\n"); */
  /*   write_logfile(DBGMSG_WARNING,"sys","Unable to correct distrosion"); */
  /*   return -1; */
  /* } */
}

int correct_y(const int x, const int y) {
  /* if( correction_dy > 0) { */
  double w = y - correction_y1;
  double z = Ny - correction_deltay * 2;
  int t = round(w * z / correction_dy + correction_deltay);
  int max = 1;
  if (max < t) {
    max = t;
  }
  if (max < Ny) {
    return max;
  }
  return Ny;
  /* } */
  /* else { */
  /*   dbg_printf(DBGMSG_WARNING,"Unable to correct distrosion\n"); */
  /*   write_logfile(DBGMSG_WARNING,"sys","Unable to correct distrosion"); */
  /*   return -1; */
  /* } */
}

///////////////////
// input in range 0->4095
// increment & decrement to match matlab values
// output in range 0 -> 127
void correct_detector_raw_data(int *x_pos, int *y_pos) {

  if (shm_cfg_ptr[CFG_FIL_CORRECT_RAW_DATA]) {
    // store original values
    int x = (*x_pos) + 1;
    int y = (*y_pos) + 1;
    (*y_pos) = (correct_x(y, x) - 1);
    (*x_pos) = (correct_y(y, x) - 1);
  } else {
    (*x_pos) /= 32;
    (*y_pos) /= 32;
  }
}

void process_rita2(packet_type *p) {
  static uint hws0;
  uint event;
  uint32_t timestamp;
  uint binpos, tpos, axlen[MAX_NO_AXIS];
  int val[2], pos, bank, axis;
  uint evt;
  uint hws;

  bank = 0;
  for (axis = 0; axis < banks[bank].rank; ++axis) {
    axlen[axis] = getAxisDescription(0, axis)->length;
  }

  for (event = 0; event < 2 * (p->length); event++) {

    timestamp = p->data[event];

    event++;

    hws = p->data[event] & RITA2_COUNTER_MASK;
    evt = p->data[event] & RITA2_PACKET_TYPE_MASK;

    if (timestamp < 0) {
      dbg_printf(DBGMSG_ERROR, "Negative timestamp: event skipped\n");
      mongohm_log(DBGMSG_ERROR, "sys", "Event with negative timestamp");
      continue;
    } else {
      if (timestamp > MAX_TIMESTAMP) {
        dbg_printf(DBGMSG_ERROR,
                   "Timestamp out of range: event skipped (%ld,%ld)\n",
                   timestamp, MAX_TIMESTAMP);
        mongohm_log(DBGMSG_ERROR, "sys", "Event with timestamp out of range");
        continue;
      }
    }

    if (shm_cfg_ptr[CFG_FIL_ZMQ_FIRST_PKG]) {
      hws0 = hws;
    }

    if (hws != hws0) {
      snprintf(message, 1024, "neutron counter status changed: %x -> %x", hws0,
               hws);
      sinqhmlog(DBGMSG_INFO1, "sys", message);
      dbg_printf(DBGMSG_WARNING, "%s\n", message);
      hws0 = hws;
    }

    if (evt != EV_TYPE_POS_1D) {

      if (hws != RITA2_COUNTER_MASK) {
        shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]++;
        continue;
      }
      shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++;

      val[1] = mask_shift(p->data[event], 0x00fff000, 12);
      if (evt == EV_TYPE_NC) {
        val[0] = val[1] % 4096;
        val[1] = val[1] / 4096;
      } else {
        val[0] = mask_shift(p->data[event], 0x00000fff, 0);
      }

      if ((val[0] == 0) || (val[0] == 4095) || (val[1] == 0) ||
          (val[1] == 4095)) {
        continue;
      }

      correct_detector_raw_data(val, val + 1);

      pos = 0;
      for (axis = 0; axis < banks[bank].rank - 1; ++axis) { // skip timestamp
        pos = banks[bank].axis[axis].fcn(val[axis], &banks[bank].axis[axis]);

        if (pos < 0) {
          UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_LOW]);
          mongohm_log(DBGMSG_ERROR, "sys", "position lower than threshold");
          continue;
        } else {
          if (pos >= axlen[axis]) {
            mongohm_log(DBGMSG_ERROR, "sys", "position greater than threshold");
            UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_HIGH]);
            continue;
          }
        }

        if (axis > 0) {
          binpos = binpos * axlen[axis - 1] + pos;
        } else
          binpos = pos;
      }

      /* timestamp */
      axis = banks[bank].rank - 1;
      tpos = banks[bank].axis[axis].fcn(timestamp, &banks[bank].axis[axis]);

      if (tpos < 0) {
        UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_LOW]);
        continue;
      } else {
        if (tpos >= axlen[axis]) {
          UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_HIGH]);
          continue;
        }
      }

      /* Only if bot x,y and timestamp makes sense data is stored */
      UINT32_INC_CEIL_CNT_OVL(banks[bank].data[binpos],
                              shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);
      UINT32_INC_CEIL_CNT_OVL(banks[bank].data[axlen[0] * axlen[1] + tpos],
                              shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]);

    } else {
      shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]++;
      snprintf(message, 1024, "ts: %u\t(b,c,r,g):%u\tevt:%u\tpos: %uchan: %u",
               timestamp, hws >> 28, evt >> 24,
               mask_shift(p->data[event], 0x00fff000, 12),
               mask_shift(p->data[event], 0x00000fff, 0));
      write_logfile(DBGMSG_INFO1, "sys", message);
    }
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   process_rita2_mb
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

void process_rita2_mb(packet_type *p0) {
  /*   uint header_type, event,binpos; */
  /*   uint64_t timestamp; */
  /*   int  channel,zeromqpos; */
  /*   int  val, lookup, bank, axis, bankch; */
  /*   packet_zmq_type* p; */
  /*   volatile histo_descr_type *histo_descr_ptr; */

  /*   histo_descr_ptr = getShmHistoPtr(); */
  /*   //  printf("process_rita2_mb\n"); */

  /*   p = (packet_zmq_type*)p0; */

  /*   if( p->header_found == EV_TYPE_POS_2D ) { */
  /*     xy_t* data; */
  /*     data = (xy_t*)(p->data); */
  /*     for(int event = 0; event < p->length; ++event ) { */

  /*       timestamp = *(uint64_t*)(data[event].u); */
  /*       dbg_printf(DBGMSG_INFO7,"timestamp=%d --",timestamp); */

  /*       if( *(uint*)(data[event].u+4) == GOOD_EVENT ) { */
  /*         shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]++; */

  /*         for(bank = 0; bank < histo_descr_ptr->nBank; ++bank) { */
  /*           for(axis = 0; axis < banks[bank].rank-1 ;++axis) { */

  /*             val =
   * banks[bank].axis[axis].fcn((uint*)(data[event].u+6+2*axis),  */
  /*                                              &banks[bank].axis[axis]); */
  /*             /\* printf("val = %d\n",val); *\/ */

  /*             if (val < 0) { */
  /*               // val low */
  /*               UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_LOW]); */
  /*               return; */
  /*             } */
  /*             else if (val >= bankmap_len) { */
  /*               // val high */
  /*               UINT32_INC_CEIL(shm_cfg_ptr[CFG_FIL_CNT_HIGH]); */
  /*               return; */
  /*             } */

  /*             binpos = (axis * banks[bank].axis[1].len) + val; */
  /*             UINT32_INC_CEIL_CNT_OVL(banks[bank].data[binpos],
   * shm_cfg_ptr[CFG_FIL_BIN_OVERFLOWS]); */

  /*           } */
  /*         } */

  /*       } */
  /*       else { */
  /*         shm_cfg_ptr[CFG_FIL_EVT_ERROR]++; */
  /*       } */
  /*     } */
  /*   } */
}
