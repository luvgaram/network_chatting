#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define IP "127.0.0.1"
#define PORT 3000

void* send_msg(void* arg);
void* recv_msg(void* arg);

char msg[BUF_SIZE];

int main()
{
	int clnt_sock;
	struct sockaddr_in serv_adr;

	pthread_t snd_thread, rcv_thread;
	void* thread_rtn;

	if((clnt_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(IP);
	serv_adr.sin_port = htons(PORT);

	if(connect(clnt_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		perror("connect");
		close(clnt_sock);
	}

	pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&clnt_sock);
	pthread_join(snd_thread, &thread_rtn);
	pthread_join(rcv_thread, &thread_rtn);
	close(clnt_sock);
	return 0;
}

void* send_msg(void* arg)
{
	int clnt_sock = *((int*)arg);

	while(1)
	{
		fgets(msg, BUF_SIZE, stdin);
		
		if(!strcmp(msg, "q\n"))
		{
			write(clnt_sock, msg, strlen(msg));
			close(clnt_sock);
			return NULL ;
		}

		printf("client: %s", msg);
		write(clnt_sock, msg, strlen(msg));
	}

	return NULL;
}

void* recv_msg(void* arg)
{
	int clnt_sock = *((int*)arg);
	int rcv_len;

	while(1)
	{
		rcv_len = read(clnt_sock, msg, BUF_SIZE - 1);

		if(rcv_len == -1) {
			perror("receive");
			return (void*) -1;
		}
		
		if(!strcmp(msg, "q\n")) {
			printf("%s", "server is gone\n");
			close(clnt_sock);
			exit(0);
		}

		if(rcv_len > 0) {
			msg[rcv_len] = 0;
			printf("server: %s", msg);
		}
	}

	return NULL;
}
