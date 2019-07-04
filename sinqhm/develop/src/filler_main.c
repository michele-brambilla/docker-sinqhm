/*******************************************************************************
  filler_main.c
*******************************************************************************/

/*******************************************************************************
  includes
*******************************************************************************/

#ifdef FILLER_RTAI

#include <linux/kernel.h>
#include <linux/module.h>
#include <rtai.h>
#include <rtai_sched.h>

#elif defined FILLER_USER

#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#define ENXIO 1024

#endif

#include "controlshm.h"
#include "datashm.h"
#include "debugshm.h"
#include "lwlpmc.h"
#include "needfullthings.h"
#include "process_common.h"
#include "sinqhm_errors.h"
#include "sinqhm_types.h"

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:34:00 2017
#include "zeromq.h"

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Mon Apr 24 13:59:38 2017
#include "sinqhmlog.h"

/*******************************************************************************
  kernel module defines
*******************************************************************************/

#ifdef FILLER_RTAI
MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;
#endif

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define SINQHM_FIL_VER_MAJ 0
#define SINQHM_FIL_VER_MIN 4
#define SINQHM_FIL_VER_SUB 0
#define SINQHM_FIL_VER_BLD 0

#define SINQHM_FIL_VERSION                                                     \
  (SINQHM_FIL_VER_MAJ << 24 | SINQHM_FIL_VER_MIN << 16 |                       \
   SINQHM_FIL_VER_SUB << 8 | SINQHM_FIL_VER_BLD)

#define MODULE_COMPILED                                                        \
  "******* SINQHM-Filler compiled "__DATE__                                    \
  " "__TIME__                                                                  \
  " *******\n"

// this function detects oversized packets before they are processed
// but is a bit slower...
//#define PACKET_GET   packet_get_ov_chk

// this is faster but an oversized packet is processed before that is recognized
#define PACKET_GET packet_get

/*******************************************************************************
  Real Time Task Settings
*******************************************************************************/

/* timer period length in ns  */

#define DEFAULT_TASK_PERIOD 1000000
#define DEFAULT_FIL_DUTY_CYCLE 80

#ifdef ARCH_X86

#ifdef FILLER_RTAI
#define NANOSEC_PER_TIC 1
#elif defined FILLER_USER
#define NANOSEC_PER_TIC 1000
#endif

#elif defined ARCH_PPC
// TBD: this is Hardware dependant !!!
// valid for MEN A12 CPU with 100 MHz Bus Clock
#define NANOSEC_PER_TIC 40
#endif

/* 0 is highest priority, RT_LOWEST_PRIORITY is lowest */
#define RT_TASK_PRIORITY 1

#define RT_TASK_USES_FPU 0
#define RT_TASK_STACK_SIZE 16384

/*******************************************************************************
  global vars
*******************************************************************************/

#ifdef FILLER_RTAI
RT_TASK task_descr;
#endif

volatile unsigned int *shm_cfg_ptr = NULL;
volatile histo_descr_type *shm_histo_ptr = NULL;

extern volatile unsigned int *shm_dbg_ptr;
extern volatile lwl_pmc_val_type *pmc_base_ptr;

/*******************************************************************************
  kernel module parameters
*******************************************************************************/

int hst_size = DEFAULT_HISTO_MEM_SIZE;
int dbg_size = DEFAULT_DEBUG_BUFF_SIZE;
int task_period = DEFAULT_TASK_PERIOD;
int rt_duty_cycle = DEFAULT_FIL_DUTY_CYCLE;
int max_rt_tics;

#ifdef FILLER_RTAI
MODULE_PARM(hst_size, "i");
MODULE_PARM(dbg_size, "i");
MODULE_PARM(task_period, "i");
MODULE_PARM(rt_duty_cycle, "i");
#endif

/*******************************************************************************
  function prototypes
*******************************************************************************/

static void periodic_rt_task(int t);
void daq_loop(void);
void config_loop(void);

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:34:44 2017
static void periodic_zeromq_rt_task(int);
void daq_loop_0mq(void);
void config_zeromq_loop(void);

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
  zeroHM

  zeros the histogram memory and all counters
*******************************************************************************/

