/*******************************************************************************
  lwlpmc.c
*******************************************************************************/

/*******************************************************************************
  includes
*******************************************************************************/

#include "lwlpmc.h"
#include "rawdata.h"
#include "controlshm.h"
#include "needfullthings.h"
#include "sinqhm_errors.h"
#include "debugshm.h"


#ifdef APP_FILLER

#ifdef FILLER_RTAI
#include <linux/pci.h>
#include <rtai.h>
#include <rtai_sched.h>

#ifdef ARCH_X86
#include <asm-i386/byteorder.h>
#endif

#elif defined FILLER_USER
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
int lwl_fifo = -1;
char lwl_fifo_pipe_name[256] = "/tmp/lwlfifo";
#endif




#elif defined APP_SERVER

#include <stdio.h>
#include <stdlib.h>

#endif  // APP_FILLER


/*******************************************************************************
  global vars
*******************************************************************************/

#ifdef APP_FILLER

#ifndef _INLINE
volatile lwl_pmc_val_type *pmc_base_ptr;
#else
extern volatile lwl_pmc_val_type *pmc_base_ptr;
#endif

extern volatile unsigned int *shm_cfg_ptr ;

#endif  // APP_FILLER

/*******************************************************************************
  function declarations
*******************************************************************************/



/*******************************************************************************
 *
 * FUNCTION
 *   packet_len
 *
 * DESCRIPTION
 *   This function returns the number of PCI reads required for reading
 *   the whole packet including the read which has already been done
 *   to get the header.
 *
 * PARAMETERS
 *   header    header (32 bit) of packet
 *
 * RETURNS
 *   number of PCI read for complete packet
 *
 ******************************************************************************/

__INLINE unsigned int packet_len(lwl_pmc_val_type header)
{
  lwl_pmc_val_type header_type;

  /* use only DS4 .. DS0 bits for decision */
  header_type = header & LWL_HDR_TYPE_MASK;

  if (header_type == LWL_HEADER_TSI_L2 )
  {
    /* check this first, because DS4 is also set in this case */
    /* TSI which needs 2 PCI reads */
    return 2;
  }
  if (header_type == LWL_HEADER_TSI_SANS2 )
  {
    /* SANS2 TSI which needs 4 PCI reads */
    return 6;
  }
  else if (header_type == LWL_GUMMI_MON )
  {
    /* Warning :                                       */
    /* This header does not meet the stadard !!!       */
    /* The packet length conficts with the encoded     */
    /* length of headers with DS4 bit set.             */
    /* If LWL_HM_NC_C4 is used, it will get the wrong  */
    /* length !!!!                                     */
    return 7;
  }
  else if (header_type & LWL_HEADER_DS4 )
  {
    /* if DS4 is set the length is encoded in DS3..DS0 */
    /* the return value is the same as [DS3..DS0] + 1  */
    return ( ((header_type>>24) & 0x0f) + 1);
  }
  else if (header_type == LWL_HEADER_TSI_L4 )
  {
    /* TSI which needs 4 PCI reads */
    return 4;
  }
  else
  {
    /* all other packets should need 2 pci reads */
    /* if other packet structures are introduced */
    /* in future this has to be handled here     */
    return 2;
  }
}


#ifdef APP_FILLER

#ifdef FILLER_RTAI
/*******************************************************************************
 *   lwl_value_get
 ******************************************************************************/

__INLINE lwl_pmc_val_type lwl_value_get(void)
{
#ifdef ARCH_X86
   return  __arch__swab32(pmc_base_ptr[0]);
#else
  return  pmc_base_ptr[0];
#endif
}


/*******************************************************************************
 *   lwl_status_get
 ******************************************************************************/

__INLINE lwl_pmc_val_type lwl_status_get(void)
{
#ifdef ARCH_X86
  return  __arch__swab32(pmc_base_ptr[16]);
#else
  return  pmc_base_ptr[16];
#endif
}

#else

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/******************************************************************************/

#define LWL_SIM_BUFFER_SIZE 1024

char lwl_sim_buffer[LWL_SIM_BUFFER_SIZE];
int lwl_sim_ppos = -1;
int lwl_sim_spos = -1;
int lwl_sim_bytes_read = 0;
int lwl_sim_comment = 0;
int lwl_sim_status  = 0;

