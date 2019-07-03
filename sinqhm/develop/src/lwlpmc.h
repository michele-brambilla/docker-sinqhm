/*******************************************************************************
  lwlpmc.h
*******************************************************************************/

#ifndef _LWLPMC_H_
#define _LWLPMC_H_


/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#define __INLINE static inline
#else
#define __INLINE
#endif

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define LWL_TUBE_MASK  0xFE00
#define LWL_TUBE_SHIFT 9
#define LWL_TUBE_ERROR 127    // Indicates zero or more than 1 address line were set

#define LWL_DATA_MASK  0x01FF
#define LWL_DATA_SHIFT 0

// for test with position Encoder
// row A - H are tubes, colums are positions inside tubes
//#define LWL_TUBE_MASK  0x0007
//#define LWL_TUBE_SHIFT 0
//#define LWL_DATA_MASK  0x00F8
//#define LWL_DATA_SHIFT 3

/******************************************************************************/

#define STATUS_FLAGS__PF     0x8000                 /* PF  - Power Fail */
#define STATUS_FLAGS__SWC    0x4000                 /* SWC - Status Word Changed */
#define STATUS_FLAGS__NRL    0x2000                 /* NRL - Neutron Rate Low */
#define STATUS_FLAGS__DAQ    0x1000
/* DAQ on -- set if Hdr Mask Bits are
 ** correct so that data acq is active */
#define STATUS_FLAGS__SYNC3  0x0800                 /* Ext Synch Bit #3 */
#define STATUS_FLAGS__SYNC2  0x0400                 /* Ext Synch Bit #2 */
#define STATUS_FLAGS__SYNC1  0x0200                 /* Ext Synch Bit #1 */
#define STATUS_FLAGS__SYNC0  0x0100                 /* Ext Synch Bit #0 */
#define STATUS_FLAGS__UD     0x0080                 /* UD  - Up/Down */
#define STATUS_FLAGS__GU     0x0040                 /* GU  - Gummi (i.e. Strobo) */

/*
 **------------------------------------------------------------------------------
 **       Definitions for the LWL Datagrams
 */

#define LWL_HDR_TYPE_MASK    0x1f000000             /* DS4 DS3 DS2 DS1 DS0 */
/* Mask for extracting main dgrm ..
 ** .. hdr command-type bits */
#define LWL_HDR_PF_MASK      0x80000000             /* Mask for extr Power Fail bit */
#define LWL_HDR_SWC_MASK     0x40000000             /* Mask for extr Status Word Chng bit */
#define LWL_HDR_NRL_MASK     0x20000000             /* Mask for extr Neutron Rate Low bit */
#define LWL_HDR_SYNC3_MASK   0x00800000             /* Mask for one of ext synch bits */
#define LWL_HDR_SYNC2_MASK   0x00400000             /* Mask for one of ext synch bits */
#define LWL_HDR_SYNC1_MASK   0x00200000             /* Mask for one of ext synch bits */
#define LWL_HDR_SYNC0_MASK   0x00100000             /* Mask for one of ext synch bits */
#define LWL_HDR_TS_MASK      0x000fffff             /* Mask for TSI Time Stamp */

#define LWL_FIFO_EMPTY       0x1e000000             /* FIFO Empty */

#define LWL_TOF_C1           0x01000000             /* TOF-Mode 1 chan dgrm hdr */
#define LWL_TOF_C2           0x02000000             /* TOF-Mode 2 chan dgrm hdr */
#define LWL_TOF_C3           0x03000000             /* TOF-Mode 3 chan dgrm hdr */
#define LWL_TOF_C4           0x04000000             /* TOF-Mode 4 chan dgrm hdr */
#define LWL_TOF_C5           0x05000000             /* TOF-Mode 5 chan dgrm hdr */
#define LWL_TOF_C6           0x06000000             /* TOF-Mode 6 chan dgrm hdr */
#define LWL_TOF_C7           0x07000000             /* TOF-Mode 7 chan dgrm hdr */
#define LWL_TOF_C8           0x08000000             /* TOF-Mode 8 chan dgrm hdr */
#define LWL_TOF_C9           0x09000000             /* TOF-Mode 9 chan dgrm hdr */
#define LWL_TOF_C10          0x0A000000             /* TOF-Mode 10 chan dgrm hdr */

