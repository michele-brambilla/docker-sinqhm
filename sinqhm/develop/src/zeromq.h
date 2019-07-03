#ifndef _ZEROMQ_H
#define _ZEROMQ_H
#include<stdio.h>
#include <stdint.h>

#include <zmq.h>
#include "controlshm.h"
#include "lwlpmc.h"

#include <assert.h>
#include <assert.h>
#include <time.h>

extern char zeromq_bind_address[1024];
extern int32_t status;
extern void *zmqContext;
extern void *pullSocket;


typedef struct {
  char u[10];
} xy_t;

typedef struct {
  uint32_t u[2];
} detectorid_t;

typedef struct {
  uint32_t u[2];
} debug_t;


typedef struct
{
  uint32_t ptr;
  uint32_t length;
  uint32_t header_found;
  void* data;
} packet_zmq_type;


int32_t zeromq_init(void);


/* void zmqReceive(packet_zmq_type*); */
int zmqReceive(packet_type*);
void zmqReceiveForget(packet_type*);

extern void process_0mq(packet_type*);
//void zeromq_packet_process(packet_zmq_type*);

/* void align_with_electronics(); */


#define MAX_HEADER_DATA (1024*1024)

#define ZMQ_NO_ERROR           0
#define ZMQ_RECV_TIMEOUT       1
#define ZMQ_CONNECT_ERROR      2
#define ZMQ_OPT_ERROR          3
#define ZMQ_HEADER_ERROR       4
#define ZMQ_INCOMPLETE_PACKAGE 5
#define JSON_HEADER_FAILURE    6
#define ZMQ_DATA_ERROR         7
#define ZMQ_GENERIC_ERROR    100

#define RITA2_COUNTER_MASK 0xf0000000     // corresponding to b,c,r,g = 1,1,1,1

#define RITA2_PACKET_TYPE_MASK 0x0f000000     
#define EV_TYPE_NC       0x01000000
#define EV_TYPE_POS_1D   0x02000000
#define EV_TYPE_POS_2D   0x03000000


/* time (in millisecond) before zmq_recv throws a timeout, assuming either
   network or electronics is down and the system tries to reconnect */
#define DEFAULT_TIMEOUT    1000
#define DEBUG_TIMEOUT      1000

#endif //ZEROMQ_H
