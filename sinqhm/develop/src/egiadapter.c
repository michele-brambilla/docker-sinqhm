

/*******************************************************************************
  egiadapter.c
*******************************************************************************/

/**
 * This is an adapter between the general part of the SINQ histogram
 * memory software and the EGI interface of the appWeb http server.
 *
 * Mark Koennecke, Gerd Theidel, September 2005
 */

/*******************************************************************************
  includes
*******************************************************************************/

#include <appWeb.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define BUILD_CFG_DESCR
#include "controlshm.h"
#undef BUILD_CFG_DESCR

#define BUILD_DATA_DESCR
#include "datashm.h"
#undef BUILD_DATA_DESCR

#define USE_CAPI
#include "appWrapper.h"
#undef USE_CAPI

#include "debugshm.h"
#include "egiadapter.h"
#include "lwlpmc.h"
#include "sinqhm_errors.h"
#include "sinqhmsrv.h"

/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define BUFF_LEN 1024

#define SINQHM_DO_REQUEST_LOG 1

#if SINQHM_DO_REQUEST_LOG
#define requestLog(rq, uri) doRequestLog(rq, uri)
#else
#define requestLog(rq, uri)
#endif

/*******************************************************************************
  global vars
*******************************************************************************/

static char *sinqhmConfig = NULL;
static int displimit = 1000;
static uint32_t dispmask = 0;
static uint32_t dispexpect = 0;
static uint32_t dispnotexp = 0;
static int packetinfo = 0;

extern MaServer *mainServer;

/*******************************************************************************
  function prototypes
*******************************************************************************/

char *maGetFileVar(MaRequest *rq, char *file, char *var, char *defaultValue);

static void showConfigEgi(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen);

static void rawDataGUI(MaRequest *rq, char *script, char *uri, char *query,
                       char *postData, int postLen);

static void showDebugBuffer(MaRequest *rq, char *script, char *uri, char *query,
                            char *postData, int postLen);

static void showConfigShm(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen);

static void showDataShm(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen);

static void menuEgi(MaRequest *rq, char *script, char *uri, char *query,
                    char *postData, int postLen);

// static void indexEgi(MaRequest *rq, char *script, char *uri, char *query,
//                  char *postData, int postLen);

static void readDataEgi(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen);

static void statusEgi(MaRequest *rq, char *script, char *uri, char *query,
                      char *postData, int postLen);

/*******************************************************************************
  function declarations
*******************************************************************************/

#define MPR_STDOUT 1

int logFd = -1;
char logFileName[256]; // Current name of log file
int logMaxSize;        // Maximum extent of trace file
int logRotationCount;

/*******************************************************************************
 *
 * FUNCTION
 *   rotateLog
 *
 ******************************************************************************/

void rotateLog() {
  char buffer[1024];
  int printed_len;

  snprintf(buffer, sizeof(buffer), "%s.old", logFileName);
  unlink(buffer);

  if (logFd > 2) {
    close(logFd);
  }

  if (rename(logFileName, buffer) != 0) {
    unlink(logFileName);
  }

  logFd = open(logFileName, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);

  if (logRotationCount) {
    printed_len = snprintf(buffer, sizeof(buffer), "Log rotation count: %u\n",
                           logRotationCount);
    write(logFd, buffer, printed_len);
  }
  logRotationCount++;
}

/*******************************************************************************
 *
 * FUNCTION
 *   openLog
 *
 ******************************************************************************/

