/*******************************************************************************
  sinqhmsrv.h
*******************************************************************************/

/**
 * This file belongs to the SINQ histogram memory implementation for
 * LINUX-RTAI. This modules conatins:
 * - Configuration routines
 * - DAQ control routines
 * - Data access routines
 * - A translator from numeric error codes to text descriptions.
 * 
 * Mark Koennecke, Gerd Theidel, September 2005, February 2006
 */ 

#ifndef _SINQHMSRV_H_
#define _SINQHMSRV_H_

#include "nxdataset.h"

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

/**
 * error codes
 */

#define XMLPARSEERROR   -2000
#define BADXMLFILE      -2002
#define NOFILLER        -2003
#define FILLERUNKNOWN   -2004
#define NOBANKS         -2005
#define NOMEMORY        -2006
#define NOAXIS          -2007
#define NOAXISLENGTH    -2008
#define NOAXISMAPPING   -2009
#define NOOFFSET        -2010
#define NODIVISOR       -2011
#define BADMAPPING      -2012
#define BADFILE         -2013
#define AXARRAYTOSHORT  -2014
#define SYSTEMERROR     -2030
#define BADLENGTH       -2031
#define MAPMUSTBESECOND -2032
#define NOARRAYRANK     -2033
#define NOARRAYDIM      -2034
#define NOTENOUGHDIM    -2035
#define ARRAYSHORT      -2036
#define ARRAYMISSING    -2037
#define NOSHMMEMORY     -2038
#define CPARRAYERROR    -2039

/*******************************************************************************
  function prototypes
*******************************************************************************/

/**
 * configure the data shared memory
 * @param configBuffer holds the content of the XML data file used
 * for configuring the histogram memory.
 * @return a negative error code on failure.
 */
int configureHistogramMemory(char *configBuffer, int test_only);


/**
 * configure the histogram mmeory from an XML file
 * @param filename The filename holding the configuration data
 * @return a negative error code on failure, 1  else
 */
int configureHistogramMemoryFromFile(char *filename, int test_only);


/**
 * getTransferData reads the appropriate bit of histogram memory information 
 * and converts it into network byte order.
 * @param bank The bank for which to read data
 * @param start The start of the data to read
 * @param end The end of the data to read
 * @param data A newly allocated buffer holding the data in network byte order
 * in the unlikely case of a success. The caller is responsible for freeing
 * the data after use.  
 * @return 1 on success, a negative error code in the case of failures
 */
int getTransferData(int bank, int start, int end, int**data); 


/**
 * stop data aquisistion. Implements the server side of the protocoll
 * to the filler program
 * @return 1 on success, a negative error code on failure
 */
int stopDAQ(void);


/**
 * start data aquisition. Implements the control server side of
 * the protocoll to the filler process.
 * @return 1 on success, a negative error code on failure
 */
int startDAQ(void);


int pauseDAQ(void);


/**
 * dataErrorToText retrieves a test representation of an error code
 * @param errorCode
 * @param buffer The buffer to copy the text to
 * @param buflen The max length of character to write to buffer
 */
void dataErrorToText(int errorCode, char *buffer, int buflen);

/**
 * get a dataset which has been processed according the commands in command.
 * The command string is a semicolon separated list of subcommands. Currently
 * supported are: 
 *     sample:dim0start:dim0end,...
 *        for subsampling the HM data. satrt and end dimensions must be given
 *        for each dimension, separated by colons
 *     sum:dimno:start:end
 *        for summing along the dimension dimno from start to end
 * If multiple commands are give, the commands always act upon the 
 * the result of the previous command or the HM data if it is the first.
 * @param bankno The bank on which to operate. 
 * @param command A command string for processing HM data
 * @param error A string which will be filled with error information
 * if things go wrong.
 * @param errLen The size of error.
 * @return A NXdataset with processed data in case of success, NULL else
 */

pNXDS processData(int bankno, char *command, char *error, int errLen);

int resetBuff(void);
 

/******************************************************************************/

#endif //_SINQHMSRC_H_
