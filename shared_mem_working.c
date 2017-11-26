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

bool check (char* sent_data , char* recieved_data) {
	while (*sent_data != '\0'){
		if(*sent_data == *recieved_data) {
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



void display(char *prog, char *bytes, int n);

int producer(char * sh_mem, char const * message, int size) {
	sem_wait(&buf->empty);
	printf("prod_string%s\n", sh_mem);
	memcpy(sh_mem, message, size);
	printf("prod_string post copy%s\n", sh_mem);
	sem_post(&buf->fill); // p5
	return 0;
}

int consumer(char *sh_mem, int total) {
	int i = 0;
	sem_wait(&buf->fill); // p5
	printf("cons_string%s\n", sh_mem);
	for(i = 0; i < total; i++) {
		printf("%c ", sh_mem[i]);
	}
	printf("\n");
	sem_post(&buf->empty);
	return 0;
}
int producer2(char * sh_mem, char const * message, int size) {
	sem_wait(&buf->empty2);
	printf("prod2_string%s\n", sh_mem);
	memcpy(sh_mem, message, size);
	printf("prod_string post copy%s\n", sh_mem);
	sem_post(&buf->fill2); // p5
	return 0;
}

int consumer2(char *sh_mem, int total) {
	int i = 0;
	sem_wait(&buf->fill2); // p5
	printf("cons2_string%s\n", sh_mem);
	for(i = 0; i < total; i++) {
		printf("%c ", sh_mem[i]);
	}
	printf("\n");
	sem_post(&buf->empty2);
	return 0;
}

int main(void)
{
	const char *name = "/shm-data";	// file name
	const char *sync_name = "/shm-sync";	// file name
	const int SIZE = 4096;		// file size
	int shm_fd, sync_fd, ret = 0;		// file descriptor, from shm_open()
	char *shm_base, *ptr;	// base address, from mmap()

	const char *message0 = "Operating Systems ";
	const char *message1 = "Testing ";
	const char *message2 = "ack";
	int length[4], pid, nbufs = 2;
	length[0] = strlen(message0);
	length[1] = strlen(message1);
	length[2] = strlen(message2);

	/* create the shared memory segment as if it was a file */
	sync_fd = shm_open(sync_name, O_CREAT | O_RDWR, 0666);
	if (sync_fd == -1) {
		printf("prod: Shared memory failed: %s\n", strerror(errno));
		exit(1);
	}

	/* configure the size of the shared memory segment */
	ftruncate(sync_fd, sizeof(pcbuf));

	/* map the shared memory segment to the address space of the process */
	buf = (pcbuf *) mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sync_fd, 0);
	if (buf == MAP_FAILED) {
		printf("prod: Map failed: %s\n", strerror(errno));
		// close and shm_unlink?
		exit(1);
	}

	if (close(sync_fd) == -1) {
		printf("cons: Close failed: %s\n", strerror(errno));
		exit(1);
	}

/*	buf->bufCount = nbufs;
	buf->count = 0; */
	
	sem_init(&buf->empty, 1, nbufs);
	sem_init(&buf->fill, 1, 0);

	sem_init(&buf->empty2, 1, 1);
	sem_init(&buf->fill2, 1, 0);

	pid = fork();
	if (pid < 0) {
		printf("fork failed\n");
		exit(0);
	} else if (pid == 0) {


		/* create the shared memory segment as if it was a file */
		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
		if (shm_fd == -1) {
			printf("prod: Shared memory failed: %s\n", strerror(errno));
			exit(1);
		}

		/* configure the size of the shared memory segment */
		ftruncate(shm_fd, SIZE);

		/* map the shared memory segment to the address space of the process */
		shm_base = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (shm_base == MAP_FAILED) {
			printf("prod: Map failed: %s\n", strerror(errno));
			// close and shm_unlink?
			exit(1);
		}

		/**
		 * Write to the mapped shared memory region.
		 *
		 */

		printf("calling prod in child\n");
		ret = producer(shm_base, message0, length[0]);
		ptr = shm_base + length[0];

		printf("calling prod in child2\n");
		ret = producer(ptr, message1, length[1]);
		ptr = ptr + length[1];

		printf("calling cons2 in child3\n");
		ret = consumer2(ptr, length[2]);
		ptr = ptr + length[2];

		/* remove the mapped memory segment from the address space of the process */
		if (munmap(shm_base, SIZE) == -1) {
			printf("prod: Unmap failed: %s\n", strerror(errno));
			exit(1);
		}

		/* close the shared memory segment as if it was a file */
		if (close(shm_fd) == -1) {
			printf("prod: Close failed: %s\n", strerror(errno));
			exit(1);
		}
		return 0;
	} else if (pid > 0) {

		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
		if (shm_fd == -1) {
			printf("cons: Shared memory failed: %s\n", strerror(errno));
			exit(1);
		}

		/* configure the size of the shared memory segment */
		ftruncate(shm_fd, SIZE);

		/* map the shared memory segment to the address space of the process */
		shm_base = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		if (shm_base == MAP_FAILED) {
			printf("cons: Map failed: %s\n", strerror(errno));
			// close and unlink?
			exit(1);
		}

		printf("calling cons in parent\n");
		/* read from the mapped shared memory segment */
		ret = consumer(shm_base, length[0]);
		ptr = shm_base + length[0];

		printf("calling cons in parent2\n");
		/* read from the mapped shared memory segment */
		ret = consumer(ptr, length[1]);
		ptr = ptr + length[1];

		printf("calling prod2 in parent3 message is %s diff is %d\n", message2, (int)((int) ptr - (int) shm_base));
		/* read from the mapped shared memory segment */
		ret = producer2(ptr, message2, length[2]);
		ptr = ptr + length[2];

		/* remove the mapped shared memory segment from the address space of the process */
		if (munmap(shm_base, SIZE) == -1) {
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

