/**
* This is a little program which reads the content of a historgram
* memory data file and prints the values to stdout
* 
* Mark Koennecke, Gerd Theidel, September 2005
*/
#include <stdio.h>
#include <winsock2.h>
#include "lwlpmc.h"

unsigned int packet_len(unsigned int header)
{
  unsigned int header_type;

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
    return 4;
  }
  else if (header_type & LWL_HEADER_DS4 )
  {
    /* if DS4 is set the length is encoded in DS3..DS0 */
    /* the return value is the same as [DS3..DS0] + 1  */
    return ( (header_type>>24) - 15);
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


int pv = 0;
int plen = 0;

void process(unsigned int val)
{
    if (val & 0xffff0000)
    {
      plen = packet_len(val);
 //     if (plen>2)
      {
        printf("---> len=%d\n",plen);
        pv = 10;
      }
    }

    if (plen<0)
    { 
       printf("len ERROR\n");
       pv=10;
    }

		if (pv)
    { 
       printf("0x%08x\n",val);
       pv--;
    }
    plen--;
}


int main(int argc, char *argv[]){
	FILE *fd = NULL;
	int val;
	int numread = 0;

	if(argc < 2){
		puts("Usage:\n\tdecodehmdata datafile\n");
		exit(1);
	}
	
	fd = fopen(argv[1],"rb");
	if(fd == NULL){
		puts("Cannot open data file\n");
		exit(1);
	}

	while(!feof(fd))
  {
    numread += fread(&val,sizeof(unsigned int),1,fd);
//    printf("++++ read %d elements\n",numread);
    process(ntohl(val));
	}
	fclose(fd);

  printf("++++ read %d elements\n",numread);
}

