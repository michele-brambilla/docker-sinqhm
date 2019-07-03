/*******************************************************************************
  debugshm.c
*******************************************************************************/

/**
 * This file belongs to the SINQ Histogram Memory implementation for
 * LINUX-RTAI. SINQHM-RTAI uses two shared memory sections: one for control
 * information and another one where the bank information and the actual
 * histogram is stored. This file describes the interface to the control
 * section of SINQHM-RTAI. The idea is that the knowledge about the layout
 * of the control shared memory stays within these files.
 *
 * Gerd Theidel, Mark Koennecke, September 2005
 */

/*******************************************************************************
  includes
*******************************************************************************/

#if (!defined APP_FILLER) || (!defined FILLER_RTAI)
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#endif
//#include <sys/time.h>
//#include <time.h>


#include "sinqhm_errors.h"
#include "debugshm.h"
#include "controlshm.h"
#include "datashm.h"
#include "lwlpmc.h"
#include "needfullthings.h"

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/


/*******************************************************************************
  global vars
*******************************************************************************/

#ifndef _INLINE
/**
 * the pointer to real data
 */
volatile unsigned int *debugBuffPtr = NULL;
volatile unsigned int * shm_dbg_ptr = NULL;

#else

extern volatile unsigned int *debugBuffPtr;
extern volatile unsigned int * shm_dbg_ptr;

#endif

static int debugID = -1;

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   initShmDebug
 *
 ******************************************************************************/

