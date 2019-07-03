/**
 * This file belongs to the SINQ histogram memory software for Linux-RTAI
 * This file defines the handshaking between the http server and the
 * fille process.
 * 
 * Mark Koennecke, Gerd Theidel, September 2005
 */ 
#ifndef _FILLERHANDSHAKE_H_
#define _FILLERHANDSHAKE_H_

/*
 * doHandshake tests and performs all the hand shaking between the filler
 * and the control server.
 * @return 1 when we should aquire data, 0 when not aquiring data, -1 when 
 * the configuration has changed
 */
int doHandshake();
/**
 * test if data shared memory is configured
 * @return 1 if data is configured, 0 else
 */
int isDataConfigured();

#ifdef INLINING
#define __INLINE static inline
#include "fillerhandshake.c"
#else
#define __INLINE
#endif

#endif //_FILLERHANDSHAKE_H_
