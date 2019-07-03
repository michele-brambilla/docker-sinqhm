/*******************************************************************************
  controlshm.h
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

#ifndef _CONTROLSHM_H_
#define _CONTROLSHM_H_


//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:00:39 2017
#ifdef __cplusplus
#include <inttypes.h>
extern "C" {
#endif

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

#include "sinqhm_shm.h"
#include "sinqhm_types.h"


/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define SHM_KEY_CONTROL     0x9e852f0c  /* nam2num("SHMCFG") */

/**
 * Version of Config and Status Section
 */



#define CFG_ID     0x8A8A
#define CFG_VALID  0xa3450bfe

#define COMPATIBILITY_LEVEL     1

/**
 * filler state
 */
#define FILLER_STATE_TERMINATED       0
#define FILLER_STATE_CONFIG_LOOP      1
#define FILLER_STATE_DAQ_LOOP         2


/**
 * defines for various filler types
 */
#define FILLERTOF      0
#define FILLERPSD      1
#define FILLERDIG      2
#define FILLERTOFMAP   3
#define FILLERHRPT     4
#define FILLERSANS2    5
#define FILLERRITA2    6
#define FILLERCAMEA    7


/**
 * layout of config and status section
 */

/* #define SHM_CFG_SIZE    ( 64 * 1024 ) */
#define SHM_CFG_SIZE    ( 32 * 1024 )

#define CFG_BASE_PTR                      0
#define CFG_FIL_CFG_ID                     0
#define CFG_FIL_VERSION                    1
#define CFG_FIL_CLEVEL                     2

#define CFG_FIL_CFG_SHM_SIZE              10
#define CFG_FIL_HST_SHM_SIZE              11
#define CFG_FIL_DBG_SHM_SIZE              12

#define CFG_FIL_CFG_VALID                 15

#define CFG_SRV_DO_DAQ_CMD                33
#define CFG_FIL_DO_DAQ_ACK                34

#define CFG_SRV_DO_CFG_CMD                35
#define CFG_FIL_DO_CFG_ACK                36
#define CFG_FIL_CFG_STATUS                37

#define CFG_SRV_DAQ_PAUSE_CMD             40

#define CFG_SRV_DO_STORE_RAW_DATA         45


#define CFG_SRV_RST_PRINT_BUFF_CMD        50
#define CFG_FIL_RST_PRINT_BUFF_ACK        51

#define CFG_SRV_HDR_DAQ_MASK              60
#define CFG_SRV_HDR_DAQ_ACTIVE            61


#define CFG_SRV_SKIP_EMPTY_FIFO           65

#define CFG_FIL_FILLER_STATE              70
#define CFG_FIL_ALIVE_CONFIG_LOOP         71




#define CFG_FIL_CLEAR_EACH_DAQ_START     100       /* from here clear all values each DAQ */

#define CFG_FIL_STATUS_LWL               110
#define CFG_FIL_CNT_FIFO_OVERFLOW        113
#define CFG_FIL_CNT_TAXI_ERROR           115
#define CFG_FIL_CNT_POWER_FAIL           116

#define CFG_FIL_TSI_COUNT                120

#define CFG_FIL_TSI_DAQ_STARTED          121
#define CFG_FIL_TSI_DAQ_PAUSED           122
#define CFG_FIL_TSI_DAQ_STOPPED          123

#define CFG_FIL_TSI_HEADER               124
#define CFG_FIL_TSI_DATA                 125
#define CFG_FIL_TSI_DAQ_ACTIVE           126

#define CFG_FIL_TSI_RATE_LOW             127
#define CFG_FIL_TSI_POWER_FAIL           128
#define CFG_FIL_TSI_STATUS_WORD_CHANGED  129

#define CFG_FIL_ALIVE_DAQ_LOOP           130

#define CFG_FIL_TSI_DAQ_SYNC0            131
#define CFG_FIL_TSI_DAQ_SYNC1            132
#define CFG_FIL_TSI_DAQ_SYNC2            133
#define CFG_FIL_TSI_DAQ_SYNC3            134

#define CFG_FIL_PSD_HITS                 140
#define CFG_FIL_PSD_FLASH                141

#define CFG_FIL_PKG_READ                 150
#define CFG_FIL_PKG_INCOMPLETE           151
#define CFG_FIL_PKG_OVERSIZE             152
#define CFG_FIL_PKG_UNKNOWN              153