void openLog(char *filename, int size) {

  strncpy(logFileName, filename, sizeof(logFileName));
  logMaxSize = size;
  logRotationCount = 0;

  if (strcmp(logFileName, "stdout") == 0) {
    logFd = MPR_STDOUT;
  } else {
    rotateLog();
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   closeLog
 *
 ******************************************************************************/

void closeLog(void) {

  if (logFd > 2) {
    close(logFd);
    logFd = -1;
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   requestLog
 *
 ******************************************************************************/

void doRequestLog(MaRequest *rq, char *uri) {
  char *remote_addr;
  char buffer[1024];
  int printed_len;

  if (logFd < 0) {
    return;
  }

  printed_len = datetimestr(buffer, 1024);
  remote_addr = maGetVar(rq, MA_REQUEST_OBJ, "REMOTE_ADDR", "UNKNOWN");
  printed_len += snprintf(buffer + printed_len, 1024 - printed_len,
                          "Request from %s: %s\n", remote_addr, uri);

  write(logFd, buffer, printed_len);

  if (logFd != MPR_STDOUT) {
    struct stat sbuf;
    if (logMaxSize <= 0 || fstat(logFd, &sbuf) < 0) {
      return;
    }
    //
    //  Rotate logs when full
    //
    if (sbuf.st_mode & S_IFREG && (unsigned)sbuf.st_size > logMaxSize) {
      rotateLog();
    }
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   forwardReq
 *
 ******************************************************************************/

void forwardReq(char *caller, MaRequest *rq, char *script, char *uri,
                char *query, char *postData, int postLen) {
  if (strcmp(caller, "debugshm") == 0) {
    maRedirect(rq, 301, "/admin/debugbuffer.egi");
  } else if (strcmp(caller, "rawdata") == 0) {
    maRedirect(rq, 301, "/admin/rawdata.egi");
  } else if (strcmp(caller, "configshm") == 0) {
    maRedirect(rq, 301, "/admin/configshm.egi");
  } else if (strcmp(caller, "datashm") == 0) {
    maRedirect(rq, 301, "/admin/datashm.egi");
  } else if (strcmp(caller, "menu") == 0) {
    maRedirect(rq, 301, "/public/menu.egi");
  } else if (strcmp(caller, "configuration") == 0) {
    maRedirect(rq, 301, "/admin/showconfig.egi");
  } else if (strcmp(caller, "readdata") == 0) {
    maRedirect(rq, 301, "/admin/readdata.egi");
  } else if (strcmp(caller, "status") == 0) {
    maRedirect(rq, 301, "/public/status.egi");
  } else {
    maSetResponseCode(rq, 200);
    maSetHeader(rq, "Content-type: text/plain", 0);
    maWriteFmt(rq, "ERROR: unknown caller: %s", caller);
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   appWebGetFileVar
 *
 * DESCRIPTION
 *   function to acces variables created by the uploadHandler
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 *
 ******************************************************************************/

char *appWebGetFileVar(MaRequest *rq, char *file, char *var,
                       char *defaultValue) {
  MprVar *vp, *vp2, *variables;
  char *retValue = 0;

  variables = maGetVariables(rq);

  vp = mprGetProperty(&variables[MA_FILES_OBJ], file, 0);
  if (vp) {
    vp2 = mprGetProperty(vp, var, 0);
    if (vp2 && vp2->type == MPR_TYPE_STRING) {
      retValue = vp2->string;
    }
  }

  if (retValue) {
    return retValue;
  } else {
    return defaultValue;
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   insertFile
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

static void insertFile(MaRequest *rq, char *filename) {
  char char_buff[BUFF_LEN];
  int file, len;

  snprintf(char_buff, BUFF_LEN, "%s/%s", maGetDocumentRoot(mainServer),
           filename);

  if ((file = open(char_buff, O_RDONLY)) == -1)
    return;

  do {
    len = read(file, char_buff, BUFF_LEN - 1);
    char_buff[len] = 0;
    maWriteStr(rq, char_buff);
  } while (len > 0);

  close(file);
}

/*******************************************************************************
 *
 * FUNCTION
 *   timediffstr
 ******************************************************************************/
int timediffstr(char *buffer, int len, time_t time1, time_t time2) {
  time_t diff;
  int hours, min, sec;
  diff = time1 - time2;
  hours = diff / 3600;
  sec = diff - (hours * 3600);
  min = sec / 60;
  sec = sec - (min * 60);

  return snprintf(buffer, len, "%4d:%02d:%02d", hours, min, sec);
}

/*******************************************************************************
 *
 * FUNCTION
 *   insertStatusBar
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

static void insertStatusBar(MaRequest *rq, char *caller) {
  time_t currentTime;
  char buffer[256];
  volatile unsigned int *shm_cfg_ptr;
  volatile unsigned int *dbg_buff_ptr;

  shm_cfg_ptr = getVarPointer(CFG_BASE_PTR);
  dbg_buff_ptr = getDebugBuffPtr();
  if (!shm_cfg_ptr || !dbg_buff_ptr)
    return;

  maWriteStr(rq, "<table  bgcolor=\"silver\" border=\"1\"><tbody> <tr>");

  maWriteFmt(rq,
             "<td><a href=\"/admin/guistartdaq.egi?%s\"><img "
             "src=\"/icons/cstart.bmp\"  border=\"0\" alt=\"START\"></a></td>",
             caller);
  maWriteFmt(rq,
             "<td><a href=\"/admin/guipausedaq.egi?%s\"><img "
             "src=\"/icons/cpause.bmp\"  border=\"0\" alt=\"PAUSE\"></a></td>",
             caller);
  maWriteFmt(rq,
             "<td><a href=\"/admin/guistopdaq.egi?%s\"><img "
             "src=\"/icons/cstop.bmp\"  border=\"0\" alt=\"STOP\"></a></td>",
             caller);

  // width=\"160\" height=\"34\"

  maWriteStr(rq, "<td>Status: </td>");
  if (shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK]) {
    if (shm_cfg_ptr[CFG_SRV_DAQ_PAUSE_CMD]) {
      maWriteStr(rq, "<td bgcolor=\"#ffcc33\"> DAQ Paused");
    } else {
      maWriteStr(rq, "<td bgcolor=\"#009900\"> DAQ Running");
    }
  } else {
    maWriteStr(rq, "<td bgcolor=\"#ff3300\"> DAQ Stopped");
  }

  sprintf(buffer, "<td> LWL Packets read: %u </td>",
          shm_cfg_ptr[CFG_FIL_PKG_READ]);
  maWriteStr(rq, buffer);

  time(&currentTime);

  if (shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]) {
    if (shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME]) {
      timediffstr(buffer, 256, (time_t)shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME],
                  (time_t)shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]);
    } else {
      timediffstr(buffer, 256, currentTime,
                  (time_t)shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]);
    }
  } else {
    timediffstr(buffer, 256, (time_t)0, (time_t)0);
  }

  maWriteFmt(rq, "<td>Acqusition Time: %s </td>", buffer);

  timediffstr(buffer, 256, currentTime,
              (time_t)shm_cfg_ptr[CFG_FIL_MOD_INST_TIME]);
  maWriteFmt(rq, "<td>HM Uptime: %s </td>", buffer);

  if (dbg_buff_ptr[DBG_SRV_ERR_BUFF_ACK] !=
      dbg_buff_ptr[DBG_FIL_ERR_BUFF_SGN]) {
    maWriteStr(rq, "<td bgcolor=\"#ff3300\"> State: ERROR");
  } else {
    maWriteStr(rq, "<td bgcolor=\"#009900\"> State: OK");
  }

  maWriteFmt(rq, " </tr>\n");
  maWriteStr(rq, "</tbody></table>");
}

/*******************************************************************************
 *
 * FUNCTION
 *   insertHeader
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

static void insertHeader(MaRequest *rq, char *title, int autorefresh) {

  maWriteFmt(rq, "<html><head><title>%s</title>", title);

  if (autorefresh > 0) {
    maWriteFmt(rq, "<META HTTP-EQUIV=REFRESH CONTENT=%d>", autorefresh);
  }

  insertFile(rq, "/public/header.html");
}

/*******************************************************************************
 *
 * FUNCTION
 *   insertFooter
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

static void insertFooter(MaRequest *rq) {
  insertFile(rq, "/public/footer.html");
}

/*******************************************************************************
 *
 * FUNCTION
 *
 *
 * DESCRIPTION
 *
 *
 * PARAMETERS
 *
 *
 * RETURNS
 *
 * REMARKS
 *   Originally I wanted a 500 http code plus a text message explaining
 *   the error. appWeb did not like this: So I send a 200 message at all
 *   times and write a text message which is either OK or ERROR: bla bla
 *   in case of errors (Mark)
 *
 ******************************************************************************/

static void httpText(MaRequest *rq, int code, char *text) {
  maSetResponseCode(rq, 200);
  maSetHeader(rq, "Content-type: text/plain", 0);
  if (code == 200) {
    maWriteFmt(rq, "OK: %s\r\n", text);
  } else {
    maWriteFmt(rq, "ERROR: %s\r\n", text);
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   formatAxis
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

static void formatAxis(MaRequest *rq, int bank, int axis) {
  int length, i, mapping, linecount, rank;
  volatile unsigned int *axisData_ptr = NULL;
  char buffer[80];
  volatile axis_descr_type *axis_descr_ptr;

  axis_descr_ptr = getAxisDescription(bank, axis);

  maWriteFmt(rq, "<h3>Axis %d</h3><p>\r\n", axis);
  maWriteStr(rq, "<table>\r\n");

  length = axis_descr_ptr->length;

  maWriteFmt(rq, "<tr><th>Length</th><td>%d</td></tr>\r\n", length);

  mapping = axis_descr_ptr->type;

  switch (mapping) {
  case AXDIRECT:
    strcpy(buffer, "Direct");
    break;
  case AXCALC:
    strcpy(buffer, "Calculation");
    break;
  case AXBOUNDARY:
    strcpy(buffer, "Boundary Array");
    break;
  case AXLOOKUP:
    strcpy(buffer, "Lookup Table");
    break;
  case AXMAP:
    strcpy(buffer, "Map Table");
    break;
  default:
    snprintf(buffer, 80, "Unknown mapping ID %d", mapping);
    break;
  }
  maWriteFmt(rq, "<tr><th>Mapping</th><td>%s</td></tr>\r\n", buffer);

  axisData_ptr = getAxisData(axis_descr_ptr);

  switch (mapping) {
  case AXDIRECT:
    maWriteStr(rq, "</table></p>\r\n");
    break;
  case AXCALC:
    if (axisData_ptr != NULL) {
      maWriteFmt(rq, "<tr><th>Multiplier</th><td>%d</td></tr>\r\n",
                 axisData_ptr[0]);
      maWriteFmt(rq, "<tr><th>PreOffset</th><td>%d</td></tr>\r\n",
                 axisData_ptr[1]);
      maWriteFmt(rq, "<tr><th>Divisor</th><td>%d</td></tr>\r\n",
                 axisData_ptr[2]);
      maWriteFmt(rq, "<tr><th>PostOffset</th><td>%d</td></tr>\r\n",
                 axisData_ptr[3]);
    }
    maWriteStr(rq, "</table></p>\r\n");
    break;
  case AXBOUNDARY:
    maWriteFmt(rq, "<tr><th>threshold</th><td>%d</td></tr>\r\n",
               axis_descr_ptr->threshold);
    maWriteFmt(rq, "<tr><th>offset</th><td>%d</td></tr>\r\n",
               axis_descr_ptr->offset);
  case AXLOOKUP:
  case AXMAP:
    rank = axisData_ptr[0];
    maWriteFmt(rq, "<tr><th>rank</th><td>%d</td></tr>\r\n", rank);
    length = 1;
    for (i = 0; i < rank; i++) {
      maWriteFmt(rq, "<tr><th>dim</th><td>%d</td></tr>\r\n",
                 axisData_ptr[i + 1]);
      length *= axisData_ptr[i + 1];
    }
    maWriteStr(rq, "</table></p>\r\n");
    if (axisData_ptr != NULL) {
      maWriteStr(rq, "<p><table><caption>Array Data</caption>\r\n");
      linecount = 0;
      for (i = 0; i < length; i++) {
        if (linecount == 0) {
          maWriteStr(rq, "<tr>");
        }
        maWriteFmt(rq, "<td>%d</td>", axisData_ptr[i + rank + 1]);
        linecount++;
        if (linecount >= 9) {
          maWriteStr(rq, "</tr>\r\n");
          linecount = 0;
        }
      }
    }
    maWriteStr(rq, "</table></p>\r\n");
    break;
    /* case AXMAP: */
    /*   maWriteStr(rq,"</table></p>\r\n"); */
    /*   break; */
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   formatBankInfo
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

static void formatBankInfo(MaRequest *rq, int bank) {
  int axis, naxis;
  volatile bank_descr_type *bank_descr_ptr;

  maWriteFmt(rq, "<h2>Bank %d</h2><p>\r\n", bank);

  bank_descr_ptr = getBankDescription(bank);
  naxis = bank_descr_ptr->rank;

  maWriteStr(rq, "<table><tr><th>Rank</th>");
  maWriteFmt(rq, "<td>%d</td></tr></table><p>\r\n", naxis);
  for (axis = 0; axis < naxis; axis++) {
    formatAxis(rq, bank, axis);
  }

  maWriteStr(rq, "<hr>\r\n");
}

/*******************************************************************************
 *
 * FUNCTION
 *   configureEgi
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
static void configureEgi(MaRequest *rq, char *script, char *uri, char *query,
                         char *postData, int postLen) {
  int status;
  char buffer[132];
  FILE *fd = NULL;

  requestLog(rq, uri);

  /*
   * check that DAQ is not running
   */
  getControlVar(CFG_FIL_DO_DAQ_ACK, &status);
  if (status > 0) {
    httpText(rq, 500, "Cannot configure when DAQ is active");
    return;
  }

  /**
   * check if the configuration we received is somewhat useful
   */
  status = configureHistogramMemory(postData, 1);
  if (status < 0) {
    dataErrorToText(status, buffer, 131);
    httpText(rq, 500, buffer);
    return;
  }

  /*
   * now do actively configure
   */
  status = configureHistogramMemory(postData, 0);
  if (status < 0) {
    dataErrorToText(status, buffer, 131);
    httpText(rq, 500, buffer);
    return;
  }

  /*
   * seemed to have had success: replace the default configuration file
   */
  if (sinqhmConfig != NULL) {
    fd = fopen(sinqhmConfig, "w");
    if (fd != NULL) {
      fprintf(fd, "%s", postData);
      fclose(fd);
    }
  }

  httpText(rq, 200, "Histogram Memory successfully configured");
}

/*******************************************************************************
 *
 * FUNCTION
 *   configureEgiGUI
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

static void configureEgiGUI(MaRequest *rq, char *script, char *uri, char *query,
                            char *postData, int postLen) {

  int status;
  char buffer[1024];
  char *filename, *clientfilename, *storeasdef, *restorefromdef;
  int wlen, rlen;

  FILE *fin = NULL;
  FILE *fout = NULL;

  requestLog(rq, uri);

  /*
   *  For convenience, decode and convert each post data variable
   *  into the hashed environment
   */
  if (postLen > 0) {
    maCreateEnvVars(rq, postData, postLen);
  }

  filename = appWebGetFileVar(rq, "myfile", "FILENAME", "\0");
  clientfilename = appWebGetFileVar(rq, "myfile", "CLIENT_FILENAME", "\0");
  storeasdef = maGetVar(rq, MA_FORM_OBJ, "storeasdef", "off");
  restorefromdef = maGetVar(rq, MA_FORM_OBJ, "restorefromdef", "off");

  // MA_FILES_OBJ
  /*
   * check that DAQ is not running
   */
  getControlVar(CFG_FIL_DO_DAQ_ACK, &status);
  if (status > 0) {
    httpText(rq, 500, "Cannot configure when DAQ is active");
    return;
  }

  if (filename[0] && clientfilename[0]) {
    /**
     * check if the configuration we received is somewhat useful
     */

    status = configureHistogramMemoryFromFile(filename, 1);
    if (status < 0) {
      dataErrorToText(status, buffer, 131);
      httpText(rq, 500, buffer);
      return;
    }

    /*
     * now do actively configure
     */
    status = configureHistogramMemoryFromFile(filename, 0);
    if (status < 0) {
      dataErrorToText(status, buffer, 131);
      httpText(rq, 500, buffer);
      return;
    }

    if ((sinqhmConfig != NULL) && (mprStrCmpAnyCase(storeasdef, "on") == 0)) {
      /*
       * seemed to have had success: replace the default configuration file
       */
      // copy file ...

      fin = fopen(filename, "r");
      if (fin != NULL) {
        fout = fopen(sinqhmConfig, "w");
        if (fout != NULL) {
          while (!feof(fin)) // for all bytes ...
          {
            rlen = fread(buffer, 1, 1024, fin);
            wlen = fwrite(buffer, 1, rlen, fout);

            if (rlen != wlen) {
              printf("ERROR: copying configuration file");
              // tbd: Error
            }
          }
          fclose(fout);
        }
        fclose(fin);
      }
    }
  }

  if ((sinqhmConfig != NULL) && (mprStrCmpAnyCase(restorefromdef, "on") == 0)) {
    /*
     * now do actively configure
     */
    status = configureHistogramMemoryFromFile(sinqhmConfig, 0);
    if (status < 0) {
      dataErrorToText(status, buffer, 131);
      httpText(rq, 500, buffer);
      return;
    }
  }

  showConfigEgi(rq, script, uri, query, postData, postLen);
  // TBD: failed
}

/*******************************************************************************
 *
 * FUNCTION
 *   presetHMEgi
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

static void presetHMEgi(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen) {

  int i, j, nbank, length;
  char buffer[256];
  volatile unsigned int *dataPtr;
  volatile histo_descr_type *histo_descr_ptr;
  int value;
  const char *var = NULL;

  requestLog(rq, uri);

  histo_descr_ptr = getShmHistoPtr();

  nbank = histo_descr_ptr->nBank;

  var = maGetVar(rq, MA_FORM_OBJ, "value", NULL);
  if (var == NULL) {
    value = 0;
  } else {
    value = atoi(var);
  }

  for (i = 0; i < nbank; i++) {
    length = getBankDataSize(i) / sizeof(uint32_t);
    if (length < 0) {
      dataErrorToText(length, buffer, 255);
      httpText(rq, 500, buffer);
      return;
    }
    dataPtr = getBankData(i);
    if (dataPtr == NULL) {
      httpText(rq, 500, "Bad Data Pointer");
      return;
    }
    for (j = 0; j < length; j++) {
      dataPtr[j] = value;
    }
  }
  httpText(rq, 200, "Successfully preset Histogram Memory");
}
/*******************************************************************************
 *
 * FUNCTION
 *   writeTestDataEgi
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

static void writeTestDataEgi(MaRequest *rq, char *script, char *uri,
                             char *query, char *postData, int postLen) {

  int i, j, nbank, length;
  char buffer[256];
  volatile unsigned int *dataPtr;
  volatile histo_descr_type *histo_descr_ptr;

  requestLog(rq, uri);

  histo_descr_ptr = getShmHistoPtr();

  nbank = histo_descr_ptr->nBank;

  for (i = 0; i < nbank; i++) {
    length = getBankDataSize(i) / sizeof(uint32_t);
    if (length < 0) {
      dataErrorToText(length, buffer, 255);
      httpText(rq, 500, buffer);
      return;
    }
    dataPtr = getBankData(i);
    if (dataPtr == NULL) {
      httpText(rq, 500, "Bad Data Pointer");
      return;
    }
    for (j = 0; j < length; j++) {
      dataPtr[j] = (i << 28) + (j & 0x0FFFFFFF);
    }
  }
  httpText(rq, 200, "Successfully generated test data");
}

/*******************************************************************************
 *
 * FUNCTION
 *   readHMDataEgi
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

static void readHMDataEgi(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen) {
  int bankno, start, end, length;
  char *var, buffer[256];

  volatile unsigned int *dataPtr = NULL;

  requestLog(rq, uri);

  var = maGetVar(rq, MA_FORM_OBJ, "bank", NULL);
  if (var == NULL) {
    bankno = 0;
  } else {
    bankno = atoi(var);
  }
  length = getBankDataSize(bankno) / sizeof(uint32_t);
  if (length < 0) {
    dataErrorToText(length, buffer, 255);
    httpText(rq, 500, buffer);
    return;
  }
  var = maGetVar(rq, MA_FORM_OBJ, "start", NULL);
  if (var == NULL) {
    start = 0;
  } else {
    start = atoi(var);
  }
  var = maGetVar(rq, MA_FORM_OBJ, "end", NULL);
  if (var == NULL) {
    end = length;
  } else {
    end = atoi(var);
  }
  if (start < 0) {
    start = 0;
  }
  if (start > length) {
    start = length - 1;
  }

  if (end < 0) {
    end = length;
  }
  if (end < start) {
    end = start + 1;
  }
  if (end > length) {
    end = length;
  }

#ifdef TARGET_HAS_LITTLE_ENDIAN
// use getTransferData(int bank, int start, int end, int **data)
#endif

  dataPtr = getBankData(bankno);

  if (dataPtr == 0) {
    dataErrorToText(SINQHM_ERR_NOSUCHBANK, buffer, 255);
    httpText(rq, 500, buffer);
    return;
  }
  maSetResponseCode(rq, 200);
  maSetHeader(rq, "Content-type: application/x-sinqhm", 0);
  snprintf(buffer, 255, "Content-length: %lu", (end - start) * sizeof(int));
  maSetHeader(rq, buffer, 0);
  maWrite(rq, (char *)&dataPtr[start], (end - start) * sizeof(int));
}

/*******************************************************************************
 *
 * FUNCTION
 *   processHMDataEgi
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

static void processHMDataEgi(MaRequest *rq, char *script, char *uri,
                             char *query, char *postData, int postLen) {
  int bankno, i, length;
  char *var, buffer[256];
  pNXDS result = NULL;

  requestLog(rq, uri);

  var = maGetVar(rq, MA_FORM_OBJ, "bank", NULL);
  if (var == NULL) {
    bankno = 0;
  } else {
    bankno = atoi(var);
  }

  var = maGetVar(rq, MA_FORM_OBJ, "command", NULL);
  if (var == NULL) {
    httpText(rq, 500, "ERROR: command to process not  found");
    return;
  }

  result = processData(bankno, var, buffer, 255);
  if (result == NULL) {
    httpText(rq, 500, buffer);
    return;
  }

  for (i = 0, length = 1; i < result->rank; i++) {
    length *= result->dim[i];
  }

#ifdef TARGET_HAS_LITTLE_ENDIAN
  for (i = 0; i < length; i++) {
    result->u.iPtr[i] = htonl(result->u.iPtr[i]);
  }
#endif

  maSetResponseCode(rq, 200);
  maSetHeader(rq, "Content-type: application/x-sinqhm", 0);
  snprintf(buffer, 255, "Content-length: %lu", length * sizeof(int));
  maSetHeader(rq, buffer, 0);
  maWrite(rq, result->u.cPtr, length * sizeof(int));

  dropNXDataset(result);
}

/*******************************************************************************
 *
 * FUNCTION
 *   printDaqStatus
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

static void printDaqStatus(MaRequest *rq) {
  char buffer[256];
  volatile axis_descr_type *axis_descr_ptr;
  int axis;
  uint bank;
  volatile histo_descr_type *histo_descr_ptr;
  volatile unsigned int *shm_cfg_ptr;
  time_t currentTime;

  shm_cfg_ptr = getVarPointer(CFG_BASE_PTR);

  time(&currentTime);

  maWriteStr(rq, "State                   :  ");
  if (shm_cfg_ptr[CFG_FIL_DO_DAQ_ACK]) {
    if (shm_cfg_ptr[CFG_SRV_DAQ_PAUSE_CMD]) {
      maWriteStr(rq, "DAQ Paused\n");
    } else {
      maWriteStr(rq, "DAQ Running\n");
    }
  } else {
    maWriteStr(rq, "DAQ Stopped\n");
  }

  maWriteStr(rq, "Acquisition started     :  ");
  if (!shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]) {
    maWriteStr(rq, "not yet started\n");
  }
  /* if (shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]) */
  /*   { */
  /*     printf("\tCFG_FIL_ACQ_START_TIME =
   * %d\n",shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]); */
  /*     printf("\tCFG_FIL_ACQ_START_TIME =
   * %x\n",&shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]); */

  /*     ///////////////// */
  /*     // MiB I had to remove this to have everything working */
  /*     /\*
   * maWriteStr(rq,ctime((time_t*)&shm_cfg_ptr[CFG_FIL_ACQ_START_TIME])); *\/ */
  /*   } */
  /* else */
  /* { */
  /*   maWriteStr(rq,"not yet started\n"); */
  /* } */

  if (shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]) {
    if (shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME]) {
      timediffstr(buffer, 256, (time_t)shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME],
                  (time_t)shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]);
    } else {
      timediffstr(buffer, 256, currentTime,
                  (time_t)shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]);
    }
  } else {
    timediffstr(buffer, 256, (time_t)0, (time_t)0);
  }

  maWriteFmt(rq, "Acquisition Time        : %s\n\n", buffer);

  histo_descr_ptr = getShmHistoPtr();

  maWriteFmt(rq, "Events processed by filler       : %10u\n",
             shm_cfg_ptr[CFG_FIL_EVT_PROCESSED]);
  maWriteFmt(rq, "Events skipped due to mask       : %10u\n",
             shm_cfg_ptr[CFG_FIL_EVT_SKIPPED]);
  maWriteFmt(rq, "Events unknown                   : %10u\n",
             shm_cfg_ptr[CFG_FIL_PKG_UNKNOWN]);
  maWriteStr(rq, "\n");

  for (bank = 0; bank < histo_descr_ptr->nBank; bank++) {
    axis = 0;
    do {
      axis_descr_ptr = getAxisDescription(bank, axis++);
      if (axis_descr_ptr) {
        maWriteFmt(rq, "bank %d:  axis %d: ", bank, axis);
        maWriteFmt(rq, "  counts low:  %10u", axis_descr_ptr->cnt_low);
        maWriteFmt(rq, "  counts high: %10u\n", axis_descr_ptr->cnt_high);
      }
    } while (axis_descr_ptr);
  }
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Header DAQ Mask                  : 0x%08x\n",
             shm_cfg_ptr[CFG_SRV_HDR_DAQ_MASK]);
  maWriteFmt(rq, "Expected result for DAQ active   : 0x%08x\n",
             shm_cfg_ptr[CFG_SRV_HDR_DAQ_ACTIVE]);
  maWriteStr(rq, "\n");

  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Packets read                     : %10u\n",
             shm_cfg_ptr[CFG_FIL_PKG_READ]);
  maWriteFmt(rq, "Packets incomplete               : %10u\n",
             shm_cfg_ptr[CFG_FIL_PKG_INCOMPLETE]);
  maWriteFmt(rq, "Packets oversized                : %10u\n",
             shm_cfg_ptr[CFG_FIL_PKG_OVERSIZE]);
  maWriteStr(rq, "\n");

#ifndef HAVE_0MQ
  maWriteFmt(rq, "TSI Packets (total)              : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_COUNT]);
  maWriteFmt(rq, "TSI Packets with DAQ started     : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_STARTED]);
  maWriteFmt(rq, "TSI Packets with DAQ paused      : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_PAUSED]);
  maWriteFmt(rq, "TSI Packets with DAQ stopped     : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_STOPPED]);
  maWriteStr(rq, "\n");

  maWriteFmt(rq, "TSI Packets with DAQ active Mask : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_ACTIVE]);
  maWriteFmt(rq, "TSI Packets with SYNC0           : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC0]);
  maWriteFmt(rq, "TSI Packets with SYNC1           : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC1]);
  maWriteFmt(rq, "TSI Packets with SYNC2           : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC2]);
  maWriteFmt(rq, "TSI Packets with SYNC3           : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_SYNC3]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "PSD Detector Hits (from TDC)     : %10u\n",
             shm_cfg_ptr[CFG_FIL_PSD_HITS]);
  maWriteFmt(rq, "PSD Detector Flashes (from TDC)  : %10u\n",
             shm_cfg_ptr[CFG_FIL_PSD_FLASH]);
  maWriteStr(rq, "\n");

  //  maWriteFmt(rq,"TSI Packets with Rate Low       :
  //  %10d\n",shm_cfg_ptr[CFG_FIL_TSI_RATE_LOW]); maWriteFmt(rq,"TSI Packets
  //  with Power Fail     : %10d\n",shm_cfg_ptr[CFG_FIL_TSI_POWER_FAIL]);
  maWriteFmt(rq, "TSI Packets with SWC changed     : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_STATUS_WORD_CHANGED]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Detected Fifo Overflows          : %10u\n",
             shm_cfg_ptr[CFG_FIL_CNT_FIFO_OVERFLOW]);
  maWriteFmt(rq, "Detected Taxi Chip Erros         : %10u\n",
             shm_cfg_ptr[CFG_FIL_CNT_TAXI_ERROR]);
//  maWriteFmt(rq,"Detected Mdif Power Failuires   :
//  %10d\n",shm_cfg_ptr[CFG_FIL_CNT_POWER_FAIL]);
#else
  maWriteFmt(rq, "0MQ Packets with DAQ started     : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_STARTED]);
  maWriteFmt(rq, "0MQ Packets with DAQ paused      : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_PAUSED]);
  maWriteFmt(rq, "0MQ Packets with DAQ stopped     : %10u\n",
             shm_cfg_ptr[CFG_FIL_TSI_DAQ_STOPPED]);
#endif
}

/*******************************************************************************
 *
 * FUNCTION
 *   daqStatusEgi
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

static void daqStatusEgi(MaRequest *rq, char *script, char *uri, char *query,
                         char *postData, int postLen) {
  requestLog(rq, uri);

  maSetResponseCode(rq, 200);
  maSetHeader(rq, "Content-type: text/plain", 0);

  printDaqStatus(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   statusEgi
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

static void statusEgi(MaRequest *rq, char *script, char *uri, char *query,
                      char *postData, int postLen) {
  char buffer[256];
  volatile unsigned int *shm_cfg_ptr;
  uint32_t status;
  uint32_t version;
  time_t currentTime;
  hostStats_type hostStats;

  shm_cfg_ptr = getVarPointer(CFG_BASE_PTR);

  gethostname(buffer, 255);
  time(&currentTime);

  maWriteStr(rq, "\n<PRE>\n");

  maWriteFmt(rq, "<h1>Status of %s</h1>", buffer);

  /******************************************************************************/
  maWriteFmt(rq, "<h2>Current Acquisition</h2>\n");
  /******************************************************************************/
  printDaqStatus(rq);

  /******************************************************************************/
  maWriteFmt(rq, "<h2>Current LWL Status</h2>\n");
  /******************************************************************************/

  maWriteFmt(rq, "Last TSI Header                  : 0x%08x\n",
             shm_cfg_ptr[CFG_FIL_TSI_HEADER]);

  status = shm_cfg_ptr[CFG_FIL_STATUS_LWL];
  maWriteFmt(
      rq, "PMC LWL Status Register          : 0x%08x   %s %s %s %s %s %s %s\n",
      status, ((status & LWL_STATUS_PF) ? "PF " : "   "),
      ((status & LWL_STATUS_SWC) ? "SWC" : "   "),
      ((status & LWL_STATUS_NRL) ? "NRL" : "   "),
      ((status & LWL_STATUS_VLNT) ? "ERR" : "   "),
      ((status & LWL_STATUS_FF) ? "FF " : "   "),
      ((status & LWL_STATUS_HF) ? "HF " : "   "),
      ((status & LWL_STATUS_EF) ? "EF " : "   "));

  ///////////
  // MiB
  /******************************************************************************/
  maWriteFmt(rq, "<h2>Current 0MQ Status</h2>\n");
  /******************************************************************************/

  maWriteFmt(rq, "0MQ socket address                   : 0x%x\n",
             shm_cfg_ptr[CFG_FIL_ZMQ_SOCK_ADDR]);

  maWriteFmt(rq, "0MQ timeout status                   : %d\n",
             shm_cfg_ptr[CFG_FIL_ZMQ_RCVTO]);
  maWriteFmt(rq, "0MQ packet type                      : %d\n",
             shm_cfg_ptr[CFG_FIL_ZMQ_PKG_TYPE]);

  maWriteFmt(rq, "0MQ bandwith (last 5 s)              : %lf MB/s\n",
             shm_cfg_ptr[CFG_ZMQ_BYTES_READ] * 1e-6 / 5);
  shm_cfg_ptr[CFG_ZMQ_BYTES_READ] = 0;
  ///////////////

  /******************************************************************************/
  maWriteFmt(rq, "<h2>Summed Counts since Boot</h2>\n");
  /******************************************************************************/

  maWriteFmt(rq, "Acquisitions since Boot          : %10u\n",
             shm_cfg_ptr[CFG_FIL_ACQ_COUNT]);

  maWriteFmt(rq, "Packets read (low word)          : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PKG_READ_LOW]);
  maWriteFmt(rq, "Packets read (high word)         : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PKG_READ_HIGH]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Packets incomplete               : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PKG_INCOMPLETE]);
  maWriteFmt(rq, "Packets oversized                : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PKG_OVERSIZE]);
  maWriteFmt(rq, "Packets unknown                  : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PKG_UNKNOWN]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Detected Fifo Overflows          : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_CNT_FIFO_OVERFLOW]);
  maWriteFmt(rq, "Detected Taxi Chip Erros         : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_CNT_TAXI_ERROR]);
  //  maWriteFmt(rq,"Detected Mdif Power Failuires   :
  //  %10d\n",shm_cfg_ptr[CFG_FIL_SUM_CNT_POWER_FAIL]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Total Number of TSI Packets      : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_TSI_COUNT]);
  maWriteFmt(rq, "TSI Packets with DAQ started     : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_TSI_DAQ_STARTED]);
  maWriteFmt(rq, "TSI Packets with DAQ paused      : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_TSI_DAQ_PAUSED]);
  maWriteFmt(rq, "TSI Packets with DAQ stopped     : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_TSI_DAQ_STOPPED]);
  //  maWriteFmt(rq,"TSI Packets with Rate Low       :
  //  %10d\n",shm_cfg_ptr[CFG_FIL_SUM_TSI_RATE_LOW]); maWriteFmt(rq,"TSI Packets
  //  with Power Fail     : %10d\n",shm_cfg_ptr[CFG_FIL_SUM_TSI_POWER_FAIL]);
  maWriteFmt(rq, "TSI Packets with Status changed  : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_TSI_STATUS_WORD_CHANGED]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "PSD Detector Hits (from TDC)     : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PSD_HITS]);
  maWriteFmt(rq, "PSD Detector Flashes (from TDC)  : %10u\n",
             shm_cfg_ptr[CFG_FIL_SUM_PSD_FLASH]);

  /******************************************************************************/
  maWriteFmt(rq, "<h2>Real Time Module</h2>\n");
  /******************************************************************************/

  version = shm_cfg_ptr[CFG_FIL_VERSION];
  maWriteFmt(rq, "RealTime Module Version : %d.%d.%d.%d\n",
             ((version >> 24) & 0x0ff), ((version >> 16) & 0x0ff),
             ((version >> 8) & 0x0ff), (version & 0x0ff));
  maWriteFmt(rq, "RealTime Module Build   : %s\n",
             (char *)&shm_cfg_ptr[CFG_FIL_COMPTIME_BUFF_START]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "RealTime Module started : %s",
             ctime((time_t *)&shm_cfg_ptr[CFG_FIL_MOD_INST_TIME]));
  timediffstr(buffer, 256, currentTime,
              (time_t)shm_cfg_ptr[CFG_FIL_MOD_INST_TIME]);
  maWriteFmt(rq, "HM Uptime               : %s\n", buffer);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Memory Size of Control Section   : %10d Byte\n",
             shm_cfg_ptr[CFG_FIL_CFG_SHM_SIZE]);
  maWriteFmt(rq, "Memory Size of Histogram Section : %10d Byte\n",
             shm_cfg_ptr[CFG_FIL_HST_SHM_SIZE]);
  maWriteFmt(rq, "Memory Size of Debug Section     : %10d Byte\n",
             shm_cfg_ptr[CFG_FIL_DBG_SHM_SIZE]);
  maWriteStr(rq, "\n");
  maWriteFmt(rq, "Task Period      : %d ns\n",
             shm_cfg_ptr[CFG_FIL_TASK_PERIOD]);
  maWriteFmt(rq, "Duty Cycle       : %d percent\n",
             shm_cfg_ptr[CFG_FIL_DUTY_CYCLE]);
  maWriteFmt(rq, "Tic Duration     : %d ns\n",
             shm_cfg_ptr[CFG_FIL_TIC_DURATION]);
  maWriteFmt(
      rq, "Max RT-Task Tics : %d tics (%d ns)\n",
      shm_cfg_ptr[CFG_FIL_MAX_FIL_TICS],
      (shm_cfg_ptr[CFG_FIL_MAX_FIL_TICS] * shm_cfg_ptr[CFG_FIL_TIC_DURATION]));

  /******************************************************************************/
  maWriteFmt(rq, "<h2>Server</h2>\n");
  /******************************************************************************/

  maWriteFmt(rq, "Web Server Version :  %s %s-%s %s\n", BLD_PRODUCT,
             BLD_VERSION, BLD_NUMBER, BLD_TYPE);
  maWriteFmt(rq, "Web Server Build   :  %s %s\n\n", __DATE__, __TIME__);
  maWriteFmt(rq, "Web Server started :  %s\n",
             ctime((time_t *)&shm_cfg_ptr[CFG_SRV_SERVER_START_TIME]));
  maWriteFmt(rq, "Document Root      :  %s\n", maGetDocumentRoot(mainServer));
  maWriteFmt(rq, "Server Root        :  %s\n", maGetServerRoot(mainServer));
  maWriteStr(rq, "\n");

  maGetHostStats(mainServer, &hostStats);
  maWriteFmt(rq, "Access violations                : %10d\n",
             hostStats.accessErrors);
  maWriteFmt(rq, "Currently active requests        : %10d\n",
             hostStats.activeRequests);
  maWriteFmt(rq, "Maximum active requests          : %10d\n",
             hostStats.maxActiveRequests);
  maWriteFmt(rq, "General errors                   : %10d\n", hostStats.errors);
  maWriteFmt(rq, "Requests service on keep-alive   : %10d\n",
             hostStats.keptAlive);
  maWriteFmt(rq, "Total requests                   : %10d\n",
             hostStats.requests);
  maWriteFmt(rq, "Redirections                     : %10d\n",
             hostStats.redirects);
  maWriteFmt(rq, "Request timeouts                 : %10d\n",
             hostStats.timeouts);
  maWriteFmt(rq, "Times buffer had to copy down    : %10d\n",
             hostStats.copyDown);

  maWriteStr(rq, "\n</PRE>\n");
}

/*******************************************************************************
 *   statusAdminEgi
 ******************************************************************************/
#if 0
static void statusPublicEgi(MaRequest *rq, char *script, char *uri, char *query,
  char *postData, int postLen)
{

  insertFile(rq, "/public/status.html");
  statusEgi(rq, script, uri, query, postData, postLen);
  insertFooter(rq);
}
#endif
/*******************************************************************************
 *   statusPublicEgi
 ******************************************************************************/

static void statusPublicEgi(MaRequest *rq, char *script, char *uri, char *query,
                            char *postData, int postLen) {
  requestLog(rq, uri);

  insertHeader(rq, "Status", 5);
  insertStatusBar(rq, "status");
  statusEgi(rq, script, uri, query, postData, postLen);
  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   textStatusEgi
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

static void textStatusEgi(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen) {
  char buffer[256];
  int val, acqtime;
  time_t currentTime;
  volatile unsigned int *shm_cfg_ptr;

  requestLog(rq, uri);

  maSetResponseCode(rq, 200);
  maSetHeader(rq, "Content-type: text/plain", 0);

  gethostname(buffer, 255);
  maWriteFmt(rq, "HM-Host: %s\n", buffer);

  getControlVar(CFG_FIL_DO_DAQ_ACK, &val);
  maWriteFmt(rq, "DAQ: %d\n", val);

  getControlVar(CFG_FIL_STATUS_LWL, &val);
  maWriteFmt(rq, "LWL-Status: %d\n", val);

  getControlVar(CFG_FIL_CNT_FIFO_OVERFLOW, &val);
  maWriteFmt(rq, "LWL-Overflow-Count: %u\n", val);

  getControlVar(CFG_FIL_RATE_VALID, &val);
  maWriteFmt(rq, "Rate-Valid: %d\n", val);

  getControlVar(CFG_FIL_RATE_AN, &val);
  maWriteFmt(rq, "Rate-AN: %d\n", val);

  getControlVar(CFG_FIL_RATE_KV, &val);
  maWriteFmt(rq, "Rate-KV: %d\n", val);

  getControlVar(CFG_FIL_RATE_KH, &val);
  maWriteFmt(rq, "Rate-KH: %d\n", val);

  getControlVar(CFG_FIL_COINC_TIME, &val);
  maWriteFmt(rq, "Coinc-Time: %d\n", val);

  /* 0MQ status */
  getControlVar(CFG_ZMQ_SICS_STATUS, &val);
  maWriteFmt(rq, "0MQ-Status: %d\n", val);

  shm_cfg_ptr = getVarPointer(CFG_BASE_PTR);

  time(&currentTime);

  if (shm_cfg_ptr[CFG_FIL_ACQ_START_TIME]) {
    if (shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME]) {
      acqtime = shm_cfg_ptr[CFG_FIL_ACQ_STOP_TIME] -
                shm_cfg_ptr[CFG_FIL_ACQ_START_TIME];
    } else {
      acqtime = currentTime - shm_cfg_ptr[CFG_FIL_ACQ_START_TIME];
    }
  } else {
    acqtime = 0;
  }

  maWriteFmt(rq, "Acq-Time: %d\n", acqtime);
}

  /*******************************************************************************
   *
   * FUNCTION
   *   rawDataGUI
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

#define PRINT_BUFFERLEN 1024

static void rawDataGUI(MaRequest *rq, char *script, char *uri, char *query,
                       char *postData, int postLen) {
  char buffer[PRINT_BUFFERLEN];
  uint i, dispsize;
  volatile histo_descr_type *histo_descr_ptr;
  volatile uint32_t *RawDataBuffer;
  uint32_t val;
  uint32_t packets = 0;
  uint32_t readpackets = 0;
  int store, dodisp;
  char *var;
  packet_type packet;

  requestLog(rq, uri);

  histo_descr_ptr = getShmHistoPtr();

  getControlVar(CFG_SRV_DO_STORE_RAW_DATA, &store);

  insertHeader(rq, "RT Raw Data Buffer", 0);
  insertStatusBar(rq, "rawdata");

  gethostname(buffer, 255);
  maWriteStr(rq, "<PRE>");

  maWriteFmt(rq, "<h1>Raw Data Buffer on %s</h1>\n\n", buffer);

  maWriteFmt(rq, "<table border=\"1\">");
  maWriteFmt(rq, "<tr><td>Store Raw Data</td><td>%s</td>",
             (store ? "Yes" : "No"));
  maWriteFmt(rq, "<tr><td>Available Data SHM</td><td>%10d Bytes</td>",
             dataShmAvail());
  maWriteFmt(rq, "<tr><td>Raw Data Buffer Size</td><td>%10d Bytes</td>",
             histo_descr_ptr->rawdata_size);
  maWriteFmt(rq, "<tr><td>Raw Data Stored</td><td>%10d Words a 32 bit</td>",
             histo_descr_ptr->rawdata_stored);
  maWriteFmt(rq, "<tr><td>Raw Data Missed</td><td>%10d Words a 32 bit</td>",
             histo_descr_ptr->rawdata_missed);
  maWriteFmt(rq, "</table>");

  maWriteStr(rq, "<BR><BR><form action=\"/admin/rawapply.egi\" method=\"post\" "
                 "enctype=\"multipart/form-data\">");
  maWriteFmt(rq,
             "<INPUT TYPE=CHECKBOX NAME=\"storeraw\" value=\"on\" %s>Store Raw "
             "Data in Buffer<BR>",
             (store ? "checked" : "\0"));
  maWriteStr(rq, "<BR><input type=\"submit\" value=\"Apply\"> (Data stored "
                 "after a new Start of Acquisition) ");
  maWriteStr(rq, "</form>");

  maWriteStr(rq,
             "<BR><BR><form action=\"/admin/readrawdata.egi\" METHOD=\"GET\">");
  maWriteStr(rq, "Download Sizelimit: <input type=\"text\"   "
                 "name=\"sizelimit\" size=\"10\" value=\"-1\"> KByte ( value "
                 "<= 0 : No Limit )<BR>");
  maWriteStr(rq, "<BR><input type=\"submit\" value=\"Download\"> ");
  maWriteStr(rq, "</form>");

  var = maGetVar(rq, MA_FORM_OBJ, "setdispvals", NULL);
  if (var) {
    var = maGetVar(rq, MA_FORM_OBJ, "displimit", NULL);
    if (var) {
      displimit = atoi(var);
    }

    var = maGetVar(rq, MA_FORM_OBJ, "dispmask", NULL);
    if (var) {
      dispmask = 0;
      sscanf(var, "%x", &dispmask);
    }

    var = maGetVar(rq, MA_FORM_OBJ, "dispnotexp", "off");
    dispnotexp = (mprStrCmpAnyCase(var, "on") == 0);

    var = maGetVar(rq, MA_FORM_OBJ, "dispexpect", NULL);
    if (var) {
      dispexpect = 0;
      sscanf(var, "%x", &dispexpect);
    }

    var = maGetVar(rq, MA_FORM_OBJ, "packetinfo", "off");
    packetinfo = (mprStrCmpAnyCase(var, "on") == 0);
  }

  maWriteStr(rq,
             "<BR><BR><form action=\"/admin/rawdata.egi\" METHOD=\"POST\">");
  maWriteFmt(rq,
             "Display Sizelimit: <input type=\"text\"   name=\"displimit\" "
             "size=\"10\" value=\"%d\"> Packets ( value <= 0 : No Limit)<BR>",
             displimit);
  maWriteFmt(rq,
             "<BR>Display Filter:  Mask <input type=\"text\"   "
             "name=\"dispmask\" size=\"10\" value=\"0x%08x\">   [<INPUT "
             "TYPE=CHECKBOX NAME=\"dispnotexp\" value=\"on\" %s>not]  Expected "
             "<input type=\"text\"   name=\"dispexpect\" size=\"10\" "
             "value=\"0x%08x\"><BR>",
             dispmask, (dispnotexp ? "checked" : "\0"), dispexpect);
  maWriteFmt(rq,
             "<BR><INPUT TYPE=CHECKBOX NAME=\"packetinfo\" value=\"on\" %s> "
             "Show packet Info<BR>",
             (packetinfo ? "checked" : "\0"));
  maWriteStr(rq, "<INPUT TYPE=HIDDEN NAME=\"setdispvals\" value=\"on\" >");
  maWriteStr(rq, "<BR><input type=\"submit\" value=\"Display\"> ");
  maWriteStr(rq, "</form>");

  if (histo_descr_ptr->rawdata_offs) {
    RawDataBuffer = dataShmOffsToPtr(histo_descr_ptr->rawdata_offs);
    dispsize = histo_descr_ptr->rawdata_stored;

    if (displimit <= 0)
      displimit = 0x7fffffff;

    packets = 0;
    packet.header_found = 0;
    readpackets = 0;

    for (i = 0; ((i < dispsize) && (packets <= displimit)); i++) {
      val = RawDataBuffer[i];
      if (VAL_IS_HEADER(val)) {
        if (packetinfo && packet.header_found) {
          maWriteStr(rq, "\n");
          snprint_packet_info(buffer, PRINT_BUFFERLEN, "          ", &packet);
          maWriteStr(rq, buffer);
        }

        if (dispnotexp) {
          dodisp = ((val & dispmask) != dispexpect);
        } else {
          dodisp = ((val & dispmask) == dispexpect);
        }

        if (dodisp) {
          if (packets < displimit) {
            //            maWriteFmt(rq,"\n%8d: 0x%08x",packets,val);
            maWriteFmt(rq, "\n%8d: 0x%08x", readpackets, val);
          }
          packet.data[0] = val;
          packet.ptr = 1;
          packet.header_found = 1;
          packet.length = packet_len(val);
          packets++;
        } else {
          packet.header_found = 0;
        }
        readpackets++;
      } else {
        if (packet.header_found) {
          maWriteFmt(rq, " 0x%04x", val);
          if (packet.ptr < MAX_PACKET_LENGTH) {
            packet.data[(packet.ptr)++] = val;
          }
        }
      }
    }
  }

  maWriteStr(rq, "</PRE>");

  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   rawApplyStore
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

static void rawApplyStore(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen) {
  char *var;
  int state;

  requestLog(rq, uri);

  var = maGetVar(rq, MA_FORM_OBJ, "storeraw", "off");

  if (mprStrCmpAnyCase(var, "on") == 0) {
    state = 1;
  } else {
    state = 0;
  }

  setControlVar(CFG_SRV_DO_STORE_RAW_DATA, state);

  var = maGetVar(rq, MA_FORM_OBJ, "cmd", "0");

  if (mprStrCmpAnyCase(var, "1") == 0) {
    if (state) {
      httpText(rq, 200, "rawdata = on");
    } else {
      httpText(rq, 200, "rawdata = off");
    }
  } else {
    rawDataGUI(rq, script, uri, query, postData, postLen);
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   readRawDataEgi
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

static void readRawDataEgi(MaRequest *rq, char *script, char *uri, char *query,
                           char *postData, int postLen) {
  char *var, buffer[256];
  int size, sizelimit;

  volatile histo_descr_type *histo_descr_ptr;
  volatile uint32_t *RawDataBuffer;

  requestLog(rq, uri);

  histo_descr_ptr = getShmHistoPtr();

  RawDataBuffer = dataShmOffsToPtr(histo_descr_ptr->rawdata_offs);
  size = histo_descr_ptr->rawdata_stored * sizeof(int);

  if (!histo_descr_ptr->rawdata_offs) {
    maSetResponseCode(rq, 200);
    maSetHeader(rq, "Content-type: text/plain", 0);
    maWriteFmt(rq, "ERROR: No Raw Data to download\n\n");
    return;
  }

  var = maGetVar(rq, MA_FORM_OBJ, "sizelimit", NULL);
  if (var == NULL) {
    sizelimit = -1;
  } else {
    sizelimit = atoi(var);
    sizelimit *= 1024; // value given in KByte
  }

  if (sizelimit > 0) {
    if (size > sizelimit) {
      size = sizelimit;
    }
  }

  maSetResponseCode(rq, 200);
  maSetHeader(rq, "Content-type: application/x-sinqhm", 0);
  snprintf(buffer, 255, "Content-length: %d", size);
  maSetHeader(rq, buffer, 0);
  maWrite(rq, (char *)RawDataBuffer, size);

#ifdef TARGET_HAS_LITTLE_ENDIAN
// use getTransferData(int bank, int start, int end, int **data)
#endif
}

/*******************************************************************************
 *
 * FUNCTION
 *   showDebugBuffer
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

static void showDebugBuffer(MaRequest *rq, char *script, char *uri, char *query,
                            char *postData, int postLen) {
  char buffer[256];
  char *var;
  int dbgmsglevel;
  volatile unsigned int *buff;

  requestLog(rq, uri);

  buff = getDebugBuffPtr();

  var = maGetVar(rq, MA_FORM_OBJ, "errorbuffack", NULL);
  if (var) {
    buff[DBG_SRV_ERR_BUFF_ACK] = buff[DBG_FIL_ERR_BUFF_SGN];
  }

  insertHeader(rq, "RT Debug Buffer", 0);
  insertStatusBar(rq, "debugshm");

  maWriteStr(rq, "<pre><span class=\"DBGMSG_ERROR\">");

  gethostname(buffer, 255);
  maWriteFmt(rq, "<h1>Error String Buffer on %s</h1>\n<p>", buffer);

  if (buff[DBG_SRV_ERR_BUFF_ACK] != buff[DBG_FIL_ERR_BUFF_SGN]) {
    maWriteStr(rq, (char *)&buff[DBG_FIL_ERR_BUFF_START]);
  }
  maWriteStr(rq, "</span>");

  maWriteStr(rq, "<form action=\"/admin/debugbuffer.egi\" method=\"post\">");
  maWriteStr(rq, "<BR><input type=\"submit\" value=\"Acknowledge\"> ");
  maWriteStr(rq, "<INPUT TYPE=HIDDEN NAME=\"errorbuffack\" value=\"on\" >");
  maWriteStr(rq, "</form>");
  maWriteStr(rq, "</pre>");

  maWriteStr(rq, "\n");
  maWriteStr(rq, "<PRE>");

  maWriteStr(rq, "\n\n");

  var = maGetVar(rq, MA_FORM_OBJ, "setdispvals", NULL);
  if (var) {
    var = maGetVar(rq, MA_FORM_OBJ, "dbgmsglevel", NULL);
    if (var) {
      dbgmsglevel = atoi(var);
      if (dbgmsglevel < 0)
        dbgmsglevel = 0;
      if (dbgmsglevel > 9)
        dbgmsglevel = 9;
      buff[DBG_SRV_MSG_LEVEL] = dbgmsglevel;
    }
    var = maGetVar(rq, MA_FORM_OBJ, "wraparound", "off");
    buff[DBG_SRV_CHAR_BUFF_ONCE] = (mprStrCmpAnyCase(var, "on") != 0);
  }

  maWriteStr(rq, "<form action=\"/admin/debugbuffer.egi\" method=\"post\">");
  maWriteFmt(rq,
             "<BR>Message Level: <input type=\"text\"   name=\"dbgmsglevel\" "
             "size=\"4\" value=\"%d\"> ( 0: None 1: Errors 2: Warnings 3...9: "
             "Infos )<BR>",
             buff[DBG_SRV_MSG_LEVEL]);
  maWriteFmt(rq,
             "<BR><INPUT TYPE=CHECKBOX NAME=\"wraparound\" value=\"on\" %s> "
             "wrap around after buffer is full<BR>",
             (buff[DBG_SRV_CHAR_BUFF_ONCE] ? "" : "checked"));
  maWriteStr(rq, "<BR><input type=\"submit\" value=\"Apply\"> ");
  maWriteStr(rq, "<INPUT TYPE=HIDDEN NAME=\"setdispvals\" value=\"on\" >");
  maWriteStr(rq, "</form>");

  maWriteStr(rq, "<form action=\"/admin/resetbuffer.egi\" method=\"get\">");
  maWriteStr(rq, "<BR><input type=\"submit\" value=\"Reset Buffer\">");
  maWriteStr(rq, "</form>\n");

  maWriteFmt(rq, "DBG Message Level: %d\n", buff[DBG_SRV_MSG_LEVEL]);
  maWriteFmt(rq, "Wrap around: %s\n",
             (buff[DBG_SRV_CHAR_BUFF_ONCE] ? "OFF" : "ON"));
  maWriteFmt(rq, "Number of Wrap Arounds: %d\n", buff[DBG_FIL_WRAP_AROUND]);
  maWriteFmt(rq, "buff[DBG_FIL_BUFF_SIZE]:   %d\n", buff[DBG_FIL_BUFF_SIZE]);
  maWriteFmt(rq, "buff[DBG_FIL_STR_PTR]:     %d\n", buff[DBG_FIL_STR_PTR]);
  maWriteFmt(rq, "free bytes:     %d\n",
             buff[DBG_FIL_BUFF_SIZE] - 1 - buff[DBG_FIL_STR_PTR]);

  gethostname(buffer, 255);
  maWriteFmt(rq, "<h1>Debug String Buffer  on %s</h1>\n<p>", buffer);
  maWriteStr(rq, (char *)&buff[DBG_FIL_CHAR_BUFF_START]);
  maWriteStr(rq, "\n\n");

  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   resetDebugBuffer
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

static void resetDebugBuffer(MaRequest *rq, char *script, char *uri,
                             char *query, char *postData, int postLen) {
  int status;
  char buffer[256];

  requestLog(rq, uri);

  status = resetBuff();
  if (status < 0) {
    dataErrorToText(status, buffer, 255);
    httpText(rq, 500, buffer);
  } else {
    maRedirect(rq, 301, "/admin/debugbuffer.egi");
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   showConfigShm
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

static void showConfigShm(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen) {
  char buffer[256];

  volatile unsigned int *buff;

  int i, entries;

  requestLog(rq, uri);

  entries = sizeof(cfg_descr) / sizeof(shm_descr_type);

  insertHeader(rq, "RT Show Config", 0);
  insertStatusBar(rq, "configshm");

  buff = getVarPointer(CFG_BASE_PTR);

  gethostname(buffer, 255);
  maWriteStr(rq, "<PRE>");

  maWriteStr(rq, "<form action=\"/admin/configshm.egi\" method=\"get\">");
  maWriteStr(rq, "<input type=\"submit\" value=\"Update\">");
  maWriteStr(rq, "</form>\n");

  maWriteFmt(rq, "<h1>Config Section of %s</h1>\n<p>", buffer);
  //  maWriteStr(rq,"<table>\n");

  maWriteStr(rq, "<table border=\"1\"><tbody>");

  maWriteFmt(rq, "<tr><td><b>ID</b></td><td><b>Name</b></td><td><b>dec</b></"
                 "td><td><b>hex</b></td><td><b>Comment</b></td></tr>\n");

  for (i = 0; i < entries; i++) {
    maWriteFmt(rq,
               "<tr><td>%03d</td><td>%s</td><td>%d</td><td>0x%08x</td><td>%s</"
               "td></tr>\n",
               cfg_descr[i].id, cfg_descr[i].name, buff[cfg_descr[i].id],
               buff[cfg_descr[i].id], cfg_descr[i].comment);
  }

  maWriteStr(rq, "</tbody></table>");

  maWriteFmt(rq, "%8d  %8d  %8d  %8d  %8d  %8d  %8d  %8d\n\n", buff[0], buff[1],
             buff[2], buff[3], buff[4], buff[5], buff[6], buff[7]);
  for (i = 0; ((unsigned)i) < SHM_CFG_SIZE / (8 * sizeof(unsigned int));
       i += 8) {
    maWriteFmt(rq, "0x%04x:  %08x  %08x  %08x  %08x  %08x  %08x  %08x  %08x\n",
               i, buff[i], buff[i + 1], buff[i + 2], buff[i + 3], buff[i + 4],
               buff[i + 5], buff[i + 6], buff[i + 7]);
  }

  //  maWriteStr(rq,"</table>");
  maWriteStr(rq, "<form action=\"/admin/configshm.egi\" method=\"get\">");
  maWriteStr(rq, "<input type=\"submit\" value=\"Update\">");
  maWriteStr(rq, "</form>\n");

  maWriteStr(rq, "</PRE>");

  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   showDataShm
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

static void showDataShm(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen) {
  char buffer[256];
  volatile histo_descr_type *histo_descr_ptr;
  volatile unsigned int *buff;
  int i, entries, dispsize;

  requestLog(rq, uri);

  histo_descr_ptr = getShmHistoPtr();
  buff = (volatile unsigned int *)histo_descr_ptr;

  entries = sizeof(data_descr) / sizeof(shm_descr_type);

  insertHeader(rq, "RT Data SHM", 0);
  insertStatusBar(rq, "datashm");

  gethostname(buffer, 255);
  maWriteStr(rq, "<PRE>");

  maWriteStr(rq, "<form action=\"/admin/datashm.egi\" method=\"get\">");
  maWriteStr(rq, "<input type=\"submit\" value=\"Update\">");
  maWriteStr(rq, "</form>\n");

  maWriteFmt(rq, "<h1>HM Data Section of %s</h1>\n<p>", buffer);
  //  maWriteStr(rq,"<table>\n");

  maWriteStr(rq, "<table border=\"1\"><tbody>");

  maWriteFmt(rq, "<tr><td><b>ID</b></td><td><b>Name</b></td><td><b>dec</b></"
                 "td><td><b>hex</b></td><td><b>Comment</b></td></tr>\n");

#if 1
  for (i = 0; i < entries; i++) {
    maWriteFmt(rq,
               "<tr><td>%03d</td><td>%s</td><td>%d</td><td>0x%08x</td><td>%s</"
               "td></tr>\n",
               data_descr[i].id, data_descr[i].name, buff[data_descr[i].id],
               buff[data_descr[i].id], data_descr[i].comment);
  }
#endif

  maWriteStr(rq, "</tbody></table>");

  maWriteFmt(rq, "%8d  %8d  %8d  %8d  %8d  %8d  %8d  %8d\n\n", buff[0], buff[1],
             buff[2], buff[3], buff[4], buff[5], buff[6], buff[7]);

  dispsize = (histo_descr_ptr->cfg_mem_used / sizeof(uint32_t));

  if (dispsize > 10000)
    dispsize = 10000;

  for (i = 0; i < dispsize; i += 8) {
    maWriteFmt(rq, "0x%04x:  %08x  %08x  %08x  %08x  %08x  %08x  %08x  %08x\n",
               i * 4, buff[i], buff[i + 1], buff[i + 2], buff[i + 3],
               buff[i + 4], buff[i + 5], buff[i + 6], buff[i + 7]);
  }

  //  maWriteStr(rq,"</table>");
  maWriteStr(rq, "<form action=\"/admin/datashm.egi\" method=\"get\">");
  maWriteStr(rq, "<input type=\"submit\" value=\"Update\">");
  maWriteStr(rq, "</form>\n");

  maWriteStr(rq, "</PRE>");

  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   startDAQEgi
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

static void startDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen) {

  int status;
  char buffer[256];
  volatile histo_descr_type *histo_descr_ptr;

  // check if filler is configured ...
  requestLog(rq, uri);

  histo_descr_ptr = getShmHistoPtr();
  if (!histo_descr_ptr ||
      histo_descr_ptr->filler_valid != DATASHM_CFG_FIL_VALID) {
    httpText(rq, 500, "Histogram Memory not configured");
    return;
  }

  status = startDAQ();
  if (status < 0) {
    dataErrorToText(status, buffer, 255);
    httpText(rq, 500, buffer);
  } else {
    httpText(rq, 200, "DAQ Started");
  }
}
/*******************************************************************************
 *
 * FUNCTION
 *   continueDAQEgi
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

static void continueDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                           char *postData, int postLen) {

  int status, val;
  char buffer[256];

  requestLog(rq, uri);

  getControlVar(CFG_SRV_DO_CFG_CMD, &val);
  if (val != 1) {
    httpText(rq, 500, "Histogram Memory not configured");
    return;
  }
  status = startDAQ();
  if (status < 0) {
    dataErrorToText(status, buffer, 255);
    httpText(rq, 500, buffer);
  } else {
    httpText(rq, 200, "DAQ Started");
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   stopDAQEgi
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

static void stopDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                       char *postData, int postLen) {

  int status, val;
  char buffer[256];
  getControlVar(CFG_SRV_DO_CFG_CMD, &val);

  requestLog(rq, uri);

  if (val != 1) {
    httpText(rq, 500, "Histogram Memory not configured");
    return;
  }
  status = stopDAQ();
  if (status < 0) {
    dataErrorToText(status, buffer, 255);
    httpText(rq, 500, buffer);
  } else {
    httpText(rq, 200, "DAQ Stopped");
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   pauseDAQEgi
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

static void pauseDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen) {

  int status, val;
  char buffer[256];

  requestLog(rq, uri);

  getControlVar(CFG_SRV_DO_CFG_CMD, &val);
  if (val != 1) {
    httpText(rq, 500, "Histogram Memory not configured");
    return;
  }
  status = pauseDAQ();
  if (status < 0) {
    dataErrorToText(status, buffer, 255);
    httpText(rq, 500, buffer);
  } else {
    httpText(rq, 200, "DAQ Paused");
  }
}

/*******************************************************************************
 *
 * FUNCTION
 *   GUIstartDAQEgi
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

static void GUIstartDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                           char *postData, int postLen) {

  int status;
  volatile histo_descr_type *histo_descr_ptr;

  requestLog(rq, uri);

  // check if filler is configured ...

  histo_descr_ptr = getShmHistoPtr();
  if (!histo_descr_ptr ||
      histo_descr_ptr->filler_valid != DATASHM_CFG_FIL_VALID) {
    //    maWriteStr(rq,"Histogram Memory not configured");
    //    return;
  } else {
    status = startDAQ();
    if (status < 0) {
      //      dataErrorToText(status,buffer,255);
      //      maWriteStr(rq,buffer);
    }
  }

  forwardReq(query, rq, script, uri, query, postData, postLen);
  //  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   GUIstopDAQEgi
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

static void GUIpauseDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                           char *postData, int postLen) {

  int status, val;

  requestLog(rq, uri);

  getControlVar(CFG_SRV_DO_CFG_CMD, &val);
  if (val != 1) {
    //    maWriteStr(rq,"Histogram Memory not configured");
  } else {
    status = pauseDAQ();
    if (status < 0) {
      //      dataErrorToText(status,buffer,255);
      //      maWriteStr(rq,buffer);
    }
  }

  forwardReq(query, rq, script, uri, query, postData, postLen);
}

/*******************************************************************************
 *
 * FUNCTION
 *   GUIstopDAQEgi
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

static void GUIstopDAQEgi(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen) {

  int status, val;

  requestLog(rq, uri);

  getControlVar(CFG_SRV_DO_CFG_CMD, &val);
  if (val != 1) {
    //    maWriteStr(rq,"Histogram Memory not configured");
  } else {
    status = stopDAQ();
    if (status < 0) {
      //      dataErrorToText(status,buffer,255);
      //      maWriteStr(rq,buffer);
    }
  }
  forwardReq(query, rq, script, uri, query, postData, postLen);
}

#if 0
/*******************************************************************************
 *
 * FUNCTION
 *   indexEgi
 *
 ******************************************************************************/


static void indexEgi(MaRequest *rq, char *script, char *uri, char *query,
                    char *postData, int postLen)
{
  requestLog(rq, uri);

//	maRedirect(rq, 301, "/public/menu.egi");
	maRedirect(rq, 301, "/public/status.egi");
}
#endif

/*******************************************************************************
 *
 * FUNCTION
 *   menuEgi
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

static void menuEgi(MaRequest *rq, char *script, char *uri, char *query,
                    char *postData, int postLen) {
  requestLog(rq, uri);

  insertHeader(rq, "Menu", 0);
  insertStatusBar(rq, "menu");
  insertFile(rq, "/public/menu.html");
  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   showConfigEgi
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

static void showConfigEgi(MaRequest *rq, char *script, char *uri, char *query,
                          char *postData, int postLen) {
  char buffer[256];
  int i, nbank, filler;
  int status = 0;
  volatile histo_descr_type *histo_descr_ptr;

  requestLog(rq, uri);

  /*
   * check that DAQ is not running
   */
  getControlVar(CFG_FIL_DO_DAQ_ACK, &status);

  insertHeader(rq, "Configuration", 0);
  insertStatusBar(rq, "configuration");

  if (status > 0) {
    maWriteStr(rq, "<h1>Change Current Configuration</h1>");
    maWriteStr(rq, "<BR>Cannot configure when DAQ is active<BR><BR>");
  } else {
    insertFile(rq, "/admin/configuration.html");
  }

  gethostname(buffer, 255);
  maWriteFmt(rq, "<h1>Current Histogram Memory Configuration on %s</h1>\n<p>",
             buffer);

  histo_descr_ptr = getShmHistoPtr();

  if (!histo_descr_ptr) {
    maWriteFmt(rq, "ERROR: can not get histo_descr_ptr\n");
    return;
  }

  if ((histo_descr_ptr->server_valid == DATASHM_CFG_SRV_VALID) &&
      (histo_descr_ptr->filler_valid == DATASHM_CFG_FIL_VALID)) {
    filler = histo_descr_ptr->histo_type;

    switch (filler) {
    case FILLERTOF:
      strcpy(buffer, "Time-Of-Flight");
      break;
    case FILLERPSD:
      strcpy(buffer, "EMBL-PSD-Filler");
      break;
    case FILLERDIG:
      strcpy(buffer, "Standard Filler");
      break;
    case FILLERTOFMAP:
      strcpy(buffer, "2D TOF with Lookup Table");
      break;
    case FILLERHRPT:
      strcpy(buffer, "Cerca HRPT Frame Mode");
      break;
    case FILLERSANS2:
      strcpy(buffer, "Sans2-PSD Filler");
      break;
    case FILLERRITA2:
    case FILLERCAMEA:
      strcpy(buffer, "0MQ Filler");
      break;

    default:
      snprintf(buffer, 255, "Unknown filler ID %d", filler);
      break;
    }

    maWriteFmt(rq,
               "<table><tr><th>Filler Type</th><td>%s</td></tr></table></p>\n",
               buffer);
    maWriteStr(rq, "<hr>\r\n");

    nbank = histo_descr_ptr->nBank;

    for (i = 0; i < nbank; i++) {
      formatBankInfo(rq, i);
    }
  } else {
    maWriteStr(rq, "<br><br> Histo Memory is not configured<br><br>\n");
  }

  insertFooter(rq);
}

/*******************************************************************************
 *
 * FUNCTION
 *   readDataEgi
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

static void readDataEgi(MaRequest *rq, char *script, char *uri, char *query,
                        char *postData, int postLen) {

  requestLog(rq, uri);

  insertHeader(rq, "Read Data", 0);
  insertStatusBar(rq, "readdata");
  insertFile(rq, "/admin/readdata.html");
  insertFooter(rq);
}

/*******************************************************************************/

int fileExists(char *fileName) {
  if (access(fileName, R_OK + W_OK) == 0) {
    return 1;
  }
  return 0;
}
/*******************************************************************************
 *
 * FUNCTION
 *   checkRTModule
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

int checkRTModule(void) {
  char buffer[256];
  int found = 0;
  FILE *proc_modules_file;

#ifdef FILLER_USER
  found = system("ps -A | grep sinqhm_filler");
  /* printf("found=%d\n",found); */
  if (found == 0) {
    return 1;
  } else {
    return -1;
  }

#endif

  if (!fileExists("/dev/rtai_shm")) {
    printf("ERROR: RTAI shared Memory device /dev/rtai_shm does not exist\n");
    return -1;
  }

  if ((proc_modules_file = fopen("/proc/modules", "r")) == NULL) {
    printf("ERROR: cannot open /proc/modules\n");
    return -1;
  }

  while (!feof(proc_modules_file) && !found) {
    fgets(buffer, 256, proc_modules_file);
    if (ferror(proc_modules_file)) {
      printf("ERROR: Read error of /proc/modules\n");
      break;
    }
    found = (strncmp(buffer, "sinqhm_filler ", 14) == 0);
  }
  fclose(proc_modules_file);
  return found;
}

/*******************************************************************************
 *
 * FUNCTION
 *   initializeSINQHM
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

int initializeSINQHM(void) {
  int status;
  char *config = NULL;
  /* uint32_t cfg_valid, clevel; */
  int cfg_valid, clevel;
  char buffer[256];
  volatile histo_descr_type *histo_descr_ptr;
  time_t serverStartupTime;

  config = getenv("SINQHMCONFIG");
  if (config != NULL) {
    sinqhmConfig = strdup(config);
  } else {
    sinqhmConfig = strdup("sinqhm.xml");
  }

  if (checkRTModule() <= 0) {
    printf("ERROR: SINQHM Realtime Module not installed\n");
    return -1;
  }

  status = initShmControl();
  if (status < 0)
    return status;

  getControlVar(CFG_FIL_CFG_VALID, &cfg_valid);
  if (cfg_valid != CFG_VALID) {
    perror("ERROR: CFG SHM is not valid\n");
    return -1;
  }

  getControlVar(CFG_FIL_CLEVEL, &clevel);
  if (clevel != COMPATIBILITY_LEVEL) {
    perror("ERROR: This Server Version is not compatible with installed real "
           "time module\n");
    return -1;
  }

  getControlVar(CFG_FIL_HST_SHM_SIZE, &hst_size);
  getControlVar(CFG_FIL_DBG_SHM_SIZE, &dbg_size);

  /*
   *  Store startup time
   */
  time(&serverStartupTime);
  setControlVar(CFG_SRV_SERVER_START_TIME, serverStartupTime);

  status = initShmDebug();
  if (status < 0)
    return status;

  status = initShmHisto();
  //  assert(status == SINQHM_OK);
  if (status < 0)
    return status;

  // check if filler is configured ...

  histo_descr_ptr = getShmHistoPtr();
  if ((histo_descr_ptr->server_valid == DATASHM_CFG_SRV_VALID) &&
      (histo_descr_ptr->filler_valid == DATASHM_CFG_FIL_VALID)) {
    printf("Using Current Configuration\n");
  } else {
    stopDAQ();

    /**
     * load configuration
     */

    printf("Configuring Histogram from file: %s\n", sinqhmConfig);
    status = configureHistogramMemoryFromFile(sinqhmConfig, 0);
    if (status < 0) {
      dataErrorToText(status, buffer, 255);
      mprTrace(1, "%s\n", buffer);
      //      exit(1);
    }
  }

  /*
   * initialize our callbacks
   */

  //  maDefineEgiForm("/index.egi",                indexEgi);

  maDefineEgiForm("/admin/showconfig.egi", showConfigEgi);
  maDefineEgiForm("/admin/configure.egi", configureEgi);
  maDefineEgiForm("/admin/configuregui.egi", configureEgiGUI);
  maDefineEgiForm("/admin/maketestdata.egi", writeTestDataEgi);
  maDefineEgiForm("/admin/presethm.egi", presetHMEgi);
  maDefineEgiForm("/admin/readhmdata.egi", readHMDataEgi);
  maDefineEgiForm("/admin/processhmdata.egi", processHMDataEgi);
  maDefineEgiForm("/admin/readrawdata.egi", readRawDataEgi);
  maDefineEgiForm("/admin/rawdata.egi", rawDataGUI);
  maDefineEgiForm("/admin/rawapply.egi", rawApplyStore);
  maDefineEgiForm("/admin/textstatus.egi", textStatusEgi);
  maDefineEgiForm("/admin/daqstatus.egi", daqStatusEgi);
  maDefineEgiForm("/admin/resetbuffer.egi", resetDebugBuffer);
  maDefineEgiForm("/admin/startdaq.egi", startDAQEgi);
  maDefineEgiForm("/admin/stopdaq.egi", stopDAQEgi);
  maDefineEgiForm("/admin/pausedaq.egi", pauseDAQEgi);
  maDefineEgiForm("/admin/continuedaq.egi", continueDAQEgi);
  maDefineEgiForm("/admin/guistartdaq.egi", GUIstartDAQEgi);
  maDefineEgiForm("/admin/guistopdaq.egi", GUIstopDAQEgi);
  maDefineEgiForm("/admin/guipausedaq.egi", GUIpauseDAQEgi);
  maDefineEgiForm("/admin/readdata.egi", readDataEgi);

  maDefineEgiForm("/admin/debugbuffer.egi", showDebugBuffer);
  maDefineEgiForm("/admin/configshm.egi", showConfigShm);
  maDefineEgiForm("/admin/datashm.egi", showDataShm);

  maDefineEgiForm("/public/menu.egi", menuEgi);
  maDefineEgiForm("/public/status.egi", statusPublicEgi);

  return 1;
}

/*******************************************************************************
 *
 * FUNCTION
 *   closeSINQHM
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

int closeSINQHM(void) {
  releaseShmDebug();
  releaseShmControl();
  releaseShmHisto();
  return 1;
}

/******************************************************************************/