/******************************************************************************/

int is_hex_char(char c)
{
  return (     ((c >= '0') && (c <= '9'))
           ||  ((c >= 'a') && (c <= 'f'))
           ||  ((c >= 'A') && (c <= 'F'))
           ||   (c == 'x') 
           ||   (c == 'X') );
}

/******************************************************************************/

int is_seperator(char c)
{
  return ( (c == 9) || (c == 10) || (c == 13) || (c == ' ') || (c == '#') );
}

/******************************************************************************/

__INLINE lwl_pmc_val_type lwl_value_get(void)
{
  lwl_pmc_val_type value;
  int bytes_read;
  int i;

  if (lwl_fifo<0) return LWL_FIFO_EMPTY;

  if(lwl_sim_ppos<0)
  {
    bytes_read = read(lwl_fifo,&lwl_sim_buffer[lwl_sim_bytes_read],(LWL_SIM_BUFFER_SIZE-lwl_sim_bytes_read));
    if (bytes_read>0) 
    {
//      printf("lwl_got_data:>>%s<<\n",&lwl_sim_buffer[lwl_sim_bytes_read]);
      lwl_sim_bytes_read += bytes_read;
      lwl_sim_ppos=0;
    }
    else
    {
      return LWL_FIFO_EMPTY;
    }
  }
    
  while ( (lwl_sim_ppos>=0) && (lwl_sim_ppos<lwl_sim_bytes_read))
  {

    if (!lwl_sim_comment)
    {
      if (is_hex_char(lwl_sim_buffer[lwl_sim_ppos]))
      {
        if(lwl_sim_spos<0)
        {
          // found a new hex value
          lwl_sim_spos = lwl_sim_ppos;
        }
      }
      else if (is_seperator(lwl_sim_buffer[lwl_sim_ppos]))
      {
        if  (lwl_sim_spos >=0) 
        {
          if (lwl_sim_buffer[lwl_sim_ppos] == '#') lwl_sim_comment = 1;
          lwl_sim_buffer[lwl_sim_ppos]=0;
          sscanf(&lwl_sim_buffer[lwl_sim_spos],"%x",&value);
          lwl_sim_spos = -1;
          lwl_sim_ppos++;
//          printf("LWL FIFO: 0x%08x\n",value);
          return value;
        }
      }
      else
      {
       // syntax error - set error Flag
        lwl_sim_status |= LWL_STATUS_VLNT;
      }
    }

    if (lwl_sim_buffer[lwl_sim_ppos] == '#') lwl_sim_comment = 1;
    if (lwl_sim_buffer[lwl_sim_ppos] == 10 ) lwl_sim_comment = 0;
    if (lwl_sim_buffer[lwl_sim_ppos] == 13 ) lwl_sim_comment = 0;

    lwl_sim_ppos++;
  }

  if (lwl_sim_spos >= 0) 
  {
    lwl_sim_bytes_read -= lwl_sim_spos;
    for (i=0; i<lwl_sim_bytes_read; i++)
    {
      lwl_sim_buffer[i]=lwl_sim_buffer[i+lwl_sim_spos];
    }
  }
  else
  {
    lwl_sim_bytes_read = 0;
  }

  lwl_sim_ppos = -1;
  lwl_sim_spos = -1;
  return LWL_FIFO_EMPTY;

}

/******************************************************************************/

__INLINE lwl_pmc_val_type lwl_status_get(void)
{
  lwl_pmc_val_type status;

  status = lwl_sim_status;

  // reset stored Error Flags
  lwl_sim_status &= ~LWL_STATUS_VLNT;
  lwl_sim_status &= ~LWL_STATUS_FF;

  if (status)  printf("LWL STATUS: 0x%08x\n",status);

  return status;

}
#endif



/******************************************************************************/

__INLINE void packet_init(packet_type *p)
{
  p->header_found=0;
  p->ptr=0;
  p->length=2;
}