__INLINE int initShmDebug(void)
{
  debugBuffPtr = (volatile unsigned int*) sinqhm_shm_malloc(SHM_KEY_DEBUG_BUFF, &debugID, dbg_size);

  if(debugBuffPtr == NULL)
  {
    return SINQHM_ERR_ALLOCFAIL;
  }
  else
  {
    return 0;
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   releaseShmDebug
 *
 ******************************************************************************/

__INLINE void releaseShmDebug(void)
{
  if (debugBuffPtr)
  {
    sinqhm_shm_free(SHM_KEY_DEBUG_BUFF, debugID, (void*)debugBuffPtr);
    debugBuffPtr=NULL;
  }
}




/*******************************************************************************
 *
 * FUNCTION
 *   getDebugBuffPtr
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

__INLINE volatile unsigned int *getDebugBuffPtr(void)
{
  return debugBuffPtr;
}


/******************************************************************************/

#ifndef _INLINE


/*******************************************************************************
 *
 * FUNCTION
 *   datetimestr
 *
 ******************************************************************************/

int datetimestr (char* buff, int len)
{
  struct timeval tv;
  int printed_len;

#if (defined APP_FILLER) && (defined FILLER_RTAI)
  struct rtc_time tm;

  do_gettimeofday(&tv);
  to_tm(tv.tv_sec, &tm);
  printed_len = snprintf(buff,len,"[%4d/%02d/%02d %02d:%02d:%02d.%06d UTC] ",tm.tm_year,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec,(unsigned int)tv.tv_usec);

#else

  struct tm  mytm;
  gettimeofday(&tv,0);
//  gmtime_r(&(tv.tv_sec), &mytm);
  localtime_r(&(tv.tv_sec), &mytm);
  printed_len = snprintf(buff,len,"[%4d/%02d/%02d %02d:%02d:%02d.%06d %s] ",1900+mytm.tm_year,1+mytm.tm_mon,mytm.tm_mday,mytm.tm_hour,mytm.tm_min,mytm.tm_sec,(unsigned int)tv.tv_usec, mytm.tm_zone);

//  printed_len += snprintf(buff+printed_len,len-printed_len,"[%4d/%02d/%02d %02d:%02d:%02d.%06d localtime mytm=%d  %s] ",1900+mytm.tm_year,1+mytm.tm_mon,mytm.tm_mday,mytm.tm_hour,mytm.tm_min,mytm.tm_sec,(unsigned int)tv.tv_usec,mytm.tm_isdst, mytm.tm_zone);

#endif

  return printed_len;
}


#ifdef APP_FILLER

/*******************************************************************************
 *
 * FUNCTION
 *   init_dbg_print_buff
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

void init_dbg_print_buff(void)
{
  if(shm_dbg_ptr)
  {
    shm_dbg_ptr[DBG_FIL_CHAR_BUFF_START] = 0;
    shm_dbg_ptr[DBG_FIL_BUFF_SIZE]       = dbg_size;
    shm_dbg_ptr[DBG_FIL_STR_PTR]         = DBG_FIL_CHAR_BUFF_START<<2;
    shm_dbg_ptr[DBG_FIL_WRAP_AROUND]     = 0;
    shm_dbg_ptr[DBG_FIL_LOG_NUMBER]      = 0;
  }
}



/*******************************************************************************
 *
 * FUNCTION
 *   set_error_msg
 *
 ******************************************************************************/

void set_error_msg(char *buff, int len)
{
  char *err_buff_char_ptr;
  int count, bytes_free;

  if (!shm_dbg_ptr) return;

  err_buff_char_ptr = (char*)&shm_dbg_ptr[DBG_FIL_ERR_BUFF_START];

  if (shm_dbg_ptr[DBG_FIL_ERR_BUFF_SGN] == shm_dbg_ptr[DBG_SRV_ERR_BUFF_ACK])
  {
    // clear Error Buff;
    shm_dbg_ptr[DBG_FIL_ERR_BUFF_PTR] = 0;
    shm_dbg_ptr[DBG_FIL_ERR_BUFF_SGN]++;
    err_buff_char_ptr[0]=0;
  }
  bytes_free = shm_dbg_ptr[DBG_FIL_ERR_BUFF_SIZE]-shm_dbg_ptr[DBG_FIL_ERR_BUFF_PTR]-1;

  if (bytes_free <=0) return;

  count = my_min_int(len,bytes_free);
  if (count > 0)
  {
    memcpy(&err_buff_char_ptr[shm_dbg_ptr[DBG_FIL_ERR_BUFF_PTR]],buff,count);
    shm_dbg_ptr[DBG_FIL_ERR_BUFF_PTR]+=count;
  }

  err_buff_char_ptr[shm_dbg_ptr[DBG_FIL_ERR_BUFF_PTR]]=0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   set_debug_msg
 *
 ******************************************************************************/

int set_debug_msg(char *buff, int len)
{
  char *shm_dbg_char_ptr;
  char *src_ptr;
  int count,todo;
  int buff_free_bytes;
  unsigned int temp_uint32;

  if (!shm_dbg_ptr) return 0;

  shm_dbg_ptr[DBG_FIL_LOG_NUMBER]++;

  shm_dbg_char_ptr = (char*)shm_dbg_ptr;
  buff_free_bytes = shm_dbg_ptr[DBG_FIL_BUFF_SIZE]-1-shm_dbg_ptr[DBG_FIL_STR_PTR];
  src_ptr = buff;
  todo    = len;

  do
  {
    buff_free_bytes = shm_dbg_ptr[DBG_FIL_BUFF_SIZE]-1-shm_dbg_ptr[DBG_FIL_STR_PTR];
    count = my_min_int(todo,buff_free_bytes);
    if (count > 0)
    {
      memcpy(&shm_dbg_char_ptr[shm_dbg_ptr[DBG_FIL_STR_PTR]],src_ptr,count);
      todo -=count;
      shm_dbg_ptr[DBG_FIL_STR_PTR]+=count;
    }
    if ((!shm_dbg_ptr[DBG_SRV_CHAR_BUFF_ONCE]) && (shm_dbg_ptr[DBG_FIL_STR_PTR]>=(shm_dbg_ptr[DBG_FIL_BUFF_SIZE]-1)))
    {
      shm_dbg_char_ptr[shm_dbg_ptr[DBG_FIL_BUFF_SIZE]-1] = 0;
      shm_dbg_ptr[DBG_FIL_STR_PTR] = DBG_FIL_CHAR_BUFF_START<<2;

      temp_uint32 = shm_dbg_ptr[DBG_FIL_WRAP_AROUND]+1;
      if (temp_uint32) shm_dbg_ptr[DBG_FIL_WRAP_AROUND]=temp_uint32;
    }
    src_ptr+=count;
  }
  while (todo >0 && (shm_dbg_ptr[DBG_FIL_STR_PTR]<(shm_dbg_ptr[DBG_FIL_BUFF_SIZE]-1)) );

  if (!(shm_dbg_ptr[DBG_FIL_WRAP_AROUND]))
  {
    shm_dbg_char_ptr[shm_dbg_ptr[DBG_FIL_STR_PTR]]=0;
  }
  else
  {
    shm_dbg_char_ptr[shm_dbg_ptr[DBG_FIL_STR_PTR]]=13;
  }

  return len-todo;

}


/*******************************************************************************
 *
 * FUNCTION
 *   dbg_printf
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int dbg_printf(int level, const char *fmt, ...)
{
  va_list args;
  int printed_len=0, total_len=0, html_len, time_len;
  static char temp_buf[DGB_MAX_MESSAGE_LENGTH+1];
  static char html_buf[100];
  static char time_buf[100];


  if (!shm_dbg_ptr) return 0;
  if ((level != DBGMSG_ERROR) && (level>shm_dbg_ptr[DBG_SRV_MSG_LEVEL])) return 0;

  time_len = datetimestr(time_buf, 100);

  va_start(args, fmt);
  printed_len += vsnprintf(&temp_buf[printed_len], DGB_MAX_MESSAGE_LENGTH-printed_len, fmt, args);
  va_end(args);

  if (level == DBGMSG_ERROR)
  {
    set_error_msg(time_buf,time_len);
    set_error_msg(temp_buf,printed_len);
  }


  if (level>shm_dbg_ptr[DBG_SRV_MSG_LEVEL]) return 0;


  total_len += set_debug_msg(time_buf,time_len);

  if (level == DBGMSG_ERROR)
  {
    html_len = snprintf(html_buf, 100, "<span class=\"DBGMSG_ERROR\">ERROR: ");
    total_len += set_debug_msg(html_buf,html_len);
  }
  else if (level == DBGMSG_WARNING)
  {
    html_len = snprintf(html_buf, 100, "<span class=\"DBGMSG_WARNING\">WARNING: ");
    total_len += set_debug_msg(html_buf,html_len);
  }
  else
  {
    html_len = snprintf(html_buf, 100, "<span class=\"DBGMSG_INFO\">");
    total_len += set_debug_msg(html_buf,html_len);
  }

  total_len += set_debug_msg(temp_buf,printed_len);

  html_len = snprintf(html_buf, 100, "</span>");
  total_len += set_debug_msg(html_buf,html_len);

  return total_len;

}

/*******************************************************************************
 *
 * FUNCTION
 *   print_array
 *
 ******************************************************************************/

#define PPRINT     printed_len += snprintf
#define PPBUFFLEN  256
#define PPBUFF     &buffer[printed_len], (PPBUFFLEN-printed_len)

void print_array(int level, volatile uint32* arrayptr)
{
  int printed_len=0, length, rank, count, i;
  char buffer[PPBUFFLEN];


  if(!arrayptr) return;

  rank = arrayptr[0];

  PPRINT(PPBUFF,"Array: rank=%d  dim=[ ",rank);

  length = 1;
  for(i = 0; i < rank; i++)
  {
    PPRINT(PPBUFF,"%d ",arrayptr[i+1]);
	  length *= arrayptr[i+1];
  }

  PPRINT(PPBUFF,"]\n");
  dbg_printf(level, buffer);
  printed_len=0;
  buffer[0]=0;


  count = 0;
  for(i = 0; i  < length; i++)
  {
    PPRINT(PPBUFF,"%d ",arrayptr[1+rank+i]);
    count++;
    if((count >= 10) || (i==(length -1)))
    {
      dbg_printf(level,"%s\n",buffer);
      printed_len=0;
      count = 0;
    }
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   print_config
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

void print_config(void)
{
  int length,  mapping;
  char buffer[256];
  int nbank, bank;
  int naxis, axis;
  int filler;
  volatile uint32           *bmap_arraydesc_ptr;
  volatile histo_descr_type *histo_descr_ptr;
  volatile bank_descr_type  *bank_descr_ptr;
  volatile axis_descr_type  *axis_descr_ptr;
  volatile uint32 *axisData_ptr;

  dbg_printf(DBGMSG_INFO3, "Histogram Memory Configuration\n");
  histo_descr_ptr = getShmHistoPtr();

  if(!histo_descr_ptr)
  {
    dbg_printf(DBGMSG_ERROR, "print_config(): can not get data pointer\n");
  }

  filler=histo_descr_ptr->histo_type;

  switch(filler)
  {
    case FILLERTOF:
      strcpy(buffer,"Time-Of-Flight");
      break;
    case FILLERPSD:
      strcpy(buffer,"EMBL-PSD-Filler");
      break;
    case FILLERDIG:
      strcpy(buffer,"Standard Filler");
      break;
    case FILLERTOFMAP:
      strcpy(buffer,"2D TOF with Lookup Table");
      break;
    case FILLERHRPT:
      strcpy(buffer,"Cerca HRPT Frame Mode");
      break;
    case FILLERSANS2:
      strcpy(buffer,"Sans2-PSD Filler");
      break;
    case FILLERRITA2:
    case FILLERCAMEA:
      strcpy(buffer,"0MQ Filler");
    break;
  default:
      snprintf(buffer,255,"Unknown filler ID %d",filler);
      break;
  }

  dbg_printf(DBGMSG_INFO3, "Filler Type: %s (ID=%d)\n",buffer,filler);

  if (histo_descr_ptr->bank_mapping_array.offs)
  {
    bmap_arraydesc_ptr = (uint32*)dataShmOffsToPtr(histo_descr_ptr->bank_mapping_array.offs);
    dbg_printf(DBGMSG_INFO4, "Bank Mapping Array:\n");
    print_array(DBGMSG_INFO4, bmap_arraydesc_ptr);
  }

  nbank = histo_descr_ptr->nBank;
  for(bank = 0; bank < nbank; bank++)
  {
    bank_descr_ptr = getBankDescription(bank);
    naxis = bank_descr_ptr->rank;
    dbg_printf(DBGMSG_INFO3, "Bank %d: rank=%d\n",bank ,naxis);

    for(axis = 0; axis < naxis; axis++)
    {
      axis_descr_ptr = getAxisDescription(bank, axis);
      length = axis_descr_ptr->length;
      mapping = axis_descr_ptr->type;

      dbg_printf(DBGMSG_INFO3, "Axis %d: length=%d\n", axis,length);

      axisData_ptr = getAxisData(axis_descr_ptr);
      switch(mapping)
      {
      case AXDIRECT:
	dbg_printf(DBGMSG_INFO3, "Mapping: Direct (ID=%d)\n",mapping);
	break;

      case AXCALC:
	dbg_printf(DBGMSG_INFO3, "Mapping: Calculation (ID=%d)\n",mapping);
	if(axisData_ptr != NULL)
          {
            dbg_printf(DBGMSG_INFO3, "Multiplier  : %d\n", axisData_ptr[0]);
            dbg_printf(DBGMSG_INFO3, "PreOffset   : %d\n", axisData_ptr[1]);
            dbg_printf(DBGMSG_INFO3, "Divisor     : %d\n", axisData_ptr[2]);
            dbg_printf(DBGMSG_INFO3, "PostOffset  : %d\n", axisData_ptr[3]);
          }
	break;

      case AXBOUNDARY:
	dbg_printf(DBGMSG_INFO4, "Mapping: Boundary Array (ID=%d)\n",mapping);
	print_array(DBGMSG_INFO4, axisData_ptr);

	break;
      case AXLOOKUP:
	dbg_printf(DBGMSG_INFO4, "Mapping: Lookup Table (ID=%d)\n",mapping);
	print_array(DBGMSG_INFO4, axisData_ptr);
          break;

      case AXMAP:
	dbg_printf(DBGMSG_INFO4, "Mapping: Map Table (ID=%d)\n",mapping);
	print_array(DBGMSG_INFO4, axisData_ptr);

          break;

        default:
          dbg_printf(DBGMSG_INFO3, "Mapping: Unknown (ID=%d)\n",mapping);
          break;
      }
    }
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   print_lwl_status
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

int print_lwl_status (unsigned int status)
{
  dbg_printf(DBGMSG_INFO3, "LWL_Status [0x%02x] : %s %s %s %s %s %s %s\n",status,
    ((status & LWL_STATUS_PF)   ? "PF " : "   "),
    ((status & LWL_STATUS_SWC)  ? "SWC" : "   "),
    ((status & LWL_STATUS_NRL)  ? "NRL" : "   "),
    ((status & LWL_STATUS_VLNT) ? "ERR" : "   "),
    ((status & LWL_STATUS_FF)   ? "FF " : "   "),
    ((status & LWL_STATUS_HF)   ? "HF " : "   "),
    ((status & LWL_STATUS_EF)   ? "EF " : "   "));

  return 0;
}


/*******************************************************************************
 *
 * FUNCTION
 *   print_packet
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

void print_packet(packet_type *p)
{
  int i;
  char buff[256];
  int printed_len=0;

  if (shm_dbg_ptr[DBG_SRV_MSG_LEVEL] < DBGMSG_INFO5) return;

  printed_len += snprintf(&buff[printed_len], 256-printed_len, "Packet found (len=%2d): 0x%08x", p->length,p->data[0]);

  for (i=1; ((i < p->length) && (i < MAX_PACKET_LENGTH));i++)
  {
    printed_len += snprintf(&buff[printed_len], 256-printed_len, " 0x%04x", p->data[i]);
  }
  printed_len += snprintf(&buff[printed_len], 256-printed_len, "\n");
  dbg_printf(DBGMSG_INFO5,buff);
}

/******************************************************************************/

#endif // RT

#endif /* not _INLINE */
