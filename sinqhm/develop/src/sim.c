
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>




int main( int argc, char **argv )
{
	int fd;
  unsigned int val;
  int result ;
  FILE *stream;
  char buffer[1024];

   stream = fopen( "/tmp/fifo", "r" );
   if( stream == NULL )
   {
      printf( "The fifo was not opened\n" );
      exit(1);
   }
   
   
    do
    {
      printf(".\n");
      if (!feof(stream))
      {
       printf("*\n");
      // result = fscanf( stream, "%x", &val );
//       fread(buffer,1,1,stream); 
       printf(">>>>>>>>>>>>>%c", buffer[0]);
      }
    }
    while (1);

//  fclose( stream );


  return 0;

}
/*******************************************************************/

