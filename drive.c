/*
 * Copyright (c) 2018 Florian Scholz and Jennifer Gommans <florian90@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of mosquitto nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


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

	x = 43 + (xi+500)*184/1000;
	y = 43 + (yi+500)*184/1000;

	printf("x=%x - y=%x\n", x, y);
	
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
