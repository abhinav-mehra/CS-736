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
#include "synch.h"

#define NUM_INPUT 12
#define TOTAL_BUFFER 524288
pcbuf *buf;

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
	while (*sent_data != '\0'){
		if(((*sent_data) + 1) == *recieved_data) {
			sent_data ++ ;
			recieved_data ++ ;
		} else {
			return 0 ;
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



void display(char *prog, char *bytes, int n);

int producer(char * sh_mem, char const * message, int size) {
	sem_wait(&buf->empty);
//	printf("prod_string%s\n", sh_mem);
	memcpy(sh_mem, message, size);
//	printf("prod_string post copy%s\n", sh_mem);
	sem_post(&buf->fill); // p5
	return 0;
}

int consumer(char *sh_mem, char *buffer, int total) {
	sem_wait(&buf->fill); // p5
//	printf("cons_string%s\n", sh_mem);
	memcpy(buffer, sh_mem, total);
//	printf("\n");
	sem_post(&buf->empty);
	return 0;
}
int producer2(char * sh_mem, char const * message, int size) {
	sem_wait(&buf->empty2);
	memcpy(sh_mem, message, size);
	sem_post(&buf->fill2); // p5
	return 0;
}

int consumer2(char *sh_mem, char *buffer, int total) {
	sem_wait(&buf->fill2); // p5
//	printf("cons2_string%s\n", sh_mem);
	memcpy(buffer, sh_mem, total);
//	printf("\n");
	sem_post(&buf->empty2);
	return 0;
}

int main(int argc, char *argv[])
{
	const char *name = "/shm-data";	// file name
	const char *sync_name = "/shm-sync";	// file name
	int shm_fd, sync_fd, ret = 0;		// file descriptor, from shm_open()
	char *shm_base;	// base address, from mmap()
	pid_t pid;

	bool result ;
	int  count , i , j  ;     //pipe fd is for sending the data from parent to child and pipefd1 is for recieving the data from parent to child
	long int a[NUM_INPUT] = {1,2,4, 16, 64, 256, 1024, 4096, 16384,65536,262144,524288} ;
	struct timespec tp, tp2, diff, tp_cpu, tp_cpu2, diff2;   // variables for timer
	char *all_strings[NUM_INPUT] ;
	char *send_buf = NULL ;
	char *data_temp = NULL  ;
	char* data = NULL ;
	int iterations = atoi(argv[1]) ; 
	int nbufs = 1;
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

	/* create the shared memory segment as if it was a file */
	sync_fd = shm_open(sync_name, O_CREAT | O_RDWR, 0666);
	if (sync_fd == -1) {
		printf("prod: Shared memory failed: %s\n", strerror(errno));
		exit(1);
	}

	/* configure the size of the shared memory segment */
	ftruncate(sync_fd, sizeof(pcbuf));

	/* map the shared memory segment to the address space of the process */
	buf = (pcbuf *) mmap(0, sizeof(pcbuf), PROT_READ | PROT_WRITE, MAP_SHARED, sync_fd, 0);
	if (buf == MAP_FAILED) {
		printf("prod: Map failed: %s\n", strerror(errno));
		// close and shm_unlink?
		exit(1);
	}

	if (close(sync_fd) == -1) {
		printf("cons: Close failed: %s\n", strerror(errno));
		exit(1);
	}

	sem_init(&buf->empty, 1, nbufs);
	sem_init(&buf->fill, 1, 0);

	sem_init(&buf->empty2, 1, 1);
	sem_init(&buf->fill2, 1, 0);

	pid = fork();
	send_buf = malloc(TOTAL_BUFFER) ;   // TO DO : Discuss with abhinav 
	if (pid < 0) {
		printf("fork failed\n");
		exit(0);
	} else if (pid == 0) {

		CPU_ZERO(&mask);
		CPU_SET(0, &mask);
		result = sched_setaffinity(0, sizeof(mask), &mask);
		assert(result == 0);

		/* create the shared memory segment as if it was a file */
		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
		if (shm_fd == -1) {
			printf("prod: Shared memory failed: %s\n", strerror(errno));
			exit(1);
		}

		/* configure the size of the shared memory segment */
		ftruncate(shm_fd, TOTAL_BUFFER);

		/* map the shared memory segment to the address space of the process */
		shm_base = mmap(0, TOTAL_BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (shm_base == MAP_FAILED) {
			printf("prod: Map failed: %s\n", strerror(errno));
			// close and shm_unlink?
			exit(1);
		}

		for ( j = 0 ; j < iterations ; j++) {   
			for ( i = 0 ; i < NUM_INPUT ; i++) { 

				ret = consumer(shm_base, send_buf, a[i]);
				for(count = 0; count < a[i]; count++) {
					send_buf[count] = send_buf[count] + 1;
				}
				ret = producer2(shm_base, send_buf, a[i]);
			}
		}

		/* remove the mapped memory segment from the address space of the process */
		if (munmap(shm_base, TOTAL_BUFFER) == -1) {
			printf("prod: Unmap failed: %s\n", strerror(errno));
			exit(1);
		}

		/* close the shared memory segment as if it was a file */
		if (close(shm_fd) == -1) {
			printf("prod: Close failed: %s\n", strerror(errno));
			exit(1);
		}

		exit(EXIT_SUCCESS);
		return 0;
	} else if (pid > 0) {

		CPU_ZERO(&mask);
		CPU_SET(1, &mask);
		result = sched_setaffinity(0, sizeof(mask), &mask);
		assert(result == 0);

		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
		if (shm_fd == -1) {
			printf("cons: Shared memory failed: %s\n", strerror(errno));
			exit(1);
		}

		/* configure the size of the shared memory segment */
		ftruncate(shm_fd, TOTAL_BUFFER);

		/* map the shared memory segment to the address space of the process */
		shm_base = mmap(0, TOTAL_BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (shm_base == MAP_FAILED) {
			printf("cons: Map failed: %s\n", strerror(errno));
			// close and unlink?
			exit(1);
		}

		for ( j = 0 ; j < iterations ; j++) {   
			for ( i = 0 ; i < NUM_INPUT ; i++) { 
				data = all_strings[i] ;
				//printf("The message sent by parent is %s \n" , data) ;

				//measuring roundtrip message latency
				ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
				ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu);

				// Starting message sending
				ret = producer(shm_base, data, a[i]);
				ret = consumer2(shm_base, send_buf, a[i]);
				//

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

		/* remove the mapped shared memory segment from the address space of the process */
		if (munmap(shm_base, TOTAL_BUFFER) == -1) {
			printf("cons: Unmap failed: %s\n", strerror(errno));
			exit(1);
		}

		/* close the shared memory segment as if it was a file */
		if (close(shm_fd) == -1) {
			printf("cons: Close failed: %s\n", strerror(errno));
			exit(1);
		}

		/* remove the shared memory segment from the file system */
		if (shm_unlink(name) == -1) {
			printf("cons: Error removing %s: %s\n", name, strerror(errno));
			exit(1);
		}

		wait(NULL);                /* Wait for child */
		exit(0);
		return 0;	
	}
		return 0;	
}

void display(char *prog, char *bytes, int n)
{
	int i = 0;
	printf("display: %s\n", prog);
	for (i = 0; i < n; i++) 
	{ printf("%02x%c", bytes[i], ((i+1)%16) ? ' ' : '\n'); }
	printf("\n");
}

