#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdlib.h>
#include <errno.h>


int main(int argc, char *argv[]){
	FILE* rptr;
	
	rptr = fopen("/proc/net/tcpstat", "r");
	char outbuf[4096];
	
	printf("Активные соединения с интернетом\n");
	
	while(fgets(outbuf, 4096, rptr)) {
		printf("%s", outbuf);
	}	
	
	fclose(rptr);
	
	memset(outbuf, 0, 4096);
	rptr = fopen("/proc/net/udpstat", "r");
	
	fgets(outbuf, 4096, rptr);
	
	while (fgets(outbuf, 4096, rptr)) {
		printf("%s", outbuf);
	}
	
	fclose(rptr);
		
	memset(outbuf, 0, 4096);
	
	printf("Активные сокеты домена UNIX\n");
	
	rptr = fopen("/proc/net/unixstat", "r");
	
	while (fgets(outbuf, 4096, rptr)) {
		printf("%s", outbuf);
	}
	
	fclose(rptr);
	
	return 0;
}