#define CFG_FIL_TMP_PKG_READ             155
#define CFG_FIL_TMP_PKG_INCOMPLETE       156
#define CFG_FIL_TMP_PKG_OVERSIZE         157


#define CFG_FIL_EVT_PROCESSED            160
#define CFG_FIL_EVT_SKIPPED              161
#define CFG_FIL_EVT_ERROR                162


#define CFG_FIL_PKG_GUMMI_MONITOR        165

#define CFG_FIL_CNT_LOW                  170
#define CFG_FIL_CNT_HIGH                 171

#define CFG_FIL_BIN_OVERFLOWS            180

#define CFG_FIL_TEST_1                   191
#define CFG_FIL_TEST_2                   192
#define CFG_FIL_TEST_3                   193
#define CFG_FIL_TEST_4                   194
#define CFG_FIL_TEST_5                   195

#define CFG_FIL_CLEAR_EACH_DAQ_END       199      /* clear until here */

// summed count und status information

#define CFG_FIL_MOD_INST_TIME                200
#define CFG_SRV_SERVER_START_TIME            201

#define CFG_FIL_ACQ_START_TIME               202
#define CFG_FIL_ACQ_STOP_TIME                203
#define CFG_FIL_ACQ_COUNT                    204

#define CFG_FIL_SUM_PKG_READ_LOW             210
#define CFG_FIL_SUM_PKG_READ_HIGH            211
#define CFG_FIL_SUM_PKG_INCOMPLETE           212
#define CFG_FIL_SUM_PKG_OVERSIZE             213
#define CFG_FIL_SUM_PKG_UNKNOWN              214

#define CFG_FIL_SUM_CNT_FIFO_OVERFLOW        215
#define CFG_FIL_SUM_CNT_TAXI_ERROR           216
#define CFG_FIL_SUM_CNT_POWER_FAIL           217

#define CFG_FIL_SUM_TSI_DAQ_STARTED          218
#define CFG_FIL_SUM_TSI_DAQ_PAUSED           219
#define CFG_FIL_SUM_TSI_COUNT                220
#define CFG_FIL_SUM_TSI_RATE_LOW             221
#define CFG_FIL_SUM_TSI_POWER_FAIL           222
#define CFG_FIL_SUM_TSI_STATUS_WORD_CHANGED  223
#define CFG_FIL_SUM_TSI_DAQ_STOPPED          224

#define CFG_FIL_SUM_PKG_GUMMI_MONITOR        225


#define CFG_FIL_TASK_PERIOD                  230
#define CFG_FIL_DUTY_CYCLE                   231
#define CFG_FIL_TIC_DURATION                 232
#define CFG_FIL_MAX_FIL_TICS                 233

#define CFG_FIL_SUM_PSD_HITS                 240
#define CFG_FIL_SUM_PSD_FLASH                241

#define CFG_FIL_COINC_TIME                   250
#define CFG_FIL_RATE_VALID                   251
#define CFG_FIL_RATE_AN                      252
#define CFG_FIL_RATE_KV                      253
#define CFG_FIL_RATE_KH                      254
#define CFG_FIL_PIC_LENGTH                   255

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:03:28 2017
#define CFG_FIL_ZMQ_INI_START                260
#define CFG_FIL_ZMQ_INI_DONE                 261
#define CFG_FIL_ZMQ_INI_ERROR                262

#define CFG_FIL_ZMQ_SOCK_ADDR                263
#define CFG_FIL_ZMQ_FIRST_PKG                264
#define CFG_FIL_ZMQ_PKG_TYPE                 265

#define CFG_FIL_ZMQ_KEEPALIVE                266
#define CFG_FIL_ZMQ_RCVTIMEO                 267
#define CFG_FIL_ZMQ_RCVTO                    268
#define CFG_ZMQ_RECONNECT_NOTIFIED           269

#define CFG_ZMQ_SICS_STATUS                  270
#define CFG_ZMQ_BYTES_READ                   271

#define CFG_ZMQ_RCVMORE                      272
#define CFG_FIL_RCVMORE_ACK                  273

#define CFG_FIL_CORRECT_RAW_DATA             280

/*******************************************************************************
end 0mq specific
*******************************************************************************/

#define CFG_FIL_COMPTIME_BUFF_START          300
#define CFG_FIL_COMPTIME_BUFF_END            400



#ifdef BUILD_CFG_DESCR
#define CFG_DESCR(A,B)  {A,#A,B}

