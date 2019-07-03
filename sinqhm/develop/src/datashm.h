/*******************************************************************************
  datashm.h
*******************************************************************************/

/**
 * This file belongs to the Linux-RTAI implementation of the SINQ histogram
 * memory software. HM software on Linux consists of two subsystems:
 * - the filler which is responsible for the actual histogramming
 * - a server (http) which is responsible for configuration and  
 *   communication.
 * Both processes communicate through shared memory. Two regions are used:
 * - a shared memory area for control
 * - a data shared memory area.
 * This is the header file for the management of the data area. This also 
 * includes the configuration of the histogram memory. 
 * 
 * Mark Koennecke, Gerd Theidel, September 2005
 */

#ifndef _DATASHM_H_
#define _DATASHM_H_


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

#define SHM_KEY_HISTOMEM    0x9e853447  /* nam2num("SHMDAT") */

#define DEFAULT_HISTO_MEM_SIZE  (20*1024*1024)

#define DATASHM_ID            0xDADA
#define DATASHM_CFG_FIL_VALID  0x82A932F7
#define DATASHM_CFG_SRV_VALID  0xA3F74198

#define DATASHM_VERSION   1


/**
 * axis mapping types
 */
#define AXDIRECT    0
#define AXCALC      1
#define AXBOUNDARY  2
#define AXLOOKUP    3
//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:07:59 2017
#define AXMAP       4

/*******************************************************************************
  Layout of Data Section
*******************************************************************************/

#define HM_CFG_BASE_PTR            0

#define HM_CFG_ID                  0
#define HM_CFG_VERSION             1
#define HM_CFG_SRV_VALID            2
#define HM_CFG_FIL_VALID            3

#define HM_CFG_MEM_SIZE            4
#define HM_CFG_MEM_USED            5

#define HM_CFG_HISTO_SIZE          6
#define HM_CFG_HISTO_OFFS          7

#define HM_CFG_HISTO_TYPE          8
#define HM_CFG_NBANK               9
#define HM_CFG_BANK_MAP_ARRAY     10
#define HM_CFG_BANK_OFFS_ARRAY    11

#define HM_CFG_RAWDATA_OFFS       12
#define HM_CFG_RAWDATA_SIZE       13
#define HM_CFG_RAWDATA_STORED     14
#define HM_CFG_RAWDATA_MISSED     15

#define HM_CFG_INCREMENT          16


#define HM_CFG_DYNAMIC_MEM_START  32

/******************************************************************************/

#ifdef BUILD_DATA_DESCR
#define CFG_DESCR(A,B)  {A,#A,B}

shm_descr_type  data_descr[] = 
{
    CFG_DESCR(HM_CFG_BASE_PTR           ,""),
    CFG_DESCR(HM_CFG_ID                 ,"ID of Histo Section"),
    CFG_DESCR(HM_CFG_VERSION            ,"Version of Histo Section"),
    CFG_DESCR(HM_CFG_SRV_VALID           ,"Server has written a valid Cfg"),
    CFG_DESCR(HM_CFG_FIL_VALID           ,"RT Filler is configured"),
    CFG_DESCR(HM_CFG_MEM_SIZE           ,""),
    CFG_DESCR(HM_CFG_MEM_USED           ,""),
    CFG_DESCR(HM_CFG_HISTO_SIZE         ,""),
    CFG_DESCR(HM_CFG_HISTO_OFFS         ,""),
    CFG_DESCR(HM_CFG_HISTO_TYPE         ,""),
    CFG_DESCR(HM_CFG_NBANK              ,""),
    CFG_DESCR(HM_CFG_BANK_OFFS_ARRAY    ,""),
    CFG_DESCR(HM_CFG_RAWDATA_OFFS       ,""),
    CFG_DESCR(HM_CFG_RAWDATA_SIZE       ,""),
    CFG_DESCR(HM_CFG_RAWDATA_STORED     ,""),
    CFG_DESCR(HM_CFG_RAWDATA_MISSED     ,""),
    CFG_DESCR(HM_CFG_INCREMENT          ,""),
    CFG_DESCR(HM_CFG_DYNAMIC_MEM_START  ,""),


};

