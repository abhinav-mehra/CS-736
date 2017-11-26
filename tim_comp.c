#include<stdlib.h>
#include<stdio.h>
#include<time.h>

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

int
main(int argc, char* argv[]) {
	char a[65536];
	int i = 0,j = 0, ret = 0;
	struct timespec tp, tp2, diff, tp_cpu, tp_cpu2, diff2;
	for(i=0;i<10;i++) {
		ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
		ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu);
		for(j=0;j<(i*1000);j++) {
			a[j] = (j+i);
		}
		ret = clock_gettime(CLOCK_MONOTONIC_RAW, &tp2);
		ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp_cpu2);
		timediff(tp, tp2, &diff);
		timediff(tp_cpu, tp_cpu2, &diff2);
		printf("the monotonic value is %ld:%ld\tCPU  value is %ld:%ld for %d\n", diff.tv_sec, diff.tv_nsec,diff2.tv_sec, diff2.tv_nsec, i);
	}
	return i;
}
