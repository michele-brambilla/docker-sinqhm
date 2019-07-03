/*******************************************************************************

    Description : Testgenerator fuer Histogram-Memory

    Author(s)   : P.Rasmussen, PSI Abt. AEI
                  G. Theidel, PSI
 
    Date        :15.8.2003

********************************************************************************
The HM Testgenerator is based upon a VMIO10 VME Module.

This module has 2 Z8536-CIO chips (Counter/timer and parallel I/O units),
each of which has 2 I/O 8-bit ports (Port A and Port B),
a 4-bit I/O port (Port C) and 3 counter/timer registers.
Hence, the module has four 8-bit I/O ports, two 4-bit I/O ports
and six counter/timers.

Just to confuse things, the four 8 bit I/O Port on the VMIO10
are called  port A, B, C and D!
Ports A & B are associated with the first Z8536-CIO and Ports C & D are
associated with the second Z8536-CIO.

Z8536-CIO 1                       Z8536-CIO 2
-----------                       -----------
Port A    :  vmio_port_a          Port A    :  vmio_port_c
Port B    :  vmio_port_b          Port B    :  vmio_port_d
Port C    :  vmio_port_c_ab       Port C    :  vmio_port_c_cd
Ctrl Reg  :  vmio_ctrl_ab         Ctrl Reg  :  vmio_ctrl_cd


Memory Map of VMIO10
--------------------
VMIOBASE + 0X01  vmio_port_c_ab  (8 bit R/W Register)  Z8536-CIO 1
           0X09  vmio_port_c_cd  (8 bit R/W Register)  Z8536-CIO 2
           0x11  vmio_port_b     (8 bit R/W Register)  Z8536-CIO 1
           0x19  vmio_port_d     (8 bit R/W Register)  Z8536-CIO 2
           0x21  vmio_port_a     (8 bit R/W Register)  Z8536-CIO 1
           0x29  vmio_port_c     (8 bit R/W Register)  Z8536-CIO 2
           0x31  vmio_ctrl_ab    (8 bit R/W Register)  Z8536-CIO 1
           0x39  vmio_ctrl_cd    (8 bit R/W Register)  Z8536-CIO 2

********************************************************************************

Port Usage for HM Tesgenerator

vmio_port_a
===========
bit I/O
0 : (O) Data Bit 0 Fifo 
1 : (O) Data Bit 1 Fifo 
2 : (O) Data Bit 2 Fifo 
3 : (O) Data Bit 3 Fifo 
4 : (O) Data Bit 4 Fifo 
5 : (O) Data Bit 5 Fifo 
6 : (O) Data Bit 6 Fifo 
7 : (O) Data Bit 7 Fifo 


vmio_port_b
===========
bit I/O
0 : (O) Data Bit 0 Transparent Mode
1 : (O) Data Bit 1 Transparent Mode
2 : (O) Data Bit 2 Transparent Mode
3 : (O) Data Bit 3 Transparent Mode
4 : (O) Data Bit 4 Transparent Mode
5 : (O) Data Bit 5 Transparent Mode
6 : (O) Data Bit 6 Transparent Mode
7 : (O) Data Bit 7 Transparent Mode

vmio_port_c
===========
bit I/O
0 : (O) Mode Bit 0  (Value 1)
1 : (O) Mode Bit 1  (Value 2)
2 : (O) Mode Bit 2  (Value 4)
3 : (O) 
4 : (O) Data Bit 8 Fifo and Transparent Mode
5 : (O) DC5
6 : (O) DC6
7 : (O) DC7


Mode = 0X00 -> R,W = High 
Mode = 0X01  
Mode = 0X02  
Mode = 0X07 -> Reset


vmio_port_d
===========
bit I/O
0 : (I) Q0
1 : (I) Q1
2 : (I) Q2
3 : (I) EF0_S   - FIFO empty
4 : (O) Run
5 : (O) 
6 : (O) 
7 : (O) 

vmio_port_c_ab
==============
bit I/O
0 : (O) Ackn
1 : (O) Strobe
2 : (O) Write
3 : (O) 

vmio_port_c_cd
==============
bit 
0 : IntCLK
1 : 
2 : 
3 : 
************************************************************************************/


/***********************************************************************************/
/* Includes                                                                        */
/***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/vme4l.h>


#include  "vme_testgen.h"


/***********************************************************************************/
/* Includes                                                                        */
/***********************************************************************************/

volatile unsigned char *testgen_base = (volatile unsigned char *)0;


