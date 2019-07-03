/*******************************************************************************
  process_common.h
*******************************************************************************/

#ifndef _PROCESS_COMMON_H_
#define _PROCESS_COMMON_H_

/*******************************************************************************
  includes
*******************************************************************************/

#include "lwlpmc.h"
#include "sinqhm_types.h"
#include "datashm.h"


/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define DO_RANGE_CHECK  1
#define NO_CHECK        0

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 12:16:22 2017
#define MAX_TIMESTAMP  -1

/*******************************************************************************
  type definitions
*******************************************************************************/

struct AXISTRUCT;

typedef int (*axismap_fcnptr_type)(int val, struct AXISTRUCT *axis);

typedef struct AXISTRUCT
{
  axismap_fcnptr_type fcn;
  uint32  len;
  uint32 *array;
  uint32  arraylen;
  uint32 *data;
  uint32  datalen;
  uint32  rangecheck;
  volatile uint32 *cnt_low_ptr;
  volatile uint32 *cnt_high_ptr;
  uint32 threshold;
  uint32 offset;
} process_axis_type;


/*******************************************************************************
  global vars
*******************************************************************************/

extern void (*process_packet_fcn)(packet_type*);
extern int  (*process_construct_fcn)(void);
extern void (*process_destruct_fcn)(void);
extern void (*process_init_daq_fcn)(void);

extern uint32  hdr_daq_mask;
extern uint32  hdr_daq_active;


/*******************************************************************************
  function prototypes
*******************************************************************************/

int  searchBinBoundary(unsigned int val, unsigned int *array, unsigned int arraySize);

void process_destruct(void);
void process_init_daq(void);
void process_leave_daq(void);

int  process_construct(void);
int  process_tsi(packet_type *p);

int  axismap_boundary(int val, process_axis_type *axis);
int  axismap_direct(int val, process_axis_type *axis);
int  SetAxisMapping(volatile axis_descr_type  *axis_descr_ptr, process_axis_type *axis, unsigned int rangecheck);

/*******************************************************************************
  different filler types
*******************************************************************************/

// #ifdef FILLER_DIG
int  process_dig_construct(void);
int  process_tof_construct(void);
int  process_psd_construct(void);
int  process_hrpt_construct(void);
int  process_tofmap_construct(void);
int  process_sans2_construct(void);
// 0mq processes
int  process_rita2_construct(void);
int  process_camea_construct(void);
uint mask_shift(uint value, uint mask, uint shift);

#endif /* _PROCESS_COMMON_H_ */
