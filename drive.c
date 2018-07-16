#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

int main( int argc, char **argv, char **envp )
{
	int fdx, fdy;
	int xi, yi; // x input, y input
	int time=0;
	unsigned char x, y;
	struct timespec sleeptime;

	printf("argc: %d\n", argc );
	if (argc<3) {
		printf("usage: %s x y [time]\n", argv[0]);
		printf("\t-500<=x<=500; -500<=y<=500\n");
		printf("\ttime in ms: Zeit bis zum stoppen (ohne time oder "
			"bei time=0 kein automatischer Stop)\n");
		return -1;
	}
	xi = atoi( argv[1] );
	yi = atoi( argv[2] );
	if (argc==4) {
		time = atoi( argv[3] );
		printf("stopping after %d seconds and %d milliseconds\n",
			time/1000, time%1000);
	}
	if (xi >= 500) xi =  499;
	if (xi <=(-500)) xi = -499;
	if (yi >= 500) yi =  499;
	if (yi <=(-500)) yi = -499;

	printf("x=%x - y=%x -- ", xi, yi);

/* This doesn't work!  Congratulations by Florian Scholz, please visit EPR */
#if 1
#warning "The behaviour of this code is undefined.\nSigned char goes from -127 to 127\n and not from -255 to 255"
//Offset 43  und 184 statt 256
	x = 43 + (xi+500)*184/1000;
	y = 43 + (yi+500)*184/1000;
#else
	x = (xi+500)*127/1000;
	y = (yi+500)*127/1000;
#endif

	printf("x=%x - y=%x\n", x, y);
	
	//Nullwerte filtern 
	if( x == 0x00 || y == 0x00){
		return -1; 
	}

	fdx = open( "/dev/dac-1", O_WRONLY );
	fdy = open( "/dev/dac-2", O_WRONLY );
	if( fdx <0 || fdy<0 ) {
		perror("dev/dac-x/y");
		return -1;
	}

	printf("Start engine %02x/%02x...\n", x,y);
	write( fdx, &x, sizeof(x) );
	write( fdy, &y, sizeof(y) );
	if (time!=0) { // Automatischer stop
		sleeptime.tv_sec  = time/1000;
		sleeptime.tv_nsec = (time%1000)*1000000;
		clock_nanosleep( CLOCK_MONOTONIC, 0, &sleeptime, NULL );
		y=x = 0x80;
		write( fdx, &x, sizeof(x) );
		write( fdy, &y, sizeof(y) );
		printf("engine stopped...\n");
	}
	return 0;
}