void zeroHM(void) {
  size_t size;
  volatile uint32 *histoDataPtr;

  size = (CFG_FIL_CLEAR_EACH_DAQ_END - CFG_FIL_CLEAR_EACH_DAQ_START + 1) *
         sizeof(uint32);
  memset((void *)&shm_cfg_ptr[CFG_FIL_CLEAR_EACH_DAQ_START], 0, size);

  histoDataPtr = getHistDataPtr();
  size = getHistoDataSize();
  if (histoDataPtr && size) {
    memset((void *)histoDataPtr, 0, size);
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   init_daq_start
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

void init_daq_start(packet_type *p) {
  struct timeval tv;

  packet_init(p);

  zeroHM();
  process_init_daq();

  shm_cfg_ptr[CFG_FIL_ACQ_COUNT]++;

  do_gettimeofday(&tv);
  shm_cfg_ptr[CFG_FIL_ACQ_START_TIME] = (uint32)tv.tv_sec;
  shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME] = 0;

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Tue Apr 11 15:36:24 2017
  shm_cfg_ptr[CFG_FIL_PKG_INCOMPLETE] = 0;

  empty_fifo(max_rt_tics);
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:37:52 2017
void init_daq_zmq_start(packet_type *p) {
  struct timeval tv;

  /* packet_init(p); */
  {
    p->header_found = 0;
    p->ptr = 0;
    p->length = 0;
  }

  zeroHM();

  process_init_daq();

  shm_cfg_ptr[CFG_FIL_ACQ_COUNT] = 0;
  shm_cfg_ptr[CFG_FIL_EVT_SKIPPED] = 0;

  do_gettimeofday(&tv);
  shm_cfg_ptr[CFG_FIL_ACQ_START_TIME] = (uint32_t)tv.tv_sec;
  shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME] = 0;
  shm_cfg_ptr[CFG_ZMQ_BYTES_READ] = 0;

  shm_cfg_ptr[CFG_FIL_ZMQ_FIRST_PKG] = 1;
  shm_cfg_ptr[CFG_FIL_PKG_INCOMPLETE] = 0;
}

/*******************************************************************************/

void leave_daq_loop(void) {
  struct timeval tv;

  do_gettimeofday(&tv);
  shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME] = (uint32)tv.tv_sec;

  process_leave_daq();
}

/*******************************************************************************/

void update_pkg_stat_cnt(void) {

  if (shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK]) // DAQ STARTED
  {
    UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_PKG_INCOMPLETE],
                      shm_cfg_ptr[CFG_FIL_TMP_PKG_INCOMPLETE]);
    UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_PKG_OVERSIZE],
                      shm_cfg_ptr[CFG_FIL_TMP_PKG_OVERSIZE]);
    UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_PKG_READ],
                      shm_cfg_ptr[CFG_FIL_TMP_PKG_READ]);
  }

  UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PKG_INCOMPLETE],
                    shm_cfg_ptr[CFG_FIL_TMP_PKG_INCOMPLETE]);
  UINT32_INC_X_CEIL(shm_cfg_ptr[CFG_FIL_SUM_PKG_OVERSIZE],
                    shm_cfg_ptr[CFG_FIL_TMP_PKG_OVERSIZE]);
  UINT32_LOW_HIGH_INC_X(shm_cfg_ptr[CFG_FIL_SUM_PKG_READ_LOW],
                        shm_cfg_ptr[CFG_FIL_SUM_PKG_READ_HIGH],
                        shm_cfg_ptr[CFG_FIL_TMP_PKG_READ]);

  shm_cfg_ptr[CFG_FIL_TMP_PKG_READ] = 0;
  shm_cfg_ptr[CFG_FIL_TMP_PKG_INCOMPLETE] = 0;
  shm_cfg_ptr[CFG_FIL_TMP_PKG_OVERSIZE] = 0;
}

