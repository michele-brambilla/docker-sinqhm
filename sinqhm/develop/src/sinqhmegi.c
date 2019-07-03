/*******************************************************************************
  sinqhmegi.c
*******************************************************************************/

/**
 * This is a SINQHM server for Linux-RTAI based on the EGI interface of the
 * appweb www server. EGI is a means for embedding your very own C-code into
 * the appweb WWW server.
 */


/*******************************************************************************
  includes
*******************************************************************************/

#include  <appWeb.h>
#include "egiadapter.h"


/*******************************************************************************
  constant and macro definitions
*******************************************************************************/

#define UNSAFE_FUNCTIONS_OK 1

/*******************************************************************************
  global vars
*******************************************************************************/

int hst_size = 0;
int dbg_size = 0;

/*******************************************************************************
  function declarations
*******************************************************************************/

/*******************************************************************************
 *
 * FUNCTION
 *   sinqhmegiSignal
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

static void sinqhmegiSignal(int signal)
{
  printf("calling closeSINQHM()\n");
  closeSINQHM();
  closeLog();
  exit(0);
}


/*******************************************************************************
 *
 * FUNCTION
 *   main
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
MaServer  *mainServer;  /* For a HTTP server */

int main(int argc, char** argv)
{
  MaHttp    *http;    /* For the http service inside our app */

  openLog("/tmp/appweb_access.log", 1024 * 1024);

  /*
   *  Initialize the run-time and give our app a name "sinqhmegi"
   */
  mprCreateMpr("sinqhmegi");

//#if BLD_FEATURE_LOG

  /*
   *  Do the following two statements only if you want debug trace
   */
//  mprAddLogFileListener();
//  mprSetLogSpec("/tmp/appweb.log:2.1");
//#endif
  mprAddLogFileListener();
//  mprSetLogSpec("stdout:6");
  /*
   *  Start run-time services
   */
  mprStartMpr(0);

  /*
   *  Create the HTTP and server objects. Give the server a name 
   *  "default" and define "." as the default serverRoot, ie. the 
   *  directory with the server configuration files.
   */
  http = maCreateHttp();
  mainServer = maCreateServer(http, "default", ".");
  
  /*
   *  Activate the handlers. Only needed when linking statically.
   */
  mprAuthInit(0);
#ifndef APPWEB_VER_2_0_4
  mprDirInit(NULL);
#endif
  mprUploadInit(0);
  mprEgiInit(0);
  mprCopyInit(0);


  /*
   *  Configure the server based on the directives in 
   *  simpleEgi.conf.
   */
  if (maConfigureServer(mainServer, "sinqhmegi.conf") < 0) 
  {
    fprintf(stderr, 
      "Can't configure the server. Error on line %d\n", 
      maGetConfigErrorLine(mainServer));
    exit(2);
  }

  /**
   * initialize the SINQHM system
   */
  if(initializeSINQHM() < 0) 
  {
//    perror("sinqhmegi");
//    TBD:
    fprintf(stderr, "ERROR during initializeSINQHM()\n");
    closeSINQHM();
    exit(1);
  }

  signal(SIGTERM,sinqhmegiSignal);
  signal(SIGINT,sinqhmegiSignal);

  /*
   *  Start serving pages. After this we are live.
   */
  if (maStartServers(http) < 0) 
  {
    fprintf(stderr, "Can't start the server\n");
    exit(2);
  }

//  \author Michele Brambilla <michele.brambilla@psi.ch>
//  \date Tue Apr 11 15:21:50 2017
  printf("=================================\n"      );
  printf("= ID: %d \t\t\t=\n",       getShmHistoPtr()->id          );
  printf("= Version: %d \t\t\t=\n",  getShmHistoPtr()->version     );
  printf("= Server valid: %x \t=\n", getShmHistoPtr()->server_valid);
  printf("= Filler valid: %x\t\t=\n",getShmHistoPtr()->filler_valid);
  printf("=================================\n"      );

  /*
   *  Service events. This call will block until the server is exited
   *  Call mprTerminate() at any time to instruct the server to exit.
   *  The -1 is a timeout on the block. Useful if you use 
   *  MPR_LOOP_ONCE and have a polling event loop.
   */
  mprServiceEvents(MPR_LOOP_FOREVER, -1);

  /*
   *  Stop all HTTP services
   */
  maStopServers(http);

  /*
   *  Delete the server and http objects
   */
  maDeleteServer(mainServer);
  maDeleteHttp(http);

  /*
   *  Stop and delete the run-time services
   */
  mprStopMpr();
  mprDeleteMpr();

  /**
   * clean up SINQHM
   */
  closeSINQHM();
  closeLog();
   
  return 0;
}
