/**
 * This file belongs to the SINQ histogram memory software for Linux-RTAI
 * This file defines the handshaking between the http server and the
 * filler program.
 * 
 * Mark Koennecke, Gerd Theidel, September 2005, January 2006
 */ 
#include <stdio.h>
#include <string.h>
#include "datashm.h"
#include "controlshm.h"
#include "fillerhandshake.h"
/*------------------------------------------------------------------------*/
static void clearHM(){
  size_t size;

  size = (CFG_FIL_CLEAR_EACH_DAQ_END-CFG_FIL_CLEAR_EACH_DAQ_START+1)*sizeof(uint32);
  memset((void*)getVarPointer(CFG_FIL_CLEAR_EACH_DAQ_START),0,size);

  memset((void*)getHistDataPtr(), 0, getHistoDataSize());
}
/*-------------------------------------------------------------------------*/
__INLINE int doHandshake(){
	int val, valAck, status, pause;
	histo_descr_type *shmPtr = NULL;
	
	/*
	 * first DAQ
	 */
	getControlVar(CFG_SRV_DO_DAQ_CMD,&val);
	getControlVar(CFG_FIL_DO_DAQ_ACK,&valAck);
	getControlVar(CFG_SRV_DAQ_PAUSE_CMD,&pause);
	/*
	 * normal DAQ
	 */
	if(val == 1 && valAck == 1) {
	  if(pause == 1){
	    return 2;
	  } else {
	    return 1;
	  }
	}
	/**
	 * request DAQ to start
	 */
	if(val == 1 && valAck == 0){
		setControlVar(CFG_FIL_DO_DAQ_ACK,1);
		return 1;
	}
	/*
	 * request DAQ to stop
	 */
	if(val == 0 && valAck == 1){
		setControlVar(CFG_FIL_DO_DAQ_ACK,0);
		status = 0;
	} 
	/*
	 * DAQ is stopped
	 */
	if(val == 0 && valAck == 0){
		status = 0;
	}
	
	/**
	 * configuration can only happen when DAQ ist stopped. So check
	 * here for configuration things.
	 */
	getControlVar(CFG_SRV_DO_CFG_CMD,&val);
	getControlVar(CFG_FIL_DO_CFG_ACK,&valAck);
	/*
	 * no configuration request, or configuration in process
	 */
	if(val == 1 && valAck == 1){
		return 0;
	}
	if(val == 0 && valAck == 0){
		return 0;
	}
	/*
	 * controller wants to configure: release data shared memory
	 */
	if(val == 0  && valAck == 1){
		setControlVar(CFG_FIL_DO_CFG_ACK,0);
		return 0;
	}
	/*
	 * controller has finished configuration, reconnect to data shared
	 * memory
	 */
	if(val == 1 && valAck == 0){
		setControlVar(CFG_FIL_DO_CFG_ACK,1);
		shmPtr = getShmHistoPtr();
		shmPtr->filler_valid = DATASHM_CFG_FIL_VALID;
		return -1;
	}
	
	return status; 
}
/*-------------------------------------------------------------------------*/
__INLINE int isDataConfigured(){
	int val, valAck;
	
	getControlVar(CFG_SRV_DO_CFG_CMD,&val);
	getControlVar(CFG_FIL_DO_CFG_ACK,&valAck);
	
	if(val == 1 && valAck == 1){
		return 1;
	} else {
		return 0;
	}
}