/*******************************************************************************
 *
 * FUNCTION
 *   check_lwl_status
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

static inline void check_lwl_status(void) {
  unsigned int lwl_status;

  lwl_status = lwl_status_get();
  shm_cfg_ptr[CFG_FIL_STATUS_LWL] = lwl_status;

  if (shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK]) // DAQ STARTED
  {
    if (lwl_status & LWL_STATUS_FF)
      shm_cfg_ptr[CFG_FIL_CNT_FIFO_OVERFLOW]++;
    if (lwl_status & LWL_STATUS_VLNT)
      shm_cfg_ptr[CFG_FIL_CNT_TAXI_ERROR]++;
    //    if (lwl_status & LWL_STATUS_PF) shm_cfg_ptr[CFG_FIL_CNT_POWER_FAIL]++;
  }

  if (lwl_status & LWL_STATUS_FF)
    shm_cfg_ptr[CFG_FIL_SUM_CNT_FIFO_OVERFLOW]++;
  if (lwl_status & LWL_STATUS_VLNT)
    shm_cfg_ptr[CFG_FIL_SUM_CNT_TAXI_ERROR]++;
  //  if (lwl_status & LWL_STATUS_PF) shm_cfg_ptr[CFG_FIL_SUM_CNT_POWER_FAIL]++;
}

/*******************************************************************************
 *
 * FUNCTION
 *   periodic_rt_task
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

static void periodic_rt_task(int t) {
  empty_fifo(max_rt_tics);
  config_loop();
  // config loop does not return here...
}

/*******************************************************************************
 *
 * FUNCTION
 *   config_loop
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

void config_loop(void) {
  int status;
  int p_status;
  packet_type packet;
  unsigned long time1, time2;
  int empty, tcheck;

  shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_CONFIG_LOOP;
  dbg_printf(DBGMSG_INFO2, "config_loop(): Entering Config Loop\n");
  packet_init(&packet);

  while (1) {

    // CFG_SRV_DO_CFG_CMD
    tic_read(time1);
    empty = 0;
    tcheck = 0;

    if (shm_cfg_ptr[CFG_SRV_DO_CFG_CMD] && !shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
      status = process_construct();
      shm_cfg_ptr[CFG_FIL_CFG_STATUS] = status;
      shm_cfg_ptr[CFG_FIL_DO_CFG_ACK] = 1;
    }

    if (!shm_cfg_ptr[CFG_SRV_DO_CFG_CMD] && shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
      process_destruct();
      shm_cfg_ptr[CFG_FIL_DO_CFG_ACK] = 0;
    }

    // CFG_SRV_DO_DAQ_CMD

    if (shm_histo_ptr->filler_valid == DATASHM_CFG_FIL_VALID) {
      if (shm_cfg_ptr[CFG_SRV_DO_DAQ_CMD] && shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
        shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] = 1;

        if ((shm_histo_ptr->histo_type == FILLERRITA2) |
            (shm_histo_ptr->histo_type == FILLERCAMEA)) {
          daq_loop_0mq();
        } else {
          daq_loop();
        }

        dbg_printf(DBGMSG_INFO2, "config_loop(): Entering Config Loop\n");
        shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_CONFIG_LOOP;
        shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] = 0;
        packet_init(&packet);
      }
    }

    // Reset Print Buffer

    if (shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD] &&
        !shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK]) {
      /* Command is set, but Ack not, so execute command and set acknowledge */
      init_dbg_print_buff();
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 1;
    } else if (shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] &&
               !shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD]) {
      /* acknowledge is set, but Cmd not, so reset ack */
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 0;
    }

    if (!shm_cfg_ptr[CFG_SRV_SKIP_EMPTY_FIFO]) {
      //      empty_fifo_period(max_rt_tics>>1);
      while (1) {
        p_status = PACKET_GET(&packet);

        if (p_status == STATUS_PACKET_AVAIL) {
          process_tsi(&packet);
        }

        if (p_status == STATUS_FIFO_EMPTY) {
          if (empty++ > 5)
            break;
        }

        if (tcheck++ > 10) {
          tcheck = 0;
          tic_read(time2);
          if ((time2 - time1) > max_rt_tics)
            break;
        }
      }
    }

    check_lwl_status();

    // package counter
    update_pkg_stat_cnt();

    shm_cfg_ptr[CFG_FIL_ALIVE_CONFIG_LOOP]++;

    rt_task_wait_period();
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   daq_loop
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

