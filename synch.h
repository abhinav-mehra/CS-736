#include <pthread.h>
#include <semaphore.h>


typedef struct {
	sem_t empty, fill, empty2, fill2;
} pcbuf;


/* Unix pthread wrappers */
void Pthread_mutex_lock(pthread_mutex_t *m);
void Pthread_mutex_unlock(pthread_mutex_t *m);
void Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 	
	       void * (*start_routine)(void *), void *arg);
void Pthread_join(pthread_t thread, void **value_ptr);
void Pthread_mutex_init(pthread_mutex_t *mutex,
		              const pthread_mutexattr_t *attr);
void Pthread_cond_init(pthread_cond_t *cond,
	const pthread_condattr_t *attr);
void Pthread_cond_signal(pthread_cond_t *cond);
void Pthread_cond_wait(pthread_cond_t *cond,
	       	pthread_mutex_t *mutex);

void posix_error(int code, char *msg) /* posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
        unix_error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length) 
{
    if (munmap(start, length) < 0)
        unix_error("munmap error");
}

/****************************************
 * Wrapper for pthread functions
 ***************************************/
void
Pthread_mutex_lock(pthread_mutex_t *m)
{
    int rc = pthread_mutex_lock(m);
    if(rc != 0) posix_error(rc, "Pthread_mutex_lock error");
}
                                                                                
void
Pthread_mutex_unlock(pthread_mutex_t *m)
{
    int rc = pthread_mutex_unlock(m);
    if(rc != 0) posix_error(rc, "Pthread_mutex_unlock error");
}
                                                                                
void
Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 	
	       void * (*start_routine)(void *), void *arg)
{
    int rc = pthread_create(thread, attr, start_routine, arg);
    if(rc != 0) posix_error(rc, "Pthread_create error");
}

void
Pthread_join(pthread_t thread, void **value_ptr)
{
    int rc = pthread_join(thread, value_ptr);
    if(rc != 0) posix_error(rc, "Pthread_join error");
}

void 
Pthread_mutex_init(pthread_mutex_t *mutex,
		              const pthread_mutexattr_t *attr)
{
	int rc = pthread_mutex_init(mutex, attr);
    if(rc != 0) posix_error(rc, "Pthread_mutex_init error");
}

void Pthread_cond_init(pthread_cond_t *cond,
	const pthread_condattr_t *attr)
{
	int rc = pthread_cond_init(cond, attr);
    if(rc != 0) posix_error(rc, "Pthread_cond_init error");
}

void Pthread_cond_signal(pthread_cond_t *cond)
{
	int rc = pthread_cond_signal(cond);
    if(rc != 0) posix_error(rc, "Pthread_cond_signal error");
}

void Pthread_cond_wait(pthread_cond_t *cond,
	       	pthread_mutex_t *mutex)
{
	int rc = pthread_cond_wait(cond, mutex);
    if(rc != 0) posix_error(rc, "Pthread_cond_wait error");
}