#endif  /* BUILD_DATA_DESCR */


/*******************************************************************************
  type definitions
*******************************************************************************/

typedef unsigned int bankOffs_type;
typedef unsigned int dataShmOffs_type;

/******************************************************************************/

typedef struct 
{
  uint32 rank;
  uint32 *dim;
  uint32 *data;
}array_descr_type;

/******************************************************************************/

typedef struct 
{
  int type;
  int length;
  uint32 cnt_low;
  uint32 cnt_high;
  uint32 threshold;
  uint32 offset;
  union 
  {
    uint32 offs;
    uint32 *ptr;
  } axis_data;
}axis_descr_type;


/******************************************************************************/

typedef struct  
{
  uint32 rank;
  uint32 bank_data_offs;
  uint32 bank_size;
  union 
  {
    uint32 offs;
    axis_descr_type *ptr;
  } axis_descr;
} bank_descr_type;


/******************************************************************************/

typedef struct 
{
  uint32 id;
  uint32 version;
  uint32 server_valid;
  uint32 filler_valid;
  uint32 cfg_mem_size;
  uint32 cfg_mem_used;
  uint32 histo_mem_size;
  uint32 histo_mem_offs;
  uint32 histo_type;
  uint32 nBank;
  union  {
    uint32 offs;
    array_descr_type *ptr;
  } bank_mapping_array;
  union  {
    uint32 offs;
    bank_descr_type *ptr;
  } bank_descr;
  uint32 rawdata_offs;
  uint32 rawdata_size;
  uint32 rawdata_stored;
  uint32 rawdata_missed;
  uint32 increment;      // only used by tofmap 
} histo_descr_type;


/*******************************************************************************
  function prototypes
*******************************************************************************/


/**
 * attach to the shared memory section holding holding the histogram data
 * On RTAI to be called by the user side.
 * @return a negative error code on failure.
 */
__INLINE int initShmHisto(void);


/**
 * release the connection to the data shared memory. On RTAI to be called
 * by the user side.
 * @return a negative error code on failure
 */
__INLINE void releaseShmHisto(void);


 /**
  * get a pointer to the bank description in the data shared memory
  * @param bankno The number of the bank to search a description for
  * @return a pointer to the bank description or NULL if no such bank exists
  */
__INLINE volatile bank_descr_type* getBankDescription(int bankno);


/**
 * get a axis description 
 * @param bankno The bank for which to retrieve an axis desciption
 * @param axisno The dimension for which to retrieve the axis description
 * @return a pointer to the axis description or NULL if no such axis exists
 */
__INLINE volatile axis_descr_type *getAxisDescription(int bankno, int axisno);


/**
 * int getAxisData return auxiliary data to describe the axis. This can be the
 * bin array.  
 * @param bankno The bank for which to retrieve an axis desciption
 * @param axisno The dimension for which to retrieve the axis description
 * @return a pointer to the axis data or NULL if no such axis exists
 */
__INLINE volatile unsigned int *getAxisData(volatile axis_descr_type  *axis_descr_ptr);


 /**
  * getBankDataSize calculates the length of the data area for a given
  * bank
  * @param bank the bank for which to calculate the data length
  * @return The length of the data or a negative error code on
  * failure
  */
__INLINE int getBankDataSize(int bank); 


__INLINE volatile uint32 *getBankData(int bankno);
__INLINE volatile uint32 *dataShmOffsToPtr(uint32 offset);
__INLINE volatile uint32 *getHistDataPtr(void);
__INLINE unsigned int getHistoDataSize(void);
__INLINE void dataShmFreeAll(void);
__INLINE unsigned int dataShmAvail(void);
__INLINE unsigned int dataShmAlloc(unsigned int size);
__INLINE volatile histo_descr_type *getShmHistoPtr(void);

/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#include "datashm.c"
#endif


/******************************************************************************/

#endif //_DATASHM_H_
