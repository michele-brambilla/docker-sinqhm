/*******************************************************************************
  debugshm.h
*******************************************************************************/

/**
 * This file belongs to the SINQ Histogram Memory implementation for 
 * LINUX-RTAI. SINQHM-RTAI uses two shared memory sections: one for control
 * information and another one where the bank information and the actual
 * histogram is stored. This file describes the interface to the control
 * section of SINQHM-RTAI. The idea is that the knowledge about the layout
 * of this section stays within these files.
 * 
 * Gerd Theidel, Mark Koennecke, September 2005
 */

#ifndef _DEBUGSHM_H_
#define _DEBUGSHM_H_


/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#define __INLINE static inline
#else
#define __INLINE
#endif


/*******************************************************************************
  includes
*******************************************************************************/

#include "lwlpmc.h"
#include "sinqhm_shm.h"


/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define SHM_KEY_DEBUG_BUFF  0x9e853461   /* nam2num("SHMDBG") */

#define DEFAULT_DEBUG_BUFF_SIZE (1024*1024)

#define DGB_ERROR_BUFF_SIZE  1024

#define DGB_MAX_MESSAGE_LENGTH   1024


// Debug Message Levels

#define DBGMSG_ERROR    1
#define DBGMSG_WARNING  2
#define DBGMSG_INFO1    3
#define DBGMSG_INFO2    4
#define DBGMSG_INFO3    5
#define DBGMSG_INFO4    6
#define DBGMSG_INFO5    7
#define DBGMSG_INFO6    8
#define DBGMSG_INFO7    9

#define DBG_MSG_DEFAULT_LEVEL  DBGMSG_INFO3


/*******************************************************************************
  Layout of Data Section
*******************************************************************************/

#define DBG_FIL_BUFF_SIZE            1
#define DBG_FIL_STR_PTR              2
#define DBG_FIL_WRAP_AROUND          3
#define DBG_FIL_LOG_NUMBER           4
#define DBG_SRV_CHAR_BUFF_ONCE       5
#define DBG_SRV_MSG_LEVEL            6

#define DBG_FIL_ERR_BUFF_SIZE       10
#define DBG_FIL_ERR_BUFF_PTR        11
#define DBG_FIL_ERR_BUFF_SGN        12
#define DBG_SRV_ERR_BUFF_ACK        13

#define DBG_FIL_ERR_BUFF_START     50

#define DBG_FIL_CHAR_BUFF_START     (DBG_FIL_ERR_BUFF_START + DGB_ERROR_BUFF_SIZE)


/*******************************************************************************
  global vars
*******************************************************************************/

extern volatile unsigned int * shm_dbg_ptr;
extern int dbg_size;

/*******************************************************************************
  function prototypes
*******************************************************************************/

__INLINE int initShmDebug(void);
__INLINE void releaseShmDebug(void);
__INLINE volatile unsigned int *getDebugBuffPtr(void);

void init_dbg_print_buff(void);
int  print_lwl_status (unsigned int status);
void print_config(void);
void print_packet(packet_type *p);
int  dbg_printf(int level, const char *fmt, ...);
int datetimestr (char* buff, int len);


/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#include "debugshm.c"
#endif


/******************************************************************************/

#endif //_DEBUGSHM_H_