shm_descr_type  cfg_descr[] =
{
    CFG_DESCR(CFG_BASE_PTR                     ,""),
    CFG_DESCR(CFG_FIL_CFG_ID                    ,"ID of CFG Section"),
    CFG_DESCR(CFG_FIL_VERSION                   ,"Version of CFG SECTION"),
    CFG_DESCR(CFG_FIL_CFG_SHM_SIZE              ,"Size of Control/Config SHM Section in bytes"),
    CFG_DESCR(CFG_FIL_HST_SHM_SIZE              ,"Size of Histogram SHM Section in bytes"),
    CFG_DESCR(CFG_FIL_DBG_SHM_SIZE              ,"Size of Debug SHM Section in bytes"),
    CFG_DESCR(CFG_FIL_CFG_VALID                 ,""),

    CFG_DESCR(CFG_SRV_DO_DAQ_CMD                ,"Command to do DAQ"),
    CFG_DESCR(CFG_FIL_DO_DAQ_ACK                ,"Acknowledge doing DAQ"),

    CFG_DESCR(CFG_SRV_DO_CFG_CMD                ,""),
    CFG_DESCR(CFG_FIL_DO_CFG_ACK                ,""),
    CFG_DESCR(CFG_FIL_CFG_STATUS                ,""),
    CFG_DESCR(CFG_SRV_DAQ_PAUSE_CMD             ,"Pause DAQ"),
    CFG_DESCR(CFG_SRV_DO_STORE_RAW_DATA         ,"Store Raw Data until Buffer is Full"),

    CFG_DESCR(CFG_SRV_RST_PRINT_BUFF_CMD        ,""),
    CFG_DESCR(CFG_FIL_RST_PRINT_BUFF_ACK        ,""),

    CFG_DESCR(CFG_SRV_HDR_DAQ_MASK              ,""),
    CFG_DESCR(CFG_SRV_HDR_DAQ_ACTIVE            ,""),

    CFG_DESCR(CFG_FIL_STATUS_LWL                ,"Status Register of LWL PMC"),
    CFG_DESCR(CFG_FIL_CNT_FIFO_OVERFLOW         ,"Counter for FIFO overflows"),
    CFG_DESCR(CFG_FIL_CNT_TAXI_ERROR            ,""),
    CFG_DESCR(CFG_FIL_CNT_POWER_FAIL            ,""),

    CFG_DESCR(CFG_FIL_TSI_COUNT                 ,"Number of Time Status Information Packets"),

    CFG_DESCR(CFG_FIL_TSI_DAQ_STARTED           ,"Number of TSI with DAQ started"),
    CFG_DESCR(CFG_FIL_TSI_DAQ_PAUSED            ,"Number of TSI with DAQ paused"),
    CFG_DESCR(CFG_FIL_TSI_DAQ_STOPPED           ,"Number of TSI with DAQ stopped"),

    CFG_DESCR(CFG_FIL_TSI_HEADER                ,""),
    CFG_DESCR(CFG_FIL_TSI_DATA                  ,""),
    CFG_DESCR(CFG_FIL_TSI_DAQ_ACTIVE            ,""),
    CFG_DESCR(CFG_FIL_TSI_RATE_LOW              ,""),
    CFG_DESCR(CFG_FIL_TSI_POWER_FAIL            ,""),
    CFG_DESCR(CFG_FIL_TSI_STATUS_WORD_CHANGED   ,""),

    CFG_DESCR(CFG_FIL_FILLER_STATE              ,""),
    CFG_DESCR(CFG_FIL_ALIVE_CONFIG_LOOP         ,""),
    CFG_DESCR(CFG_FIL_ALIVE_DAQ_LOOP            ,""),

    CFG_DESCR(CFG_FIL_TSI_DAQ_SYNC0             ,"TSI Packets with SYNC0 (DAQ started)"),
    CFG_DESCR(CFG_FIL_TSI_DAQ_SYNC1             ,"TSI Packets with SYNC1 (DAQ started)"),
    CFG_DESCR(CFG_FIL_TSI_DAQ_SYNC2             ,"TSI Packets with SYNC2 (DAQ started)"),
    CFG_DESCR(CFG_FIL_TSI_DAQ_SYNC3             ,"TSI Packets with SYNC3 (DAQ started)"),

    CFG_DESCR(CFG_FIL_PSD_HITS                  ,"Number of Hits from TDC while DAQ started"),
    CFG_DESCR(CFG_FIL_PSD_FLASH                 ,"Number of Flashes from TDC while DAQ started"),

    CFG_DESCR(CFG_FIL_PKG_READ                  ,"Number of read Packets (incl. oversized; excl. incomplete)"),
    CFG_DESCR(CFG_FIL_PKG_INCOMPLETE            ,"Number of incomplete Packets"),
    CFG_DESCR(CFG_FIL_PKG_OVERSIZE              ,"Number of oversized  Packets"),
    CFG_DESCR(CFG_FIL_PKG_UNKNOWN               ,"Number of Packets not known by selected filler type"),

    CFG_DESCR(CFG_FIL_TMP_PKG_READ              ,"Temporary counter for read Packets"),
    CFG_DESCR(CFG_FIL_TMP_PKG_INCOMPLETE        ,"Temporary counter for incomplete Packets"),
    CFG_DESCR(CFG_FIL_TMP_PKG_OVERSIZE          ,"Temporary counter for oversized  Packets"),


    CFG_DESCR(CFG_FIL_EVT_PROCESSED             ,"Number of processed Events"),
    CFG_DESCR(CFG_FIL_EVT_SKIPPED               ,"Number of skipped Eents"),
    CFG_DESCR(CFG_FIL_EVT_ERROR                 ,"Number of Events with Error"),

    CFG_DESCR(CFG_FIL_PKG_GUMMI_MONITOR         ,"Number of Gummi Monitor Packets"),


    CFG_DESCR(CFG_FIL_CNT_LOW                   ,""),
    CFG_DESCR(CFG_FIL_CNT_HIGH                  ,""),

    CFG_DESCR(CFG_FIL_BIN_OVERFLOWS             ,"Number of Bin Overflows"),
    CFG_DESCR(CFG_FIL_TEST_1                    ,"Temporary Test Counts"),
    CFG_DESCR(CFG_FIL_TEST_2                    ,"Temporary Test Counts"),
    CFG_DESCR(CFG_FIL_TEST_3                    ,"Temporary Test Counts"),
    CFG_DESCR(CFG_FIL_TEST_4                    ,"Temporary Test Counts"),
    CFG_DESCR(CFG_FIL_TEST_5                    ,"Temporary Test Counts"),
    CFG_DESCR(CFG_FIL_MOD_INST_TIME             ,"Time when RT_Module has been installed"),
    CFG_DESCR(CFG_SRV_SERVER_START_TIME         ,"Time when Server has been started"),
    CFG_DESCR(CFG_FIL_ACQ_START_TIME            ,"Time when Acquisition has been started"),
    CFG_DESCR(CFG_FIL_ACQ_STOP_TIME             ,"Time when Acquisition has been stopped"),

    CFG_DESCR(CFG_FIL_ACQ_COUNT                     ,"Number of Acquisitions since Boot"),

    CFG_DESCR(CFG_FIL_SUM_PKG_READ_LOW              ,"Summed Number of Packages (Low Word)"),
    CFG_DESCR(CFG_FIL_SUM_PKG_READ_HIGH             ,"Summed Number of Packages (High Word)"),
    CFG_DESCR(CFG_FIL_SUM_PKG_INCOMPLETE            ,"Summed Number of imcomplete Packages"),
    CFG_DESCR(CFG_FIL_SUM_PKG_OVERSIZE              ,"Summed Number of oversized Packages"),
    CFG_DESCR(CFG_FIL_SUM_PKG_UNKNOWN               ,"Summed Number of unknown Packages"),

    CFG_DESCR(CFG_FIL_SUM_CNT_FIFO_OVERFLOW         ,"Summed Number of Fifo Ovewrflows "),
    CFG_DESCR(CFG_FIL_SUM_CNT_TAXI_ERROR            ,"Summed Number of Taxi Errors"),
    CFG_DESCR(CFG_FIL_SUM_CNT_POWER_FAIL            ,"Summed Number of Power fails"),

    CFG_DESCR(CFG_FIL_SUM_TSI_DAQ_STARTED           ,"Summed Number of TSI with DAQ started"),
    CFG_DESCR(CFG_FIL_SUM_TSI_DAQ_PAUSED            ,"Summed Number of TSI with DAQ paused"),
    CFG_DESCR(CFG_FIL_SUM_TSI_COUNT                 ,"Summed Number of total TSI Packets"),
    CFG_DESCR(CFG_FIL_SUM_TSI_RATE_LOW              ,"Summed Number of Rate Low"),
    CFG_DESCR(CFG_FIL_SUM_TSI_POWER_FAIL            ,"Summed Number of Power Fail"),
    CFG_DESCR(CFG_FIL_SUM_TSI_STATUS_WORD_CHANGED   ,"Summed Number of Changed Status Words"),
    CFG_DESCR(CFG_FIL_SUM_TSI_DAQ_STOPPED           ,"Summed Number of TSI with DAQ stopped"),

    CFG_DESCR(CFG_FIL_SUM_PKG_GUMMI_MONITOR         ,"Summed Number of Gummi Monitor Packets"),

    CFG_DESCR(CFG_FIL_SUM_PSD_HITS                  ,"Summed Number of Hits from TDC"),
    CFG_DESCR(CFG_FIL_SUM_PSD_FLASH                 ,"Summed Number of Flashes from TDC"),

    CFG_DESCR(CFG_FIL_COINC_TIME                    ,"Coincidence Time     (SANS2)"),
    CFG_DESCR(CFG_FIL_RATE_VALID                    ,"Rate of valid Events (SANS2)"),
    CFG_DESCR(CFG_FIL_RATE_AN                       ,"Total Rate of Anode Events (SANS2)"),
    CFG_DESCR(CFG_FIL_RATE_KV                       ,"Total Rate of front Cathode Events (SANS2)"),
    CFG_DESCR(CFG_FIL_RATE_KH                       ,"Total Rate of rear Cathode Events (SANS2)"),

    CFG_DESCR(CFG_FIL_PIC_LENGTH                    ,"Picture Length with 0.1 ms Resolution (HRPT)"),

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:04:29 2017
    CFG_DESCR(CFG_FIL_ZMQ_INI_START   , "0MQ initialization"),
    CFG_DESCR(CFG_FIL_ZMQ_INI_DONE    , "0MQ initialization done"),
    CFG_DESCR(CFG_FIL_ZMQ_INI_ERROR   , "0MQ initialization error"),
    CFG_DESCR(CFG_FIL_ZMQ_SOCK_ADDR   , "0MQ socket address"),
    CFG_DESCR(CFG_FIL_ZMQ_FIRST_PKG   , "0MQ is waiting for the first package"),
    CFG_DESCR(CFG_FIL_ZMQ_PKG_TYPE    , "101 == acquisition format, 102 == debug format"),
    CFG_DESCR(CFG_FIL_CORRECT_RAW_DATA, "DAQ correction of distorsions"),

    CFG_DESCR(CFG_FIL_ZMQ_KEEPALIVE   , "0MQ TCPKEEPALIVE set"),
    CFG_DESCR(CFG_FIL_ZMQ_RCVTIMEO    , "0MQ RCVTIMEO set (value)"),
    CFG_DESCR(CFG_FIL_ZMQ_RCVTO       , "0MQ recv has timed out"),

    CFG_DESCR(CFG_ZMQ_RCVMORE         , "0MQ recv has pending messages"),
    CFG_DESCR(CFG_FIL_RCVMORE_ACK     , "Aknowledged 0MQ pending messages")

};

