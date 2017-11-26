
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
#include "synch.h"

pcbuf *buf;

void display(char *prog, char *bytes, int n);

int producer(char * sh_mem, char const * message, int size) {
	Pthread_mutex_lock(&buf->mut); // p1
	while (buf->count == buf->bufCount) // p2
		Pthread_cond_wait(&buf->empty, &buf->mut); // p3
	printf("prod_string%s\n", sh_mem);
	memcpy(sh_mem, message, size);
	printf("prod_string post copy%s\n", sh_mem);
	buf->count++;
	Pthread_cond_signal(&buf->fill); // p5
	Pthread_mutex_unlock(&buf->mut); // p6
	return 0;
}

int consumer(char *sh_mem, int total) {
	int i = 0;
	Pthread_mutex_lock(&buf->mut); // c1
	while (buf->count == 0) // c2
		Pthread_cond_wait(&buf->fill, &buf->mut); // c3

	printf("cons_string%s\n", sh_mem);
	for(i = 0; i < total; i++) {
		printf("inside loop %c\n", sh_mem[i]);
	}
	buf->count--;
	Pthread_cond_signal(&buf->empty); // c5
	Pthread_mutex_unlock(&buf->mut); // c6
	return 0;
}

int main(void)
{
	const char *name = "/shm-data";	// file name
	const char *sync_name = "/shm-sync";	// file name
	const int SIZE = 4096;		// file size

	const char *message0 = "Operating Systems ";
	const char *message1 = "Testing ";
	const char *message2 = "Is Fun!";
	int length[4], pid, nbufs = 1;
	length[0] = strlen(message0);
	length[1] = strlen(message1);
	length[2] = strlen(message2);
	int shm_fd, sync_fd, ret = 0;		// file descriptor, from shm_open()
	char *shm_base;	// base address, from mmap()

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

/*	if (close(sync_fd) == -1) {
		printf("cons: Close failed: %s\n", strerror(errno));
		exit(1);
	}*/

	buf->bufCount = nbufs;
	buf->count = 0;
	// init the cond and mutex
	pthread_condattr_t cond_attr;
	pthread_condattr_init(&cond_attr);
	pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
	pthread_cond_init(&buf->empty, &cond_attr);
	pthread_cond_init(&buf->fill, &cond_attr);
	pthread_condattr_destroy(&cond_attr);

	pthread_mutexattr_t m_attr;
	pthread_mutexattr_init(&m_attr);
	pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&buf->mut, &m_attr);
	pthread_mutexattr_destroy(&m_attr);	

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

		printf("calling prod in child2\n");
		ret = producer(shm_base, message1, length[1]);


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
		sleep(3);

		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
		if (shm_fd == -1) {
			printf("cons: Shared memory failed: %s\n", strerror(errno));
			exit(1);
		}

		/* map the shared memory segment to the address space of the process */
		shm_base = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
		if (shm_base == MAP_FAILED) {
			printf("cons: Map failed: %s\n", strerror(errno));
			// close and unlink?
			exit(1);
		}

		printf("calling cons in parent\n");
		/* read from the mapped shared memory segment */
		ret = consumer(shm_base, length[0]);

		printf("calling cons in parent2\n");
		/* read from the mapped shared memory segment */
		ret = consumer(shm_base, length[1]);

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