#define LWL_HM_NC            0x10000000             /* Hist-Mode/No-Coinc 0 chan dgrm hdr */
#define LWL_HM_NC_C1         0x11000000             /* Hist-Mode/No-Coinc 1 chan dgrm hdr */
#define LWL_HM_NC_C2         0x12000000             /* Hist-Mode/No-Coinc 2 chan dgrm hdr */
#define LWL_HM_NC_C3         0x13000000             /* Hist-Mode/No-Coinc 3 chan dgrm hdr */
#define LWL_HM_NC_C4         0x14000000             /* Hist-Mode/No-Coinc 4 chan dgrm hdr */
#define LWL_HM_NC_C5         0x15000000             /* Hist-Mode/No-Coinc 5 chan dgrm hdr */
#define LWL_HM_NC_C6         0x16000000             /* Hist-Mode/No-Coinc 6 chan dgrm hdr */
#define LWL_HM_NC_C7         0x17000000             /* Hist-Mode/No-Coinc 7 chan dgrm hdr */
#define LWL_HM_NC_C8         0x18000000             /* Hist-Mode/No-Coinc 8 chan dgrm hdr */
#define LWL_HM_NC_C9         0x19000000             /* Hist-Mode/No-Coinc 9 chan dgrm hdr */
#define LWL_HM_NC_C10        0x1A000000             /* Hist-Mode/No-Coinc 10 chan dgrm hdr */

#define LWL_GUMMI_MON        0x14000000             /* GUMMI-Mode Monitor datagram */

#define LWL_PSD_DATA         0x12000000             /* PSD-mode data datagram */
#define LWL_SANS2_DATA       0x12000000             /* SANS2-mode data datagram */

/* PSD-mode timing  status info */
#define LWL_PSD_FLASH_MASK 0x00FF                   /* mask for flash count     */
#define LWL_PSD_XORF1      0x2000                   /* mask for TDC-XORF1 bit */
#define LWL_PSD_GOVFL      0x1000                   /* mask for TDC-GOVFL bit */
#define LWL_PSD_RUNNING    0x0800                   /* mask for TDC-RUNNING bit */
#define LWL_PSD_ARMED      0x0400                   /* mask for TDC-ARMED bit */
#define LWL_PSD_PLL        0x0200                   /* mask for TDC-PLL bit */
#define LWL_PSD_CONF       0x0100                   /* mask for TDC-CONF bit */


/* Telegram Header */

#define HEADER_FLAG          0xFFFF0000


#define LWL_HEADER_DS4       0x10000000
#define LWL_HEADER_DS3       0x08000000
#define LWL_HEADER_DS2       0x04000000
#define LWL_HEADER_DS1       0x02000000
#define LWL_HEADER_DS0       0x01000000
#define LWL_HEADER_DS        0x1F000000

#define LWL_HEADER_TSI_L2    0x1F000000
#define LWL_HEADER_TSI_L4    0x0E000000
#define LWL_HEADER_TSI_SANS2 0x0D000000

#define LWL_STATUS_PF        0x80
#define LWL_STATUS_SWC       0x40
#define LWL_STATUS_NRL       0x20

#define LWL_STATUS_VLNT      0x08
#define LWL_STATUS_FF        0x04
#define LWL_STATUS_HF        0x02
#define LWL_STATUS_EF        0x01


#define MAX_PACKET_LENGTH 36


#define VAL_IS_HEADER(VAL) ((VAL) & HEADER_FLAG)
#define FIFO_IS_EMPTY(VAL) ((VAL) == LWL_FIFO_EMPTY)

/*******************************************************************************
  status and error codes
*******************************************************************************/

#define STATUS_FIFO_EMPTY   (-1)
#define STATUS_PACKET_AVAIL  (1)
#define STATUS_NO_PACKET     (0)


/*******************************************************************************
  type definitions
*******************************************************************************/

typedef unsigned int lwl_pmc_val_type;

typedef struct
{
  unsigned int ptr;
  unsigned int length;
  unsigned int header_found;    // this is used to supress errors until the first
                                // header has been found after the first start
                                // or an oversized packet or after a fifo_empty call
                                // in case of ov_chk version, this is used to
                                // temporarely store the next found header

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 11:24:29 2017
#ifndef HAVE_0MQ
  unsigned int data[MAX_PACKET_LENGTH];
#else
  unsigned int *data;
#endif
} packet_type;

/*******************************************************************************
  global vars
*******************************************************************************/

#ifdef FILLER_USER
extern char lwl_fifo_pipe_name[256];
#endif

/*******************************************************************************
  function prototypes
*******************************************************************************/

__INLINE lwl_pmc_val_type lwl_status_get(void);
__INLINE lwl_pmc_val_type lwl_value_get(void);
__INLINE unsigned int packet_len(lwl_pmc_val_type header);
__INLINE void packet_init(packet_type *p);
__INLINE int packet_get(packet_type *p);
__INLINE int packet_get_ov_chk(packet_type *p);
__INLINE int  empty_fifo(unsigned long max_tics);
__INLINE void empty_fifo_period(unsigned long max_tics);

int  snprint_packet_info(char* buffer,int bufferlen, char* pre, packet_type *p);
int  pmc_module_init(void);
void pmc_module_close(void);

/*******************************************************************************
  inlining
*******************************************************************************/

#ifdef _INLINE
#include "lwlpmc.c"
#endif


/******************************************************************************/

#endif                                                      /* _LWLPMC_H_ */
