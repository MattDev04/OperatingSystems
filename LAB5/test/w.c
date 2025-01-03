#include <stdlib.h>
#include <unistd.h>
#include <lib.h>


PUBLIC int worst_fit( int w )
{
	message mess;
  	mess.m1_i1 = w;
  	return _syscall(MM, WORST_FIT, &mess);
}

int
main( int argc, char *argv[] )
{
	if( argc < 2 )
		return 1;
	worst_fit( atoi( argv[1] ) );
	return 0;
}