void daq_loop(void) {
  int empty, tcheck;
  unsigned int check_lwl_status_cnt = 0;
  unsigned long time1, time2;
  unsigned long msr;
  int p_status;
  packet_type packet;

  dbg_printf(DBGMSG_INFO2, "daq_loop(): Entering DAQ Loop\n");

  shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_DAQ_LOOP;

  init_daq_start(&packet);

  while (shm_cfg_ptr[CFG_SRV_DO_DAQ_CMD]) {
    empty = 0;
    tcheck = 0;

    msr = disable_interrupts();

    tic_read(time1);

    if (!shm_cfg_ptr[CFG_SRV_DAQ_PAUSE_CMD]) {
      while (1) {
        p_status = PACKET_GET(&packet);

        if (p_status == STATUS_PACKET_AVAIL) {
          process_packet_fcn(&packet);
        }

        if (p_status == STATUS_FIFO_EMPTY) {
          if (empty++ > 5)
            break;
        }

        if (tcheck++ > 10) {
          tcheck = 0;
          tic_read(time2);
          if ((time2 - time1) > max_rt_tics)
            break;
        }
      }
    } else {
      // DAQ paused but go on reading packets
      while (1) {
        p_status = PACKET_GET(&packet);

        if (p_status == STATUS_PACKET_AVAIL) {
          process_tsi(&packet);
        }

        if (p_status == STATUS_FIFO_EMPTY) {
          if (empty++ > 5)
            break;
        }

        if (tcheck++ > 10) {
          tcheck = 0;
          tic_read(time2);
          if ((time2 - time1) > max_rt_tics)
            break;
        }
      }
    }

    // TBD: #ifdef DEBUG
    if (shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD] &&
        !shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK]) {
      /* Command is set, but Ack not, so execute command and set acknowledge */
      // Do Command
      init_dbg_print_buff();
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 1;
    } else if (shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] &&
               !shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD]) {
      /* acknowledge is set, but Cmd not, so reset ack */
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 0;
    }
    // #endif

    // package counter
    update_pkg_stat_cnt();

    // alive counter
    shm_cfg_ptr[CFG_FIL_ALIVE_DAQ_LOOP]++;

    // get LWL Status
    if (check_lwl_status_cnt++ >= 9) {
      check_lwl_status_cnt = 0;
      check_lwl_status();
    }

    set_msr(msr); /* restore msr */
    rt_task_wait_period();
  }

  leave_daq_loop();
}

/******************************************************************************/
/******************************************************************************/

