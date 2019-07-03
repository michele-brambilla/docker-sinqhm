#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	FILE *fd = NULL;
  char val[100];
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
    numread += fread(val,sizeof(char),1,fd);
	}
	fclose(fd);

  printf("++++ read %d bytes\n",numread);
}
