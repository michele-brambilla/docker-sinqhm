/*******************************************************************************
  sinqhmlog.h
*******************************************************************************/
/**
 * This file belongs to the SINQ histogram memory implementation for 
 * LINUX-RTAI. We need some form of logging. We define a logging function
 * here, different interface adapters to whatever server is used may choose 
 * implement this differently.
 * 
 * Mark Koennecke, Gerd Theidel, September 2005
 */

#ifndef _SINQHMLOG_H_
#define _SINQHMLOG_H_

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Mon Apr 24 14:24:35 2017
#include "log.h"
#include "mongo.h"

/*******************************************************************************
  function prototypes
*******************************************************************************/

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Mon Apr 24 14:24:38 2017
/* void sinqhmlog(char *fmt, ...); */

void sinqhmlog(const int level, const char* subsystem, const char * msg);
void sinqhmlog_cleanup();

/******************************************************************************/

#endif //_SINQHMLOG_H_
