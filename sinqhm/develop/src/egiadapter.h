/*******************************************************************************
  egiadapter.h
*******************************************************************************/

/**
 * This file belongs to the SINQ histogram memory code for Linux-RTAI.
 * This file contains the interface between the more general SINQ code
 * and the EGI interface of the appWeb http server.
 * 
 * Mark Koennecke, Gerd Theidel, September 2005
 */ 

#ifndef _EGIADAPTER_H_
#define _EGIADAPTER_H_


/*******************************************************************************
  includes
*******************************************************************************/

#include "controlshm.h"
#include "datashm.h"


/*******************************************************************************
  function prototypes
*******************************************************************************/

/**
 * initialize everything for us
 * @return a negative error code on problems, 1 else
 */
int initializeSINQHM(void);
/**
 * drop all connections to the shared memory and clean up.
 */
int closeSINQHM(void);


void openLog(char* filename, int size);
void closeLog(void);



/******************************************************************************/

#endif //_EGIADAPTER_H_