int init_filler(void) {
  struct timeval tv;
  int status;

  rt_printk("\n\n" MODULE_COMPILED);

  // first allocate Debug SHM so that we can use dbg_printf
  status = initShmDebug();

  if (status != SINQHM_OK) {
    rt_printk("ERROR - can not allocate DEBUG BUFFER\n");
    return (-EBUSY);
  } else {
    shm_dbg_ptr = getDebugBuffPtr();
    rt_printk("init_module(): Debug  SHM allocated: dbg_size     =%9d bytes "
              "(shm_dbg_ptr   = %p)\n",
              dbg_size, shm_dbg_ptr);
    memset((void *)shm_dbg_ptr, 0, dbg_size);
    shm_dbg_ptr[DBG_SRV_MSG_LEVEL] = DBG_MSG_DEFAULT_LEVEL;
    shm_dbg_ptr[DBG_FIL_ERR_BUFF_SIZE] = DGB_ERROR_BUFF_SIZE;

    init_dbg_print_buff();
  }

  // repeat messages for Debug SHM
  dbg_printf(DBGMSG_INFO1, "init_module(): %s", MODULE_COMPILED);
  dbg_printf(DBGMSG_INFO1, "init_module(): Debug  SHM allocated: dbg_size     "
                           "=%9d bytes (shm_dbg_ptr   = %p)\n",
             dbg_size, shm_dbg_ptr);

  // allocate Histo SHM
  status = initShmHisto();
  if (status != SINQHM_OK) {
    dbg_printf(DBGMSG_ERROR, "init_module(): Cannot allocate Histo SHM\n");
    rt_printk("ERROR: init_module(): Cannot allocate Histo SHM\n");

    releaseShmDebug();

    return (-EBUSY);
  } else {
    shm_histo_ptr = getShmHistoPtr();
    shm_histo_ptr->server_valid = 0;
    shm_histo_ptr->filler_valid = 0;
    dbg_printf(DBGMSG_INFO1, "init_module(): Histo  SHM allocated: hst_size    "
                             " =%9d bytes (shm_histo_ptr = %p)\n",
               hst_size, shm_histo_ptr);
    rt_printk("init_module(): Histo  SHM allocated: hst_size     =%9d bytes "
              "(shm_histo_ptr = %p)\n",
              hst_size, shm_histo_ptr);
  }

  // at last allocate and initialize control shm

  status = initShmControl();

  if (status != SINQHM_OK) {
    dbg_printf(DBGMSG_ERROR, "init_module(): Cannot allocate Config SHM\n");
    rt_printk("ERROR: init_module(): Cannot allocate Config SHM\n");

    releaseShmHisto();
    releaseShmDebug();

    return (-EBUSY);
  } else {
    shm_cfg_ptr = getVarPointer(CFG_BASE_PTR);
    memset((void *)shm_cfg_ptr, 0, SHM_CFG_SIZE);
    shm_cfg_ptr[CFG_FIL_CFG_ID] = CFG_ID;
    shm_cfg_ptr[CFG_FIL_CLEVEL] = COMPATIBILITY_LEVEL;
    shm_cfg_ptr[CFG_FIL_VERSION] = SINQHM_FIL_VERSION;
    shm_cfg_ptr[CFG_FIL_CFG_SHM_SIZE] = SHM_CFG_SIZE;
    shm_cfg_ptr[CFG_FIL_HST_SHM_SIZE] = hst_size;
    shm_cfg_ptr[CFG_FIL_DBG_SHM_SIZE] = dbg_size;
    shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_TERMINATED;
    shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] = 0;
    shm_cfg_ptr[CFG_FIL_DO_CFG_ACK] = 0;

    shm_cfg_ptr[CFG_FIL_CFG_VALID] = CFG_VALID;

    dbg_printf(DBGMSG_INFO1, "init_module(): Config SHM allocated: "
                             "SHM_CFG_SIZE =%9d bytes (shm_cfg_ptr   = %p)\n",
               SHM_CFG_SIZE, shm_cfg_ptr);
    rt_printk("init_module(): Config SHM allocated: SHM_CFG_SIZE =%9d bytes "
              "(shm_cfg_ptr   = %p)\n",
              SHM_CFG_SIZE, shm_cfg_ptr);
  }

  snprintf((char *)&shm_cfg_ptr[CFG_FIL_COMPTIME_BUFF_START],
           (CFG_FIL_COMPTIME_BUFF_END - CFG_FIL_COMPTIME_BUFF_START) * 4,
           "%s %s", __DATE__, __TIME__);

  do_gettimeofday(&tv);
  shm_cfg_ptr[CFG_FIL_MOD_INST_TIME] = tv.tv_sec;

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Tue Apr 11 15:40:25 2017
  int (*init_func)(void);

  // connect DAQ2
  if (strlen(zeromq_bind_address) > 0) {
    init_func = &zeromq_init;
  } else {
    init_func = &pmc_module_init;
  }

  // initialize LWL PMC module and PCI Bridges or 0MQ
  if ((*init_func)() != 0) {
    dbg_printf(DBGMSG_ERROR, "init_filler failure");
    return (-ENXIO);
  }

  max_rt_tics = (rt_duty_cycle * task_period / 100) / NANOSEC_PER_TIC;

  rt_printk("\nTask Timing\n");
  rt_printk("***********\n");
  rt_printk("task_period   = %d ns\n", task_period);
  rt_printk("rt_duty_cycle = %d percent\n", rt_duty_cycle);
  rt_printk("tic duration  = %d ns\n", NANOSEC_PER_TIC);
  rt_printk("max_rt_tics   = %d tics (%d ns)\n", max_rt_tics,
            (max_rt_tics * NANOSEC_PER_TIC));

  shm_cfg_ptr[CFG_FIL_TASK_PERIOD] = task_period;
  shm_cfg_ptr[CFG_FIL_DUTY_CYCLE] = rt_duty_cycle;
  shm_cfg_ptr[CFG_FIL_TIC_DURATION] = NANOSEC_PER_TIC;
  shm_cfg_ptr[CFG_FIL_MAX_FIL_TICS] = max_rt_tics;

  return 0;
}