#endif  /* BUILD_CFG_DESCR */

/*******************************************************************************
  global vars
*******************************************************************************/


extern int hst_size;


/*******************************************************************************
  function prototypes
*******************************************************************************/


/**
 * connect to control shared memory
 * @return a negative error code on errors, 0 on success
 */
__INLINE int initShmControl(void);


 /**
  * release the connection to the shared control memory for the user side
  * or in test configuration
  */
__INLINE void releaseShmControl(void);


/**
 * getPointer returns the pointer to the memory area holding
 * the variable described by varID
 * @param varID The ID of the variable to retrieve
 * @return a pointer to the variable or NULL on failure
 */
__INLINE volatile unsigned int *getVarPointer(int varID);


/**
 * get the value of a control variable
 * @param varID one of the defines above
 * @return The value of the variable or a negative error code if something
 * is wrong.
 */
__INLINE int getControlVar(int varID, int* var);


/**
 * set the value of a control variable
 * @param varID one of the defines above
 * @param var The value to set.
 * @return 0 on success, a negative error code on errors.
 */
__INLINE int setControlVar(int varID, int value);


/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#include "controlshm.c"
#endif


/******************************************************************************/

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:00:48 2017
#ifdef __cplusplus
}
#endif

#endif //_CONTROLSHM_H_
