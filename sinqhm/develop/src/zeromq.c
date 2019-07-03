#include <errno.h>

#include "debugshm.h"
#include "process_common.h"
#include "rawdata.h"
#include "zeromq.h"

#include "sinqhmlog.h"

///////////
// JSON parser
#include "cJSON.h"

int parse_header_value(cJSON *const, char *const, uint *);
int parse_header_array(cJSON *const, char *const, char *const, int const,
                       uint *);

char headerData[MAX_HEADER_DATA];

char zeromq_bind_address[1024];
int status;
void *zmqContext = NULL;
void *pullSocket = NULL;

// static event_t* RawDataBuffer = NULL;

static uint32_t *dataBuffer = NULL;

extern volatile uint32_t *RawDataBuffer;

extern volatile unsigned int *shm_cfg_ptr;

static int maxSize = 1000000;

int zeromq_init(void) {

  int value;

  if (!shm_cfg_ptr[CFG_FIL_ZMQ_INI_DONE]) {
    dbg_printf(DBGMSG_INFO1, "0MQ init\n");

    zmqContext = zmq_ctx_new();
    pullSocket = zmq_socket(zmqContext, ZMQ_PULL);
    if (pullSocket == NULL) {
      shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_CONNECT_ERROR;
      dbg_printf(DBGMSG_ERROR, "0MQ init failure\n");
      sinqhmlog(DBGMSG_ERROR, "com", "0MQ init failure");
    }
    assert(pullSocket);
    shm_cfg_ptr[CFG_FIL_ZMQ_SOCK_ADDR] = (size_t)pullSocket;
  }

  value = 100;
  status = zmq_setsockopt(pullSocket, ZMQ_RCVHWM, &value, sizeof(value));
  if (status != 0) {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_OPT_ERROR;
    dbg_printf(DBGMSG_WARNING, "Unable to set ZMQ_RCVHWM\n");
    write_logfile(DBGMSG_WARNING, "com", "Unable to set ZMQ_RCVHWM");
  }

  //  snprintf(sockAddress,sizeof(sockAddress),"tcp://0.0.0.0:%s",#port);
  status = zmq_connect(pullSocket, zeromq_bind_address);
  shm_cfg_ptr[CFG_FIL_ZMQ_INI_ERROR] = status;
  if (status != 0) {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_CONNECT_ERROR;
    dbg_printf(DBGMSG_ERROR, "0MQ connection error\n");
    sinqhmlog(DBGMSG_ERROR, "com", "0MQ connection error");
  }
  assert(status == 0);

  value = 1;
  status = zmq_setsockopt(pullSocket, ZMQ_TCP_KEEPALIVE, &value, sizeof(value));

  if (status != 0) {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_OPT_ERROR;
    dbg_printf(DBGMSG_WARNING, "Unable to set ZMQ_TCP_KEEPALIVE\n");
    write_logfile(DBGMSG_WARNING, "com", "Unable to set ZMQ_TCP_KEEPALIVE");
  }

  value = 500;
  status = zmq_setsockopt(pullSocket, ZMQ_RCVBUF, &value, sizeof(value));
  if (status != 0) {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_OPT_ERROR;
    dbg_printf(DBGMSG_WARNING, "Unable to set ZMQ_RCVBUF\n");
    write_logfile(DBGMSG_WARNING, "com", "Unable to set ZMQ_RCVBUF");
  }

  //  assert(status == 0);
  shm_cfg_ptr[CFG_FIL_ZMQ_KEEPALIVE] = value;

  value = DEFAULT_TIMEOUT; /* milliseconds before receive timeout */
  status = zmq_setsockopt(pullSocket, ZMQ_RCVTIMEO, &value, sizeof(value));
  if (status != 0) {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_OPT_ERROR;
    dbg_printf(DBGMSG_WARNING, "Unable to set ZMQ_RCVTIMEO\n");
    write_logfile(DBGMSG_WARNING, "com", "Unable to set ZMQ_RCVTIMEO");
  }
  //  assert(status == 0);
  shm_cfg_ptr[CFG_FIL_ZMQ_RCVTIMEO] = 0;

  dataBuffer = malloc(maxSize);

  shm_cfg_ptr[CFG_FIL_ZMQ_INI_DONE] = 1;
  shm_cfg_ptr[CFG_FIL_ZMQ_FIRST_PKG] = 1;

  return status;
}

void zeromq_close(void) {

  if (shm_cfg_ptr[CFG_FIL_ZMQ_INI_DONE]) {
    zmq_close(pullSocket);
    zmq_ctx_destroy(zmqContext);
  }
}

int parse_header_value(cJSON *const root, char *const field, uint *value) {
  cJSON *const item = cJSON_GetObjectItem(root, field);
  if (item != NULL) {
    *value = (unsigned int)(item->valueint);
  } else {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_HEADER_ERROR;
    dbg_printf(DBGMSG_ERROR, "Error parsing header: cannot find \"%s\"\n",
               field);
    write_logfile(DBGMSG_ERROR, "com",
                  "Error parsing header in 'parse_header_value'");
    return 1;
  }
  return 0;
}