/***********************************************************************************/
/* Functions                                                                       */
/***********************************************************************************/

void vme_write(int ofs, int val)
{
  my_eieio();
  testgen_base[ofs] = val;
  my_eieio();
 /*  sysUsDelay(100); */
}

/***********************************************************************************/

int vme_read(int ofs)
{
 int var;
 /*   printf("---VME-READ---- %s=%s  ([0x%08x])\n",#var,#adr,adr);  */
  my_eieio();
  var = testgen_base[ofs] ;
  my_eieio();
  return var;
 /*  sysUsDelay(100); */
}

/***********************************************************************************/

/*software reset for cio*/
void resetcio(void)
{
  volatile unsigned char read_char;

  /* Even if the state of the Z8536 is not known, the following sequence will
      reset and put it in state 0 */

  read_char = vme_read (VMIO_CTRL_AB);   /* Insures State 0 - or Reset State */
  vme_write(VMIO_CTRL_AB, 0);           /* write pointer or clear RESET */
  read_char = vme_read (VMIO_CTRL_AB);   /* Insure State 0 */
                                        /* After reading CTRL Register the state is reset to 0, */
                                        /* so the next write goes to the pointer register */
  vme_write(VMIO_CTRL_AB, Z8536_MICR);  /* Select Master Interrupt Control Register */
  vme_write(VMIO_CTRL_AB, 0x01);        /* set   RESET bit */
  vme_write(VMIO_CTRL_AB, 0x00);        /* clear RESET bit */


  read_char = vme_read (VMIO_CTRL_CD);   /* Do the same for the second Z8536 */
  vme_write(VMIO_CTRL_CD, 0);
  read_char = vme_read (VMIO_CTRL_CD);
  vme_write(VMIO_CTRL_CD, Z8536_MICR);
  vme_write(VMIO_CTRL_CD, 0x01);
  vme_write(VMIO_CTRL_CD, 0x00);
}

/***********************************************************************************/

void initvmio(void)
{
  vme_write(VMIO_CTRL_CD, Z8536_DPPR_B);  /* VMIO_PORT_D - Data Path Polarity Register */
  vme_write(VMIO_CTRL_CD, 0x00);          /*               set all to non inverting    */
  vme_write(VMIO_CTRL_CD, Z8536_DDR_B);   /* VMIO_PORT_D - Data Direction Register -   */
  vme_write(VMIO_CTRL_CD, 0x0F);          /*               %00001111, Bit 0-3 Input    */
                                          /*                          Bit 4-7 Output   */

  vme_write(VMIO_CTRL_AB, Z8536_DPPR_B);  /* VMIO_PORT_B - Data Path Polarity Register */
  vme_write(VMIO_CTRL_AB, 0x00);          /*               set all to non inverting    */
  vme_write(VMIO_CTRL_AB, Z8536_DDR_B);   /* VMIO_PORT_B - Data Direction Register     */
  vme_write(VMIO_CTRL_AB, 0x00);          /*               set all to Output           */


  vme_write(VMIO_CTRL_AB, Z8536_DPPR_A);  /* VMIO_PORT_A - Data Path Polarity Register */
  vme_write(VMIO_CTRL_AB, 0x00);          /*               set all to non inverting    */
  vme_write(VMIO_CTRL_AB, Z8536_DDR_A);   /* VMIO_PORT_A - Data Direction Register     */
  vme_write(VMIO_CTRL_AB, 0x00);          /*               set all to Output           */

  vme_write(VMIO_CTRL_CD, Z8536_DPPR_A);  /* VMIO_PORT_C - Data Path Polarity Register */
  vme_write(VMIO_CTRL_CD, 0x00);          /*               set all to non inverting    */
  vme_write(VMIO_CTRL_CD, Z8536_DDR_A);   /* VMIO_PORT_C - Data Direction Register     */
  vme_write(VMIO_CTRL_CD, 0x00);          /*               set all to Output           */

  vme_write(VMIO_CTRL_AB, Z8536_DPPR_C);   /* VMIO_PORT_C_AB - Data Path Polarity Register */
  vme_write(VMIO_CTRL_AB, 0x00);           /*                  set all to non inverting    */
  vme_write(VMIO_CTRL_AB, Z8536_DDR_C);    /* VMIO_PORT_C_AB - Data Direction Register -   */
  vme_write(VMIO_CTRL_AB, 0x00);           /*                  set all to Output           */


  vme_write(VMIO_CTRL_AB, Z8536_MSPC_B);   /* VMIO_PORT_B - Mode Specification Register */
  vme_write(VMIO_CTRL_AB, 0x80);           /*               select output port          */

  vme_write(VMIO_CTRL_AB, Z8536_HSPC_B);   /* VMIO_PORT_B - Handshake Specification Register */
  vme_write(VMIO_CTRL_AB, 0x00);           /*               select interlock handshake       */

}