/*******************************************************************************
 *
 * FUNCTION
 *   packet_get
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





__INLINE int packet_get(packet_type *p)
{

  lwl_pmc_val_type value;

  value = lwl_value_get();
  if (FIFO_IS_EMPTY(value))
  {
    return STATUS_FIFO_EMPTY;
  }

  if (doStoreRawData) storeRawData(value);

  if (VAL_IS_HEADER(value))
  {
    /* the header uses all 32 bit */
    if (p->ptr)
    {
      /* a new header is received before all values of last packet have been read */
      /* (p->ptr is set to 0 when the packet is complete) */
      shm_cfg_ptr[CFG_FIL_TMP_PKG_INCOMPLETE]++;
    }

    /* start a new packet */
    p->data[0] = value;
    p->ptr = 1;
    p->header_found = 1;
    p->length = packet_len(value);

  }
  else
  {
    /* in the following data words only the lower 16 bit are used */
    if (p->ptr)
    {
      p->data[(p->ptr)++] = value;
    }
    else
    {
      /* received data word when header was expected */
      if (p->header_found)
      {
        /* singal errors only after a header was found */
        shm_cfg_ptr[CFG_FIL_TMP_PKG_OVERSIZE]++;
        p->header_found = 0;
      }
      return STATUS_NO_PACKET;
    }
  }

  if (p->ptr >= p->length)
  {
    /* packet complete */
    p->ptr=0;
    shm_cfg_ptr[CFG_FIL_TMP_PKG_READ]++;

    return STATUS_PACKET_AVAIL;
  }

  return STATUS_NO_PACKET;
}


/*******************************************************************************
 *
 * FUNCTION
 *   packet_get_ov_chk
 *
 * DESCRIPTION
 *   This version delays a packet of expected size until next header or 
 *   Fifo Empty is received to prevent processing of oversized packets.
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

__INLINE int packet_get_ov_chk(packet_type *p)
{

  lwl_pmc_val_type  value;

  if(p->header_found)
  {
    value = p->header_found;
    p->header_found=0;
  }
  else
  {
    value = lwl_value_get();

    if (doStoreRawData)
    {
      if (!FIFO_IS_EMPTY(value))
      {
        storeRawData(value);
      }
    }
  }

  if (VAL_IS_HEADER(value))
  {

    if (p->ptr == p->length)
    {
      /* no data left in fifo so assume that the last packet had right size */
      /* this should be true since the readout of the pmc-fifo is slower than LWL */
      p->ptr=0;
      shm_cfg_ptr[CFG_FIL_TMP_PKG_READ]++;

      if (!FIFO_IS_EMPTY(value))
      {
        p->header_found=value;
      }
      return STATUS_PACKET_AVAIL;
    }
    else if (p->ptr > p->length)
    {
      // packet oversized
      shm_cfg_ptr[CFG_FIL_TMP_PKG_OVERSIZE]++;

      //TBD : dump (p);
      p->ptr=0;
      if (!FIFO_IS_EMPTY(value))
      {
        p->header_found=value;
      }
      return STATUS_NO_PACKET;
    }

    if (FIFO_IS_EMPTY(value))
    {
      /* Fifo is empty */
      return STATUS_FIFO_EMPTY;
    }
    else
    {
      /* the header uses all 32 bit */
      if (p->ptr)
      {
        /* a new header is received before all values of last packet have been read */
        /* (p->ptr is set to 0 when the packet is complete) */
        shm_cfg_ptr[CFG_FIL_TMP_PKG_INCOMPLETE]++;
      }

      /* start a new packet */
      p->data[0] = value;
      p->ptr = 1;
      p->length = packet_len(value);
    }

  }
  else
  {
    /* no Header and not Empty */
    /* in the following data words only the lower 16 bit are used */

    if (p->ptr && (p->ptr<MAX_PACKET_LENGTH))
    {
      p->data[(p->ptr)++] = value;
    }
  }
  return STATUS_NO_PACKET;
}