int parse_header_array(cJSON *const root, char *const field,
                       char *const subfield, int const index, uint *value) {
  cJSON *const item = cJSON_GetObjectItem(root, field);
  if (item != NULL) {
    if (cJSON_GetArraySize(item) > index) {
      cJSON *const subitem = cJSON_GetArrayItem(item, index);
      if (subitem != NULL) {
        if (strlen(subfield) == 0) {
          *value = subitem->valueint;
        } else {
          parse_header_value(subitem, subfield, value);
        }
      } else {
        shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_HEADER_ERROR;
        dbg_printf(DBGMSG_ERROR, "Error parsing header: cannot find '%s->%s'\n",
                   field, subfield);
        write_logfile(DBGMSG_ERROR, "com",
                      "Error parsing header in 'parse_header_array'");
        return 1;
      }
    } else {
      shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_HEADER_ERROR;
      dbg_printf(DBGMSG_ERROR, "Wrong header array dimension in field '%s'\n",
                 field);
      write_logfile(DBGMSG_ERROR, "com", "Wrong header array dimension");
      return 1;
    }
  } else {
    shm_cfg_ptr[CFG_ZMQ_SICS_STATUS] = ZMQ_HEADER_ERROR;
    dbg_printf(DBGMSG_ERROR, "Error parsing header: cannot find '%s'\n", field);
    write_logfile(DBGMSG_ERROR, "com",
                  "Error parsing header: cannot parse json");
    return 1;
  }
  return 0;
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Thu Apr 20 09:22:16 2017
void zmqReceiveForget(packet_type *p) {

  zmq_pollitem_t items;
  size_t optlen = sizeof(int);
  int rcvmore;

  items.socket = pullSocket;
  items.events = ZMQ_POLLIN;
  /* Poll for event */
  int rc = zmq_poll(&items, 1, 1000);
  if (rc == 0) {
    if (!shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO]) {
      shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO] = 1;
      dbg_printf(DBGMSG_ERROR, "0MQ socket not connected\n");
      sinqhmlog(DBGMSG_ERROR, "com", "0MQ socket not connected");
    }
    return;
  }
  if (shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO]) {
    shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO] = 0;
    dbg_printf(DBGMSG_INFO1, "0MQ connection established\n");
    write_logfile(DBGMSG_INFO1, "com", "0MQ connection established");
  }
  shm_cfg_ptr[CFG_FIL_PKG_READ]++;
  shm_cfg_ptr[CFG_FIL_TSI_DAQ_STOPPED]++;

  do {
    zmq_msg_t msg;
    rc = zmq_msg_init(&msg);
    if (rc != 0) {
      write_logfile(DBGMSG_ERROR, "com", "Unable to initialise message");
    }
    rc = zmq_msg_recv(&msg, pullSocket, 0);
    if (rc == -1) {
      write_logfile(DBGMSG_ERROR, "com", "Unable to receive message");
    } else {
      shm_cfg_ptr[CFG_ZMQ_BYTES_READ] += rc;
    }
    rc = zmq_getsockopt(pullSocket, ZMQ_RCVMORE, &rcvmore, &optlen);
    if (rc != 0) {
      write_logfile(DBGMSG_WARNING, "com", "Multi-packet event message");
    }
    zmq_msg_close(&msg);
  } while (rcvmore);
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 25 10:02:38 2017
int processHeader(packet_type *p, zmq_msg_t *msg) {
  static char errstr[1024];
  static uint32_t packet_id = 0;
  size_t header_size;
  cJSON *root = 0;

  /* Get string message */
  packet_id++;
  header_size = zmq_msg_size(msg);
  if (header_size > MAX_HEADER_DATA - 1) {
    return ZMQ_HEADER_ERROR;
  }
  strncpy(headerData, (char *)zmq_msg_data(msg), header_size);
  strcat(headerData, "\0");

  /* Parse header */
  root = cJSON_Parse(headerData);
  if (!root) {
    dbg_printf(DBGMSG_ERROR, "Unable to parse header, discard message\n");
    write_logfile(DBGMSG_ERROR, "com",
                  "Unable to parse header, discard message");
    printf("%s\n", headerData);
    cJSON_Delete(root);
    return ZMQ_HEADER_ERROR;
  }
  if (parse_header_value(root, "pid", &(p->ptr)) ||
      parse_header_array(root, "ds", "", 1, &(p->length))) {
    cJSON_Delete(root);
    return ZMQ_HEADER_ERROR;
  }
  if (shm_cfg_ptr[CFG_FIL_ZMQ_FIRST_PKG]) {
    packet_id = p->ptr;
  }
  if (p->ptr > packet_id) {
    snprintf(errstr, 1023, "At packet %d: missed %d packet", p->ptr,
             (p->ptr - packet_id));
    sinqhmlog(DBGMSG_WARNING, "com", errstr);
    strcat(errstr, "\n");
    dbg_printf(DBGMSG_WARNING, errstr);
    packet_id = p->ptr;
  } else {
    if (p->ptr < packet_id) {
      snprintf(errstr, 1023, "Expected packet %d, received %d", packet_id,
               p->ptr);
      write_logfile(DBGMSG_ERROR, "com", errstr);
      strcat(errstr, "\n");
      dbg_printf(DBGMSG_ERROR, errstr);
      packet_id = p->ptr;
    }
  }

  cJSON_Delete(root);
  return ZMQ_NO_ERROR;
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 25 11:13:03 2017
int processData(packet_type *p, zmq_msg_t *msg) {
  static char errstr[1024];
  static size_t current_memory = 0;
  size_t required_memory;
  size_t expected_memory;
  required_memory = zmq_msg_size(msg);
  expected_memory = 2 * p->length * sizeof(uint32_t);

  /* Allocate buffer */
  if (current_memory < required_memory) {
    if (dataBuffer) {
      free(dataBuffer);
      dataBuffer = 0;
    }
    dataBuffer = calloc(required_memory, sizeof(uint32_t));
    if (dataBuffer == 0) {
      dbg_printf(DBGMSG_WARNING, "zmqReceive unable to allocate memory\n");
      sinqhmlog(DBGMSG_ERROR, "com", "zmqReceive unable to allocate memory");
      return ZMQ_DATA_ERROR;
    }
  }
  if (expected_memory != required_memory) {
    snprintf(errstr, 1023, "expected %zu bytes, received %zu", expected_memory,
             required_memory);
    sinqhmlog(DBGMSG_ERROR, "com", errstr);
    strcat(errstr, "\n");
    dbg_printf(DBGMSG_ERROR, errstr);
    return ZMQ_DATA_ERROR;
  }

  /* Get data from message */
  memcpy(dataBuffer, (uint32_t *)zmq_msg_data(msg), required_memory);
  p->data = dataBuffer;
  if (shm_cfg_ptr[CFG_SRV_DO_STORE_RAW_DATA]) {
    store0mqRawData(p);
  }

  return ZMQ_NO_ERROR;
}

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Mon Apr 24 16:08:30 2017
int zmqReceive(packet_type *p) {
  static int rcvmore = 0;

  size_t optlen = sizeof(int);
  int optval;
  int status;

  /* Reset informations */
  p->header_found = 0;
  p->ptr = 0;
  p->data = 0;
  p->length = 0;

  zmq_pollitem_t items;
  items.socket = pullSocket;
  items.events = ZMQ_POLLIN;
  int rc = zmq_poll(&items, 1, 1000);
  if (rc == 0) {
    if (!shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO]) {
      shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO] = 1;
      dbg_printf(DBGMSG_ERROR, "0MQ socket not connected\n");
      sinqhmlog(DBGMSG_ERROR, "com", "0MQ socket not connected");
    }
    return ZMQ_RECV_TIMEOUT;
  }
  if (shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO]) {
    shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO] = 0;
    dbg_printf(DBGMSG_INFO1, "0MQ connection established\n");
    write_logfile(DBGMSG_INFO1, "com", "0MQ connection established");
  }

  status = ZMQ_NO_ERROR;
  do {
    /* Initialize and receive message */
    zmq_msg_t msg;
    rc = zmq_msg_init(&msg);
    if (rc != 0) {
      write_logfile(DBGMSG_ERROR, "com", "Unable to initialise message");
      return ZMQ_GENERIC_ERROR;
    }
    rc = zmq_msg_recv(&msg, pullSocket, 0);
    if (rc == -1) {
      write_logfile(DBGMSG_ERROR, "com", "Unable to receive message");
      return ZMQ_GENERIC_ERROR;
    } else {
      shm_cfg_ptr[CFG_ZMQ_BYTES_READ] += rc;
    }

    /* Read ZMQ_RCVMORE flag */
    rc = zmq_getsockopt(pullSocket, ZMQ_RCVMORE, &optval, &optlen);
    if (rc != 0) {
      write_logfile(DBGMSG_WARNING, "com", "Unable to get ZMQ_RCVMORE value");
      return ZMQ_OPT_ERROR;
    }

    /* header received*/
    if (!rcvmore && optval) {
      p->header_found = 1;
      shm_cfg_ptr[CFG_FIL_PKG_READ]++;
      status = processHeader(p, &msg);
    }
    if (rcvmore && !optval) {
      if (status == ZMQ_NO_ERROR) {
        status = processData(p, &msg);
      }
    }
    /* if (!rcvmore && !optval) { /\* is this true? -> not on CAMEA (can be pause or no neutrons)*\/ */
    /*   sinqhmlog(DBGMSG_WARNING, "com", "Missing data message"); */
    /*   shm_cfg_ptr[CFG_FIL_PKG_INCOMPLETE]++; */
    /*   return ZMQ_INCOMPLETE_PACKAGE; */
    /* } */
    if (rcvmore && optval) {
      status = ZMQ_INCOMPLETE_PACKAGE;
      write_logfile(DBGMSG_ERROR, "com", "Multi packet data message");
    }

    rcvmore = optval;
    zmq_msg_close(&msg);
  } while (rcvmore);

  if (status != ZMQ_NO_ERROR) {
    shm_cfg_ptr[CFG_FIL_PKG_INCOMPLETE]++;
  }
  return status;
}