/***********************************************************************************/
/* output/input enable for IC6 ,IC7 */
/***********************************************************************************/
void enable(void)
{
  vme_write(VMIO_CTRL_AB, 0x01);     /* Master Config Control Register                   */
  vme_write(VMIO_CTRL_AB, 0x94);     /* %10010100  VMIO_PORT_A enable                    */
                                     /*            VMIO_PORT_B enable                    */
                                     /*            VMIO_PORT_C_AB, counter/timer3 enable */


  vme_write(VMIO_CTRL_CD, 0x01);     /* Master Config Control Register                   */
  vme_write(VMIO_CTRL_CD, 0x94);     /* %10010100  VMIO_PORT_C enable                    */
                                     /*            VMIO_PORT_D enable                    */
                                     /*            VMIO_PORT_C_CD, counter/timer3 enable */

}

/***********************************************************************************/

void testgen_init(void)
{
  resetcio();
  initvmio();
  enable();

  vme_write(VMIO_PORT_C_AB, 0x02);           /* Write=0  Strobe=1  Ackn=0 */

  vme_write(VMIO_PORT_C, 0x00);              /* Mode=0 -> R,W = High  */
  vme_write(VMIO_PORT_C, 0x07);              /* Mode=7 -> reset       */
  vme_write(VMIO_PORT_C, 0x00);              /* Mode=0                */

}


/***********************************************************************************/

int testgen_setbase(int base_addr)
{
	int fd;
  volatile void *vme_a16_d16  = NULL;


	if( (fd = open( VME4L_DEV_NAME_A16_D16, O_RDWR )) < 0 )
	{
		printf("*** can't open /dev/vme4l_a16d16\n" );
		return 0;
	}

	/*
	 * MAP VME A16 Space into user space
	 */

	vme_a16_d16 = mmap( NULL, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x0000 );
	if( vme_a16_d16 == 0 ){
		printf("*** Couldn't map VME space\n");
		return 0;
	}
  testgen_base = (char*)vme_a16_d16+base_addr;

//  status = sysBusToLocalAdrs (VME_AM_USR_SHORT_IO,
//    (char *) base_addr, (char **)&);

  printf ("base_addr      = 0x%08x \n",base_addr);
  printf ("testgen_base   = 0x%08x \n",(unsigned int)testgen_base);

  return 1;
}

/***********************************************************************************/

void testgen_send_byte(int bit8, int data)
{
  volatile unsigned char read_char;

  do
  {
    vme_write(VMIO_CTRL_AB, Z8536_CSR_B);    /* VMIO_PORT_B - Command and Status Register  */
    read_char = vme_read(VMIO_CTRL_AB);
  }
  while ((read_char & CSR_ORE) != CSR_ORE);    /*Output Register empty ?                     */

  //  printf("bit8=0x%08x  ",bit8);
  vme_write(VMIO_PORT_C , bit8);     /* write Data Bit 8   */

  //  printf("data=0x%08x\n",data);
  vme_write(VMIO_PORT_B , data);     /* write Data Bit 0-7 */
}                                                         

/***********************************************************************************/

void testgen_fifo(int bit8, int data)
{
    vme_write(VMIO_PORT_C, bit8);      /* write Data Bit 8   */
    vme_write(VMIO_PORT_A, data);      /* write Data Bit 0-7 */

    vme_write(VMIO_PORT_C_AB, 0X04);   /* Write=1  Strobe=0  Ackn=0 */                         
    vme_write(VMIO_PORT_C_AB, 0X00); 
}  

/***********************************************************************************/

void testgen_send_header(unsigned int header)
{
    testgen_send_byte(0x10, (header>>24) & 0x0ff);
    testgen_send_byte(0x00, (header>>16) & 0x0ff);
    testgen_send_byte(0x00, (header>>8) & 0x0ff);
    testgen_send_byte(0x00, header & 0x0ff);
}

/***********************************************************************************/

void testgen_send_data(unsigned int data)
{
    testgen_send_byte(0x00, (data>>8) & 0x0ff);
    testgen_send_byte(0x00, data & 0x0ff);
}

/***********************************************************************************/
void testgen_send(unsigned int data)
{
  if (data & 0xffff0000)
  {
    testgen_send_header(data);
  }
  else
  {
    testgen_send_data(data);
  }
}