/*******************************************************************************
 *
 * FUNCTION
 *   empty_fifo_period
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

__INLINE void empty_fifo_period( unsigned long max_tics)
{
  unsigned long time1, time2;
  volatile unsigned int val;
  int empty=0, tcheck=0;

  tic_read(time1);

  while(1)
  {
    val=lwl_value_get();

    if(val==LWL_FIFO_EMPTY)
    {
      if(++empty >= 3) break;
    }

    if(tcheck++>10)
    {
      tcheck=0;
      tic_read(time2);
      if ((time2-time1)>max_tics) break;
    }
  }
}


/*******************************************************************************
 *
 * FUNCTION
 *   empty_fifo
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

__INLINE int empty_fifo( unsigned long max_tics)
{
  int empty=0, tcheck=0, error=0, ok=0;
  unsigned long time1, time2, time3;
  volatile unsigned int val;
  int vals_read = 0;

  tic_read(time1);
  time2=time1;

  do
  {
    val=lwl_value_get();
    vals_read++;
    if(val==LWL_FIFO_EMPTY)
    {
      if(++empty >= 3) ok=1;
    }
    else if (++tcheck>5)
    {
      tcheck=0;
      tic_read(time3);

      if((time3-time1)>(500 * max_tics))
      {
        error = 1;
      }
      else if((time3-time2)>max_tics)
      {
        // leave time for other tasks
        rt_task_wait_period();
        tic_read(time2);
      }
    }

  } while(!error && !ok);

  /* read status to reset stored flags */
  val=lwl_status_get();
  val=lwl_value_get();
  val=lwl_status_get();

  if (error)
  {
    dbg_printf(DBGMSG_ERROR,"empty_fifo(): could not empty FIFO !!!\n");
  }

#ifdef TIME_CONSUMING_DEBUG
  {
    uint32 status;
    dbg_printf(DBGMSG_INFO3,"empty_fifo(): %s\n",(error?"ERROR":"OK"));

    tic_read(time3);
    dbg_printf(DBGMSG_INFO3,"empty_fifo(): TICS needed : %d\n",time3-time1);
    dbg_printf(DBGMSG_INFO3,"empty_fifo(): vals_read   : %d\n",vals_read);

    status=lwl_status_get();
    status=lwl_status_get();
    print_lwl_status(status);
  }
#endif

  return (error ? SINQHM_ERR_EMPTY_FIFO : SINQHM_OK);
}



#ifndef _INLINE

/*******************************************************************************
 *
 * FUNCTION
 *   pmc_module_init
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

#ifdef FILLER_RTAI
int pmc_module_init(void)
{
  int i;
  unsigned int pci_lwl_addr;
  struct pci_dev *dev=0;
  unsigned volatile int dummy;


#ifdef ARCH_X86
  unsigned short value;

  dev = pci_find_device(0x8086,0x244e,0);
  if (!dev)
  {
    rt_printk ("\npmc_module_init(): ERROR: PCI bridge not found!\n");
    return 1; // ERROR
  }

  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): dev = 0x%08x\n",dev);

  dummy = pci_read_config_word(dev,0x22,&value);
  value |= 0x20;
  pci_write_config_word(dev,0x22,value);
  pci_lwl_addr = value << 16;

  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): dummy = 0x%08x\n",dummy);
  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): value = 0x%08x\n",value);
  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): pci_lwl_addr = 0x%08x\n",pci_lwl_addr);

  pmc_base_ptr=ioremap(pci_lwl_addr,4*1024);

#else

  /* configure PCI-Bridge to acces PMC - LWL Module */
  // try  PCI bridge of  MEN A12 Rev 1
  dev = pci_find_device(0x104c,0xac21,0);
  if (!dev)
  {
    // try  PCI bridge of  MEN A12 Rev 4
    dev = pci_find_device(0x104c,0xac28,0);
  }
  if (!dev)
  {
    rt_printk ("\npmc_module_init(): ERROR: PCI bridge not found!\n");
    return 1; // ERROR
  }

  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): dev = 0x%08x\n",(unsigned int)dev);

  pci_write_config_word(dev,0x04,0x0007);
  pci_write_config_word(dev,0x20,0x8a20);
  pci_write_config_word(dev,0x22,0x8a20);

  pci_lwl_addr = 0x8a200000;
  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): pci_lwl_addr = 0x%08x\n",pci_lwl_addr);

  pmc_base_ptr=ioremap(pci_lwl_addr,4*1024);
  dbg_printf(DBGMSG_INFO2,"pmc_module_init(): remapped pmc_base_ptr = 0x%08x\n",(unsigned int)pmc_base_ptr);

#endif

  /* print some values and status */
  for (i=0;i<3;i++)
  {
    dummy = lwl_value_get();
    dbg_printf(DBGMSG_INFO2,"pmc_module_init(): val    = 0x%08x\n",dummy);
    dummy = lwl_status_get();
    dbg_printf(DBGMSG_INFO2,"pmc_module_init(): status = 0x%08x\n",dummy);
  }
  return 0;
}

