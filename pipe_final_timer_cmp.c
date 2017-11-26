#define _GNU_SOURCE
#include <sys/wait.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include<time.h>
#include <sched.h>

#include "hwtimer.h"

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

bool check (char* sent_data , char* recieved_data) {
	while (*sent_data != '\0'){
		if(((*sent_data) + 1) == *recieved_data) {
			sent_data ++ ;
			recieved_data ++ ;
		} else 
			return false ;
	}
	return true ;
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

	int
main(int argc, char *argv[])
{   
	bool result ;
	int flag , pipefd[2] , pipefd1[2] , i , j, count  ;     //pipe fd is for sending the data from parent to child and pipefd1 is for recieving the data from parent to child
	long int a[NUM_INPUT] = {1,2,4, 16, 64, 256, 1024, 4096, 16384,65536,262144,524288} ;
	struct timespec tp, tp2, diff, tp_cpu, tp_cpu2, diff2;   // variables for timer
	int ret =0 ; 
	//  long int a[10] = {1,2,3,4,5,6,7,8,9,10} ;
	char *all_strings[NUM_INPUT] ;
	int cpid;
	char *send_buf = NULL ;
	char *data_temp = NULL  ;
	char* data = NULL ;
	assert(argc == 2);
	int iterations = atoi(argv[1]) ; 
	cpu_set_t  mask;

	unsigned long ns_tsc = 0;
    	hwtimer_t timer;
    	initTimer(&timer);

	for ( i = 0 ; i < NUM_INPUT ; i++ ) {
		all_strings[i] = NULL ;
	}

	for ( i =0 ; i < NUM_INPUT ; i ++ ) { 
		data_temp = malloc(a[i]) ;
		rand_str(data_temp , a[i]) ;
		//   printf( "The message is %s \n" , data_temp  )  ;
		all_strings[i] = data_temp ;	
	}


	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}


	if (pipe(pipefd1) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}


	cpid = fork();
	send_buf = malloc(524288) ;   // TO DO : Discuss with abhinav 
	if (cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (cpid == 0) {    /* Child reads from pipe */
		CPU_ZERO(&mask);
		CPU_SET(0, &mask);
		result = sched_setaffinity(0, sizeof(mask), &mask);
		assert(result == 0);
		close(pipefd[1]);          /* Close unused write end */
		close(pipefd1[0]) ; // close unused read end

		for ( j = 0 ; j < iterations ; j++) {   
			for ( i = 0 ; i < NUM_INPUT ; i++) { 

				count = read(pipefd[0], send_buf, a[i]);
				for(count = 0; count < a[i]; count++) {
					send_buf[count] = send_buf[count] + 1;
				}
				///printf
				//printf ("The message read by child is %s \n" , send_buf )  ;	      
				write(pipefd1[1], send_buf, a[i]);
			}
		}
		close(pipefd[0]);
		close(pipefd1[1]) ;

		exit(EXIT_SUCCESS);

	} else {            /* Parent writes argv[1] to pipe */
		CPU_ZERO(&mask);
		CPU_SET(1, &mask);
		result = sched_setaffinity(0, sizeof(mask), &mask);
		assert(result == 0);
		close(pipefd[0]);          /* Close unused read end */
		close(pipefd1[1]); // close unused write end of fd1

		for ( j = 0 ; j < iterations ; j++) {   
			printf("\nstarting iteration %d\n\n",j);
			for ( i = 0 ; i < NUM_INPUT ; i++) { 
				data = all_strings[i] ;
				//printf("The message sent by parent is %s \n" , data) ;

				//measuring roundtrip message latency
    				startTimer(&timer); // rdtsc
				ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
				ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu);

				// Starting message sending
				write(pipefd[1], data , a[i] );
				count = read(pipefd1[0], send_buf, a[i]) ;
				//

				ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp2);
				ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu2);
    				stopTimer(&timer); // rdtsc
				timediff(tp, tp2, &diff);
				timediff(tp_cpu, tp_cpu2, &diff2);
				ns_tsc = getTimerNs(&timer);
				//ABhinav MOD 1
				if(count != a[i]) {
					printf( "ERROR: difference is %d and %ld\n", count, a[i]);
				}

				printf("the time taken for string with %ld bytes is\tMono %ld, %ld\tCPU %ld, %ld\tRDTSC %lu\n", a[i], diff.tv_sec, diff.tv_nsec,diff2.tv_sec, diff2.tv_nsec, ns_tsc); 


				result = check (data, send_buf) ;   // TODO :verify this
				int length = strlen(send_buf) ;
				if (!result )
					printf ("ERROR: FAIL sent and received strings dont match") ; 
			}
		}
		close(pipefd[1]);          /* Reader will see EOF */
		wait(NULL);                /* Wait for child */
		close(pipefd1[0]) ;

	}   
	return 0;
}

