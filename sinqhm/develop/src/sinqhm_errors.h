/*******************************************************************************
  sinqhm_errors.h
*******************************************************************************/

#ifndef _SINQHM_ERRORS_H_
#define _SINQHM_ERRORS_H_

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/


#define SINQHM_OK                               0

/**
 * error codes
 */

#define SINQHM_ERR_ALLOCFAIL                -1001
#define SINQHM_ERR_NOTCONNECTED             -1002 
#define SINQHM_ERR_NOTFOUND                 -1003

#define SINQHM_ERR_NOTCONFIGURED            -2001
#define SINQHM_ERR_NOSUCHBANK               -2015
#define SINQHM_ERR_BADCONFIG                -2016
#define SINQHM_ERR_FILLERTIMEOUT            -2017


#define SINQHM_ERR_EMPTY_FIFO               -8000

#define SINQHM_ERR_FIL_MALLOC_FAILED        -8001

#define SINQHM_ERR_VAL_LOW                  -8100
#define SINQHM_ERR_VAL_HIGH                 -8101

#define SINQHM_ERR_NO_VALID_CONFIG          -8200
#define SINQHM_ERR_NO_HISTO_DESCR_PTR       -8201
#define SINQHM_ERR_NO_HISTO_DATA_PTR        -8202
#define SINQHM_ERR_NO_BANK_DESCR_PTR        -8203
#define SINQHM_ERR_NO_BANK_DATA_PTR         -8204
#define SINQHM_ERR_NO_BANK_MAPPING_ARRAY    -8205
#define SINQHM_ERR_NO_AXIS_DESCR_PTR        -8206
#define SINQHM_ERR_UNKNOWN_AXIS_MAPPING     -8207
#define SINQHM_ERR_WRONG_AXIS_MAPPING       -8208
#define SINQHM_ERR_UNKNOWN_FILLER_TYPE      -8209
#define SINQHM_ERR_WRONG_NUMBER_OF_BANKS    -8210
#define SINQHM_ERR_WRONG_NUMBER_OF_AXIS     -8211



/******************************************************************************/

#endif // _SINQHM_ERRORS_H_
