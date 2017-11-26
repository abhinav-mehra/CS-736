#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/tcp.h>

#define TOTAL_BUFFER 524288
#define NUM_INPUT 12

int
timediff(struct timespec start, struct timespec end, struct timespec *temp) {
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp->tv_sec = end.tv_sec-start.tv_sec-1;
		temp->tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp->tv_sec = end.tv_sec-start.tv_sec;
		temp->tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return 0;
}

int check (char* sent_data , char* recieved_data) {
	long count = 0;
	while (*sent_data != '\0'){
		if(((*sent_data) + 1) == *recieved_data) {
			sent_data ++ ;
			recieved_data ++ ;
			count++;
		} else {
			printf("the unmatching are %c %c %ld\n", *sent_data, *recieved_data, count);
			return 0;
		}
	}
	return 1 ;
}


void rand_str(char *dest, size_t length) {
	char charset[] = "0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ\0";

	while (length-- > 0) {
		size_t index = rand() % (strlen(charset));
		*dest++ = charset[index];
	}
	*dest = '\0';
} 


int main(int argc, char *argv[])
{
	int listenfd = 0, connfd = 0;
       	long int n=0;
	struct sockaddr_in serv_addr; 
	pid_t pid;

	bool result ;
	int  count , i , j;
	long int a[NUM_INPUT] = {1,2,4, 16, 64, 256, 1024, 4096, 16384,65536,262144,524288} ;
	struct timespec tp, tp2, diff, tp_cpu, tp_cpu2, diff2;   // variables for timer
	char *all_strings[NUM_INPUT] ;
	char *send_buf = NULL ;
	char *data_temp = NULL  ;
	char* data = NULL, *ptr;
	int  ret = 0;
	int iterations = atoi(argv[1]) ; 
	assert(argc == 2);
	cpu_set_t  mask;

	for ( i = 0 ; i < NUM_INPUT ; i++ ) {
		all_strings[i] = NULL ;
	}

	for ( i =0 ; i < NUM_INPUT ; i ++ ) { 
		data_temp = malloc(a[i]) ;
		rand_str(data_temp , a[i]) ;
		//   printf( "The message is %s \n" , data_temp  )  ;
		all_strings[i] = data_temp ;	
	}

	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000); 

	pid = fork();
	send_buf = malloc(TOTAL_BUFFER) ;   // TO DO : Discuss with abhinav 
	if(pid == -1) {
		printf("fork failure, exiting \n");
		exit(1);
	} if(pid == 0) {

		CPU_ZERO(&mask);
		CPU_SET(0, &mask);
		result = sched_setaffinity(0, sizeof(mask), &mask);
		assert(result == 0);

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

		int no_delay = 1;
		if (setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(int)) < 0) {
			perror("setsockopt failed!");
			exit(1);
		}

		if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("\n Error : BIND Failed %s\n", strerror(errno));
			exit(0);
		}

		if(listen(listenfd, 10) < 0) { 
			printf("\n Error : listen Failed %s\n", strerror(errno));
			exit(0);
		}
		printf("server listening\n");

		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

		for ( j = 0 ; j < iterations ; j++) {   
			for ( i = 0 ; i < NUM_INPUT ; i++) { 

				ptr = send_buf;
				n = read(connfd, ptr, a[i]);

				while ( n < a[i])
				{
					assert(n>0);
					n+=read(connfd, ptr, (a[i] - n));
				} 
				//			
	//			read(connfd, send_buf, a[i]);
				
				for(count = 0; count < a[i]; count++) {
					send_buf[count] = send_buf[count] + 1;
				}
				ptr = send_buf;
				n = write(connfd, ptr, a[i]); 
				while ( n < a[i])
				{
					assert(n>0);
					n+=write(connfd, ptr, (a[i] - n));
				} 
			}
		}

		close(connfd);

		/* close the socketfd as if it was a file */
		if (close(listenfd) == -1) {
			printf("prod: Close failed: %s\n", strerror(errno));
			exit(1);
		}
		exit(0); // TODO close socket

	} else {

		CPU_ZERO(&mask);
		CPU_SET(1, &mask);
		result = sched_setaffinity(0, sizeof(mask), &mask);
		assert(result == 0);

		sleep(1);
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

		int no_delay = 1;
		if (setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(int)) < 0) {
			perror("setsockopt failed!");
			exit(1);
		}

		if( connect(listenfd, (struct sockaddr *)&serv_addr,
					sizeof(serv_addr)) < 0)
		{
			printf("\n Error : Connect Failed %s\n", strerror(errno));
			kill(pid, SIGTERM);
			exit(0);
		} 


		for ( j = 0 ; j < iterations ; j++) {   
			for ( i = 0 ; i < NUM_INPUT ; i++) { 
				data = all_strings[i] ;
				//printf("The message sent by parent is %s \n" , data) ;

				//measuring roundtrip message latency
				ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
				ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu);

				ptr = data;
				n = write(listenfd, data, a[i]); 

				while ( n < a[i])
				{
				assert(n>0);
					n+=write(listenfd, ptr, (a[i] - n));

				} 

				ptr = send_buf;
				n = read(listenfd, ptr, a[i]);

				while ( n < a[i])
				{
				assert(n>0);
					n+=read(listenfd, ptr, (a[i] - n));

				} 

				ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp2);
				ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu2);
				timediff(tp, tp2, &diff);
				timediff(tp_cpu, tp_cpu2, &diff2);

				printf("the time taken for string with %ld bytes is\tMono %ld, %ld\tCPU %ld, %ld \n", a[i], diff.tv_sec, diff.tv_nsec,diff2.tv_sec, diff2.tv_nsec); 

				result = check(data, send_buf) ;   // TODO :verify this
				if (!result )
					printf ("ERROR: FAIL sent and received strings dont match\n") ; 
			}
			printf("\n");
		}

		/* close the socketfd as if it was a file */
		if (close(listenfd) == -1) {
			printf("prod: Close failed: %s\n", strerror(errno));
			exit(1);
		}
		kill(pid, SIGTERM);
		exit(0);
	}

}
