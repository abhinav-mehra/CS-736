#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>


void client()
{
}

int main(int argc, char *argv[])
{
	int listenfd = 0, connfd = 0, n;
	struct sockaddr_in serv_addr; 
	pid_t pid;

	char sendBuff[1025], recvBuff[1025];

	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(sendBuff, '0', sizeof(sendBuff)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000); 

	pid = fork();
	if(pid == -1) {
		printf("fork failure, exiting \n");
		exit(1);
	} if(pid == 0) {

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("\n Error : BIND Failed %s\n", strerror(errno));
			exit(0);
		}

		if(listen(listenfd, 10) < 0) { 
			printf("\n Error : listen Failed %s\n", strerror(errno));
			exit(0);
		}
		printf("server listening\n");

		while(1)
		{
			connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

			read(connfd, recvBuff, sizeof(recvBuff)-1);
			write(connfd, recvBuff, strlen(recvBuff)); 
			printf("server recd %s\n", recvBuff);

			close(connfd);
		}
	} else {
		sleep(1);
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
		if( connect(listenfd, (struct sockaddr *)&serv_addr,
					sizeof(serv_addr)) < 0)
		{
			printf("\n Error : Connect Failed %s\n", strerror(errno));
			kill(pid, SIGTERM);
			exit(0);
		} 
		snprintf(sendBuff, sizeof(sendBuff), "testing 123 %s\n", argv[1]);
		write(listenfd, sendBuff, strlen(sendBuff)); 
		while ( (n = read(listenfd, recvBuff, sizeof(recvBuff)-1)) > 0)
		{
			recvBuff[n] = 0;
			if(fputs(recvBuff, stdout) == EOF)
			{
				printf("\n Error : Fputs error\n");
			}
		} 

		if(n < 0)
		{
			printf("\n Read error \n");
		}
		kill(pid, SIGTERM);
		exit(0);
	}

}