/******************************************************************************/

void cleanup_filler(void) {
  pmc_module_close();

  if (shm_histo_ptr) {
    shm_histo_ptr->server_valid = 0;
    shm_histo_ptr->filler_valid = 0;
    releaseShmHisto();
  }

  if (shm_cfg_ptr) {
    // reset state and ack flags in case server app has still allocated the shm
    shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_TERMINATED;
    shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] = 0;
    shm_cfg_ptr[CFG_FIL_DO_CFG_ACK] = 0;
    shm_cfg_ptr[CFG_FIL_CFG_VALID] = 0;

    releaseShmControl();
  }

  if (shm_dbg_ptr) {
    releaseShmDebug();
  }
}

#ifdef FILLER_RTAI
/*******************************************************************************
 *
 * FUNCTION
 *   init_module
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

int init_module(void) {
  RTIME tick_period;
  int status;
  // initialize filler
  status = init_filler();

  if (status < 0)
    return status;

// now setup and start periodic Task

#if !defined(CONFIG_PPC)
  /* set periodic mode for timer */
  rt_set_periodic_mode();
#else
  /* on PowerPC we use oneshot mode to reduce system load */
  rt_set_oneshot_mode();
#endif

  /* initializing the task */
  rt_task_init(&task_descr, periodic_rt_task, 0, RT_TASK_STACK_SIZE,
               RT_TASK_PRIORITY, RT_TASK_USES_FPU, 0);

  /* start timer */
  tick_period = start_rt_timer(nano2count(task_period));

  /* the realtime process should be choosen by the scheduler periodically */
  rt_task_make_periodic(&task_descr, rt_get_time() + 2 * tick_period,
                        tick_period);

  return 0; // initialisation ok
}

/*******************************************************************************
 *
 * FUNCTION
 *   cleanup_module
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

void cleanup_module(void) {
  /* kill timer */
  stop_rt_timer();

  rt_busy_sleep(10000000);

  /* kill task */
  rt_task_delete(&task_descr);
  dbg_printf(DBGMSG_INFO1,
             "\n\ncleanup_module(): Realtime Task terminated.\n\n");
  rt_printk("\n\ncleanup_module(): Realtime Task terminated.\n\n");

  cleanup_filler();
}

/******************************************************************************/
/******************************************************************************/

#elif defined FILLER_USER

static void terminateSignal(int signal) {

  dbg_printf(DBGMSG_INFO1, "\n\nsinqhm_filler: Task terminated.\n\n");
  rt_printk("\n\nsinqhm_filler: Task terminated.\n\n");
  cleanup_filler();
  exit(0);
}

/******************************************************************************/

