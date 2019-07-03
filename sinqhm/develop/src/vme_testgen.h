/***********************************************************************************/


#ifndef HM_TESTGEN_H
#define HM_TESTGEN_H
/* Z8536 CIO definitions */

#define Z8536_MICR      0x00    /* Master Interrupt Control Register */
#define Z8536_MCCR      0x01    /* Master Config Control Register */

#define Z8536_DPPR_A    0x22    /* Data Path Polarity Register - Port A */
#define Z8536_DPPR_B    0x2a    /* Data Path Polarity Register - Port B */
#define Z8536_DPPR_C    0x05    /* Data Path Polarity Register - Port C */

#define Z8536_DDR_A     0x23    /* Data Direction Register - Port A */
#define Z8536_DDR_B     0x2b    /* Data Direction Register - Port B */
#define Z8536_DDR_C     0x06    /* Data Direction Register - Port C */

#define Z8536_SIOCR_A   0x24    /* Special I/O Control Register - Port A */
#define Z8536_SIOCR_B   0x2c    /* Special I/O Control Register - Port B */
#define Z8536_SIOCR_C   0x07    /* Special I/O Control Register - Port C */

#define Z8536_MSPC_A    0x20    /* Mode Specification Register - Port A */
#define Z8536_MSPC_B    0x28    /* Mode Specification Register - Port B */

#define Z8536_HSPC_A    0x21    /* Handshake Specification Register - Port A */
#define Z8536_HSPC_B    0x29    /* Handshake Specification Register - Port B */

#define Z8536_CSR_A     0x08    /* Command and Status Register - Port A */
#define Z8536_CSR_B     0x09    /* Command and Status Register - Port B */

#define Z8536_CTMS_1    0x1c    /* Counter/Timer Mode Specification - Timer 1 */
#define Z8536_CTMS_2    0x1d    /* Counter/Timer Mode Specification - Timer 2 */
#define Z8536_CTMS_3    0x1e    /* Counter/Timer Mode Specification - Timer 3 */

#define Z8536_CTCS_3    0x0c    /* Counter/Timer Command and Status - Timer 3 */

#define Z8536_CTTC_H_3  0x1a    /* Counter/Timer Time Constant High Byte - Timer 3 */
#define Z8536_CTTC_L_3  0x1b    /* Counter/Timer Time Constant High Byte - Timer 3 */


#define VMIO_PORT_A     0x21
#define VMIO_PORT_B     0x11
#define VMIO_PORT_C     0x29
#define VMIO_PORT_D     0x19
#define VMIO_PORT_C_AB  0X01
#define VMIO_PORT_C_CD  0X09
#define VMIO_CTRL_AB    0x31
#define VMIO_CTRL_CD    0x39



/* flags in Command and Status Register */
#define CSR_ORE 0x08  /* Output Register Empty */


#define tic_read(var) {  __asm(" sync ");   __asm ("mftb %0" : "=r" (var));  __asm(" sync ");}
#define my_eieio()  { __asm(" eieio "); __asm(" sync ");}

#ifdef MEN_A012
#define CPU_BUS_CLOCK 100.0
#elif defined(MV2600)
#define CPU_BUS_CLOCK 66.0
#else
#define CPU_BUS_CLOCK 66.0
#endif


void testgen_send_data(unsigned int data);
void testgen_send_header(unsigned int header);
void testgen_send(unsigned int data);
int testgen_setbase(int base_addr);
void testgen_init(void);
void testgen_send_fifo(unsigned char cycle_time);
void testgen_load_fifo (unsigned int *data_buffer, unsigned int data_length);
void testgen_send_fifo_stop(void);
void testgen_send_fifo_start(unsigned char cycle_time);



#endif  /* HM_TESTGEN_H */