/***********************************************************************************/

void testgen_load_fifo (unsigned int *data_buffer, unsigned int data_length)
{
  int i;

  vme_write(VMIO_PORT_C_AB, 0X02);         /* Write=0  Strobe=1  Ackn=0 */
  vme_write(VMIO_PORT_C, 0X00);            /* Mode=0 -> R,W=High        */
  vme_write(VMIO_PORT_C, 0X07);            /* Mode=7 -> reset           */
  vme_write(VMIO_PORT_C, 0X00);            /* Mode=0                    */

  vme_write(VMIO_PORT_C_AB, 0x00);         /* Write=0  Strobe=0  Ackn=0 */
  vme_write(VMIO_PORT_C, 0x00);            /* Mode=0 -> R,W=High        */
  vme_write(VMIO_PORT_C, 0x07);            /* Mode=7 -> reset           */

  vme_write(VMIO_PORT_C, 0x01);            /* Mode=1                    */

  for (i = 0;i < data_length;i++)
  {
    if (data_buffer[i] & 0xffff0000)
    {
      // send header
      testgen_fifo(0x10, (data_buffer[i]>>24) & 0x0ff);
      testgen_fifo(0x00, (data_buffer[i]>>16) & 0x0ff);
      testgen_fifo(0x00, (data_buffer[i]>>8)  & 0x0ff);
      testgen_fifo(0x00,  data_buffer[i]      & 0x0ff);
    }
    else
    {
      // send data
      testgen_fifo(0x00, (data_buffer[i]>>8) & 0x0ff);
      testgen_fifo(0x00,  data_buffer[i]     & 0x0ff);
    }
  } 

//  testgen_fifo(0xAA, 0xAA);  /* last byte */

} /*FIFO_laden*/

/***********************************************************************************/

/*FIFO_senden========================================================*/
/*Nachdem der FIFO mit einem Daten-Map geladen wurde, wird der Timer fuer
den Datentransfer Rate initialisiert und gestartet. */


void testgen_send_fifo_start(unsigned char cycle_time)
{
  vme_write(VMIO_CTRL_CD, Z8536_CTMS_3) ;  /* Timer 3 - Counter/Timer Mode Specification */  
  vme_write(VMIO_CTRL_CD, 0XC2) ;          /*           %11000010  Continuous            */
                                           /*                      external output       */
                                           /*                      square wawe           */

  vme_write(VMIO_CTRL_CD, Z8536_CTTC_H_3);   /* Timer 3 - Time Constant High Byte  */
  vme_write(VMIO_CTRL_CD, 0x00);
  vme_write(VMIO_CTRL_CD, Z8536_CTTC_L_3);   /* Timer 3 - Time Constant Low Byte   */
  vme_write(VMIO_CTRL_CD, cycle_time);

  vme_write(VMIO_CTRL_CD, Z8536_CTCS_3);     /* Timer 3 - Command and Status Register */
  vme_write(VMIO_CTRL_CD, 0X06);             /* Start Timer                           */

  vme_write(VMIO_PORT_C, 0X02);       /* Mode=2  */
  vme_write(VMIO_PORT_D, 0X10);       /* Run=1   */
}



void testgen_send_fifo_stop(void)
{
  unsigned char read_char;

  vme_write(VMIO_PORT_D, 0X00);       /* Run=0   */  

  do
  {
    read_char = vme_read(VMIO_PORT_D);
  }                                                        
  while( (read_char & 0x08) != 0);      /*Wait for FIFO empty*/

  vme_write(VMIO_PORT_C_AB, 0x02);           /* Write=0  Strobe=1  Ackn=0 */ 
  vme_write(VMIO_PORT_C, 0x00);              /* Mode=0                    */

  vme_write(VMIO_CTRL_CD, Z8536_CTCS_3);     /* Timer 3 - Command and Status Register */
  vme_write(VMIO_CTRL_CD, 0x00);             /* Stop Timer                            */

//  vme_write(VMIO_PORT_C, 0x00);              /* Mode=0 -> R,W = High  */
//  vme_write(VMIO_PORT_C, 0x07);              /* Mode=7 -> reset       */
//  vme_write(VMIO_PORT_C, 0x00);              /* Mode=0                */

}                                                           /*FIFO_senden*/


void testgen_send_fifo(unsigned char cycle_time)
{
  testgen_send_fifo_start(cycle_time);
  testgen_send_fifo_stop();


}