int main(int argc, char *argv[]) {
  int status;
  int i;

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Tue Apr 11 11:42:11 2017
  int apply_detector_correction = 1;
  zeromq_bind_address[0] = '\0';

  for (i = 1; i < argc; i++) {
    if (strncmp(argv[i], "hst_size=", 9) == 0) {
      hst_size = atoi(argv[i] + 9);
    } else if (strncmp(argv[i], "dbg_size=", 9) == 0) {
      dbg_size = atoi(argv[i] + 9);
    } else if (strncmp(argv[i], "task_period=", 12) == 0) {
      task_period = atoi(argv[i] + 12);
    } else if (strncmp(argv[i], "rt_duty_cycle=", 14) == 0) {
      rt_duty_cycle = atoi(argv[i] + 14);
    } else if (strncmp(argv[i], "lwlfifo=", 9) == 0) {
      strncpy(lwl_fifo_pipe_name, argv[i] + 9, 256);
    }
    //  \author Michele Brambilla <michele.brambilla@psi.ch>
    //  \date Tue Apr 11 11:37:22 2017
    else if (strncmp(argv[i], "zmq_socket=", 11) == 0) {
      strncpy(zeromq_bind_address, argv[i] + 11, 256);
      mongohm_log(4, "sys", "SINQHM configured with zeromq");
    } else if (strncmp(argv[i], "--no-detector-correction", 18) == 0) {
      apply_detector_correction = 0;
    } else {
      printf("ERROR: wrong parameter: %s\n", argv[i]);
      exit(1);
    }
  }

  if (apply_detector_correction) {
    mongohm_log(4, "sys", "enabled SINQHM correction of detector distorsion");
  }

  printf("hst_size = %d\n", hst_size);
  printf("dbg_size = %d\n", dbg_size);
  printf("task_period = %d\n", task_period);
  printf("rt_duty_cycle = %d\n", rt_duty_cycle);

  // initialize filler
  status = init_filler();
  if (status < 0)
    return status;

  signal(SIGTERM, terminateSignal);
  signal(SIGINT, terminateSignal);

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Tue Apr 11 11:37:54 2017
  shm_cfg_ptr[CFG_FIL_CORRECT_RAW_DATA] = 0;
  if (apply_detector_correction) {
    shm_cfg_ptr[CFG_FIL_CORRECT_RAW_DATA] = 1;
  }

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Tue Apr 11 15:46:23 2017
  printf("***** Starting Periodic Task ********************\n");
  if (strlen(zeromq_bind_address) > 0) {
    while (1) {
      periodic_zeromq_rt_task(0);
      printf("Use 0MQ\n");
    }
  } else {
    while (1) {
      periodic_rt_task(0);
      printf("USE LWL FIFO\n");
    }
  }

  //  \author Michele Brambilla <michele.brambilla@psi.ch>
  //  \date Wed Apr 26 14:37:15 2017
  sinqhmlog_cleanup();
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:47:45 2017
void periodic_zeromq_rt_task(int t) { config_zeromq_loop(); }

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:54:59 2017
void daq_loop_0mq() {
  packet_type packet;
  uint64_t time1, time2;
  uint64_t msr;
  int status;
  int tcheck;

  dbg_printf(DBGMSG_INFO2, "daq_loop(): Entering DAQ Loop 0MQ\n");

  shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_DAQ_LOOP;

  init_daq_zmq_start(&packet); // zeros packet info & HM
  shm_cfg_ptr[CFG_FIL_RCVMORE_ACK] = 0;

  while (shm_cfg_ptr[CFG_SRV_DO_DAQ_CMD]) {
    tcheck = 0;
    msr = disable_interrupts();

    tic_read(time1);

    /* if( !shm_cfg_ptr[CFG_SRV_DAQ_PAUSE_CMD] )  { */

    while (1) {
      status = zmqReceive(&packet); // receives zmq message & update counters

      /* If any failure */
      if (status != 0) {
        packet.length = 0;
        continue;
        /* if( shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO]     ||   */
        /*     !shm_cfg_ptr[CFG_FIL_ZMQ_INI_DONE] ||   */
        /*     shm_cfg_ptr[CFG_FIL_ZMQ_INI_ERROR]   ) { */
        /*   if (!shm_cfg_ptr[CFG_ZMQ_RECONNECT_NOTIFIED] ) { */
        /*     shm_cfg_ptr[CFG_ZMQ_RECONNECT_NOTIFIED] = 1; */
        /*     dbg_printf(DBGMSG_WARNING, "Trying reconnect\n"); */
        /*   } */
        /*   sleep(1.); */
        /*   zeromq_init(); */
        /*   continue; */
        /* } */
      }

      /* If !DAQ_PAUSE : process events */
      if (!shm_cfg_ptr[CFG_SRV_DAQ_PAUSE_CMD]) {
        shm_cfg_ptr[CFG_FIL_TSI_DAQ_STARTED]++;
        if (packet.data != NULL && packet.length > 0 &&
            packet.header_found > 0) {
          process_packet_fcn((packet_type *)&packet);
        }
      } else {
        shm_cfg_ptr[CFG_FIL_TSI_DAQ_PAUSED]++;
      }

      if (shm_cfg_ptr[CFG_FIL_ZMQ_FIRST_PKG]) {
        shm_cfg_ptr[CFG_FIL_ZMQ_FIRST_PKG] = 0;
      }

      if (tcheck++ > 10) {
        tcheck = 0;
        tic_read(time2);
        if ((time2 - time1) > max_rt_tics) {
          break;
        }
      }
    }
    /* else { */

    /*   while(1) { */
    /*     zmqReceive(&packet); // receives zmq message & update counters */

    /* 	shm_cfg_ptr[CFG_FIL_TSI_DAQ_STARTED]++;         */
    /*     if(tcheck++>10) { */
    /*       tcheck=0; */
    /*       tic_read(time2); */
    /*       if((time2-time1)>max_rt_tics) { */
    /*         break; */
    /*       } */
    /*     } */
    /*   } */
    /* } */

    // TBD: #ifdef DEBUG
    if (shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD] &&
        !shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK]) {
      /* Command is set, but Ack not, so execute command and set acknowledge */
      init_dbg_print_buff();
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 1;
    } else if (shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] &&
               !shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD]) {
      /* acknowledge is set, but Cmd not, so reset ack */
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 0;
    }
    // #endif

    // alive counter
    shm_cfg_ptr[CFG_FIL_ALIVE_DAQ_LOOP]++;
    set_msr(msr); /* restore msr */
    update_pkg_stat_cnt();
  }

  leave_daq_loop();
  dbg_printf(DBGMSG_INFO2, "leave_daq_loop() done\n");
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:55:05 2017
void config_zeromq_loop(void) {

  shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_CONFIG_LOOP;
  dbg_printf(DBGMSG_INFO2, "config_loop(): Entering Config Loop\n");
  packet_type packet;

  while (1) {

    if (shm_cfg_ptr[CFG_SRV_DO_CFG_CMD] && !shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
      dbg_printf(DBGMSG_INFO2, "process_0mq_construct()\n");
      status = process_construct(); // --> process_0mq_construct()
      shm_cfg_ptr[CFG_FIL_CFG_STATUS] = status;
      shm_cfg_ptr[CFG_FIL_DO_CFG_ACK] = 1;
    }

    if (!shm_cfg_ptr[CFG_SRV_DO_CFG_CMD] && shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
      process_destruct(); // process_0mq_destruct()
      shm_cfg_ptr[CFG_FIL_DO_CFG_ACK] = 0;
    }

    if (shm_histo_ptr->filler_valid == DATASHM_CFG_FIL_VALID) {
      // DAQ: stop
      if (!shm_cfg_ptr[CFG_SRV_DO_DAQ_CMD] && shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
        zmqReceiveForget(&packet);
      }

      // DAQ: run
      if (shm_cfg_ptr[CFG_SRV_DO_DAQ_CMD] && shm_cfg_ptr[CFG_FIL_DO_CFG_ACK]) {
        shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] = 1;
        assert((shm_histo_ptr->histo_type == FILLERRITA2) |
               (shm_histo_ptr->histo_type == FILLERCAMEA));

        daq_loop_0mq();

        dbg_printf(DBGMSG_INFO2, "config_loop(): Entering Config Loop\n");
        shm_cfg_ptr[CFG_FIL_FILLER_STATE] = FILLER_STATE_CONFIG_LOOP;
        shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK] = 0;
      }
    }

    // Reset Print Buffer
    if (shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD] &&
        !shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK]) {
      /* Command is set, but Ack not, so execute command and set acknowledge */
      init_dbg_print_buff();
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 1;
    } else if (shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] &&
               !shm_cfg_ptr[CFG_SRV_RST_PRINT_BUFF_CMD]) {
      /* acknowledge is set, but Cmd not, so reset ack */
      shm_cfg_ptr[CFG_FIL_RST_PRINT_BUFF_ACK] = 0;
    }

    // package counter
    update_pkg_stat_cnt();
    shm_cfg_ptr[CFG_FIL_ALIVE_CONFIG_LOOP]++;
    rt_task_wait_period();
  }
}

#endif

/******************************************************************************/
