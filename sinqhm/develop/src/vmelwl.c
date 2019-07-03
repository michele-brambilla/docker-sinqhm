
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "vme_testgen.h"



int lwl_fifo = -1;

char lwl_fifo_pipe_name[256] = "/tmp/lwlfifo";

#define LWL_SIM_BUFFER_SIZE 1024

char lwl_sim_buffer[LWL_SIM_BUFFER_SIZE+1];
int lwl_sim_ppos = -1;
int lwl_sim_spos = -1;
int lwl_sim_bytes_read = 0;
int lwl_sim_comment = 0;
int lwl_sim_status  = 0;

int use_port = 0;
int sockfd =-1 ;


/******************************************************************************/

void error(char *msg)
{
    perror(msg);
    exit(1);
}

/******************************************************************************/

void sock_init(int portno)
{
     struct sockaddr_in serv_addr;
     int rv;

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) error("ERROR opening socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));

     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     rv = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
     if ( rv < 0) error("ERROR on binding");

     listen(sockfd,5);
}

/******************************************************************************/

int sock_accept(void)
{
  struct sockaddr_in cli_addr;
  int client_len, fd;

  client_len = sizeof(cli_addr);
  fd = accept(sockfd, (struct sockaddr *) &cli_addr, &client_len);
  if (fd < 0)  error("ERROR on accept");

  return fd;
}

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

unsigned int pipe_value_get(unsigned int *val_ptr)
{
  int bytes_read;
  int i;


  if(lwl_sim_ppos<0)
  {
    if (lwl_fifo<0) 
    {
      if (use_port) 
      {
        lwl_fifo = sock_accept();
      }
      else
      {
        return 0;
      }
    }

    bytes_read = read(lwl_fifo,&lwl_sim_buffer[lwl_sim_bytes_read],(LWL_SIM_BUFFER_SIZE-lwl_sim_bytes_read));

    if ((lwl_fifo==0)&&(bytes_read==0)) return -1;

    if (bytes_read>0) 
    {
      lwl_sim_buffer[lwl_sim_bytes_read+bytes_read]=0;
//      printf("%s",&lwl_sim_buffer[lwl_sim_bytes_read]);
//      printf("lwl_got_data:>>%s<<\n",&lwl_sim_buffer[lwl_sim_bytes_read]);
      lwl_sim_bytes_read += bytes_read;
      lwl_sim_ppos=0;
    }
    else
    {
      if(use_port)
      {
        close(lwl_fifo);
        lwl_fifo=-1;
      }
      return 0;
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
          sscanf(&lwl_sim_buffer[lwl_sim_spos],"%x",val_ptr);
          lwl_sim_spos = -1;
          lwl_sim_ppos++;
//          printf("LWL FIFO: 0x%08x\n",*val_ptr);
          return 1;
        }
      }
      else
      {
       // syntax error - set error Flag
        lwl_sim_status |= 1;
      }
    }
//    if (lwl_sim_buffer[lwl_sim_ppos] == EOF) exit(0);

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
  return 0;

}

/******************************************************************************/

void transmit_data(void)
{
  int status=0;
  unsigned int value;

  while(status >= 0)
  {
    status = pipe_value_get(&value);
    if(status>0)
    {
//        printf("LWL FIFO: 0x%08x\n",value);
      testgen_send(value);
    }
  }

}

/******************************************************************************/

static void terminateSignal(int signal)
{
  printf("\n\nvme_lwl: Task terminated.\n\n");

  if (lwl_fifo>2) close(lwl_fifo);
  lwl_fifo = -1;

  exit(0);
}

/******************************************************************************/


int main(int argc, char *argv[])
{
  int port, i;
  unsigned int base = 0x1a00;
  unsigned int use_pipe = 0;

  for (i=1; i<argc; i++)
  {

    if (strncmp(argv[i],"base=",5)==0)
    {
      sscanf(argv[i]+5, "%x", &base);
    }
    else if (strncmp(argv[i],"lwlfifo=",8)==0)
    {
      strncpy(lwl_fifo_pipe_name,argv[i]+8,256);
      use_pipe = 1;
    }
    else if (strcmp(argv[i],"lwlfifo")==0)
    {
      use_pipe = 1;
    }
    else if (strncmp(argv[i],"port=",5)==0)
    {
      port = atoi(argv[i]+5);
      use_port = 1;
    }
    else if (strcmp(argv[i],"port")==0)
    {
      port = 5555;
      use_port = 1;
    }
    else 
    {
      printf("ERROR: wrong parameter: %s\n",argv[i]);
      exit(1);
    }
  }




  if (!testgen_setbase(base))
  {
    printf ("ERROR: from testgen_setbase\n");
    return 1;
  }

  testgen_init();

  signal(SIGTERM,terminateSignal);
  signal(SIGINT,terminateSignal);

  if (use_pipe)
  {
    lwl_fifo = open(lwl_fifo_pipe_name,O_RDONLY | O_NONBLOCK);

    if (lwl_fifo<0)
    {
      printf("ERROR: cannot open named pipe: %s\n",lwl_fifo_pipe_name);
      exit(1);
    }
  }
  else if (use_port)
  {
    printf("VMETgen Listen on port %d\n",port);
    sock_init(port);
  }
  else
  {
 /*
    if (fcntl(0, F_SETFL, O_NONBLOCK) < 0)
    {
      perror("fcntl");
      exit(1);
    }
 */
    lwl_fifo = 0;  // stdin
  }


  transmit_data();

  return 0;

}
/*******************************************************************/