void pmc_module_close(void)
{
}


#else

/*******************************************************************************/

int pmc_module_init(void)
{

 lwl_fifo = open(lwl_fifo_pipe_name,O_RDONLY | O_NONBLOCK);

 if (lwl_fifo<0)
 {
   printf("ERROR: cannot open named pipe: %s\n",lwl_fifo_pipe_name);
   return 1;
 }
 return 0;
}

/*******************************************************************************/

void pmc_module_close(void)
{
  if (lwl_fifo>2) close(lwl_fifo);
  lwl_fifo = -1;
}

#endif

#endif  /* ndef _INLINE */

#endif  // APP_FILLER

#ifndef _INLINE

/*******************************************************************************
 *
 * FUNCTION
 *   snprint_packet_info
 *
 ******************************************************************************/

#define PPRINT     printed_len += snprintf
#define PPBUFFLEN  bufferlen
#define PPBUFF     &buffer[printed_len], (PPBUFFLEN-printed_len)

int snprint_packet_info(char* buffer,int bufferlen, char* pre, packet_type *p)
{
  int    printed_len=0;
  uint32 header_type;
  uint32 hdr;
  uint32 nrl, pf, swc, sync0, sync1, sync2, sync3;
  uint32 timestamp;
  uint32 channel;
  uint32 xpos;
  uint32 ypos;
  uint32 wire;
  uint32 errorflag;
  uint32 counts;
  uint32 frame;
  uint32 coinct;
  uint32 rate_an;
  uint32 rate_kv;
  uint32 rate_kh;
  uint32 rate_valid;
  uint32 pos_an,pos_kv,pos_kh,pos_k_avg,kv_valid,kh_valid;

  // Size
  PPRINT(PPBUFF,"%sPacket Size: Header (32bit) + %d Data Words (16bit) ",pre,p->ptr-1);
  if (p->ptr > p->length)
  {
    PPRINT(PPBUFF,"==> Oversized, expected %d Data Words\n",p->length-1);
  }
  else if (p->ptr < p->length)
  {
    PPRINT(PPBUFF,"==> Incomplete, expected %d Data Words\n",p->length-1);
  }
  else
  {
    PPRINT(PPBUFF,"==> OK\n");
  }

  // Type
  hdr = p->data[0];

  nrl   = (hdr & LWL_HDR_NRL_MASK)   ? 1 : 0;
  pf    = (hdr & LWL_HDR_PF_MASK)    ? 1 : 0; 
  swc   = (hdr & LWL_HDR_SWC_MASK)   ? 1 : 0;
  sync0 = (hdr & LWL_HDR_SYNC0_MASK) ? 1 : 0;
  sync1 = (hdr & LWL_HDR_SYNC1_MASK) ? 1 : 0;
  sync2 = (hdr & LWL_HDR_SYNC2_MASK) ? 1 : 0;
  sync3 = (hdr & LWL_HDR_SYNC3_MASK) ? 1 : 0;

  PPRINT(PPBUFF,"%sHeader:  NRL=%d  PF=%d  SWC=%d  SYNC3=%d  SYNC2=%d  SYNC1=%d  SYNC0=%d\n",
                 pre, nrl, pf, swc, sync3, sync2, sync1, sync0);

  header_type = hdr & LWL_HDR_TYPE_MASK;

  if (header_type == LWL_HEADER_TSI_L2)
  {
    PPRINT(PPBUFF,"%sTiming Status Info\n",pre);
  }
  else if (header_type == LWL_HEADER_TSI_L4)
  {

  // Information from TDC 
    uint32 hits, flashes;
    uint32 xorf1, govfl, running, armed, pll, conf;
    
    xorf1    = (p->data[3] & LWL_PSD_XORF1  ) ? 1 : 0;
    govfl    = (p->data[3] & LWL_PSD_GOVFL  ) ? 1 : 0;
    running  = (p->data[3] & LWL_PSD_RUNNING) ? 1 : 0;
    armed    = (p->data[3] & LWL_PSD_ARMED  ) ? 1 : 0;
    pll      = (p->data[3] & LWL_PSD_PLL    ) ? 1 : 0;
    conf     = (p->data[3] & LWL_PSD_CONF   ) ? 1 : 0;
    hits     =  p->data[2];
    flashes  = (p->data[3] & LWL_PSD_FLASH_MASK);

    PPRINT(PPBUFF,"%sPSD Timing Status Info: Hits=%d  Flashes=%d  (Flags: XORF1=%d GOVFL=%d RUNNING=%d ARMED=%d PLL=%d CONF=%d)\n", pre, hits, flashes, xorf1, govfl, running, armed, pll, conf);

  }
  else if (header_type == LWL_HEADER_TSI_SANS2) 
  {
    coinct = p->data[0] & LWL_HDR_TS_MASK;
    rate_valid = 10 * (p->data[2] | ((p->data[1]&0x0f0)<<12) );
    rate_an    = 10 * (p->data[3] | ((p->data[1]&0x00f)<<16) );
    rate_kv    = 10 *  p->data[4];
    rate_kh    = 10 *  p->data[5];

    PPRINT(PPBUFF,"%sSANS2 Timing Status Info: Coinc-time=%d   rate-an=%d [ev/s]  rate-kv=%d [ev/s] rate-kh=%d [ev/s]  rate-valid=%d [ev/s] \n",pre,coinct,rate_an,rate_kv,rate_kh,rate_valid);
  }
  else if (header_type == LWL_GUMMI_MON) 
  {
    uint32 mon[3];
    int i;
    uint32 timestamp;
  
    for(i = 0; i < 3; i++)
    {
      mon[i] = (p->data[2*i+1] << 16) | (p->data[2*i+2]);
    }
    timestamp = p->data[0] & LWL_HDR_TS_MASK;

    PPRINT(PPBUFF,"Gummi monitors: %d, %d, %d registered for timestamp=%d\n", mon[0],mon[1],mon[2],timestamp);
  }
  else if ((header_type >= LWL_TOF_C1) && (header_type <= LWL_TOF_C9))
  {
    timestamp = p->data[0] & LWL_HDR_TS_MASK;
    channel   = p->data[1];
    PPRINT(PPBUFF,"%sTime of Flight:  channel=%d  timestamp=%d\n",pre,channel,timestamp);
  }
  else if (header_type == LWL_HM_NC_C1)
  {
    channel   = p->data[1];
    PPRINT(PPBUFF,"%sDigital:  channel=%d\n",pre,channel);
  }
  else if (header_type == LWL_PSD_DATA)
  {
    timestamp = p->data[0] & LWL_HDR_TS_MASK;
    xpos      = p->data[1];
    ypos      = p->data[2];
    PPRINT(PPBUFF,"%sPosition Detector:  xpos=%d  ypos=%d  timestamp=%d\n",pre,xpos,ypos,timestamp);
    // SANS2
    pos_an = p->data[0] & 0x01ff;
    pos_kv = p->data[1] & 0x01ff;
    pos_kh = p->data[2] & 0x01ff;

    kv_valid = (p->data[1] & 0x8000) ? 1:0 ;
    kh_valid = (p->data[2] & 0x8000) ? 1:0 ;
    pos_k_avg = (pos_kv + pos_kh) >> (kv_valid + kh_valid - 1);
    PPRINT(PPBUFF,"%sSANS2 Detector   :  an=%d  [%d]kv=%d  [%d]kh=%d  k-avg=%d\n",pre,pos_an,kv_valid,pos_kv,kh_valid,pos_kh,pos_k_avg);

  }
  else if (header_type == LWL_HM_NC_C3)
  {
    wire      = p->data[1] >> 1;
    errorflag = p->data[1]  & 1;
    counts    = p->data[2];
    frame     = p->data[3];
    PPRINT(PPBUFF,"%sHRPT:  errorflag=%d  wire=%d  counts=%d  frame=%d\n",pre,errorflag,wire,counts,frame);
  }
  else if (header_type == LWL_SANS2_DATA)
  {
    xpos      = p->data[0] & 0x1ff;
    ypos      = (p->data[0] >> 9) & 0x1ff;
    PPRINT(PPBUFF,"%sSans2 Position Detector:  xpos=%d  ypos=%d\n",pre,xpos,ypos);
  }

  return printed_len;
}
/******************************************************************************/

#endif     /* ndef _INLINE */
