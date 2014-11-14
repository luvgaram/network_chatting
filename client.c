#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
//#include <pthread.h>

#define BUF_SIZE 1024
#define IP "127.0.0.1"
#define PORT 3000
#define IN_CURSOR 30 // 입력 커서 위치
#define LINE_SIZE 20 // 채팅줄 수
#define EPOLL_SIZE 2 // epoll fd 수, stdin & clnt_sock

void* send_msg(void* arg);
void* recv_msg(void* arg);

char msg[BUF_SIZE];
int cursor = LINE_SIZE;

int main()
{
	struct sockaddr_in serv_adr;
	struct epoll_event *ep_evnts;
	struct epoll_event evnt;
	int clnt_sock, read_tot, write_tot, ep_fd, evnt_cnt, nfds, flag, i;

//	pthread_t snd_thread, rcv_thread;
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

	printf("\033[2J");
	printf("%c[%d;%df", 0x1B, ((cursor++) % 20) + 1, 25);
	printf ("server connected\n");

	ep_fd = epoll_create(EPOLL_SIZE);
	ep_evnts = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

	evnt.events = EPOLLIN;
	evnt.data.fd = clnt_sock;

	if(epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &evnt) == -1) {
		perror("epoll_ctl_clnt_sock");
		exit(EXIT_FAILURE);
	}

	evnt.events = EPOLLIN | EPOLLET;
	evnt.data.fd = 0;

	if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, 0, &evnt) == -1){
		perror("epoll_ctli_stdin");
		exit(EXIT_FAILURE);
	}

	// nonblocking 
	flag = fcntl(clnt_sock, F_GETFL, 0);
	fcntl(clnt_sock, F_SETFL, flag | O_NONBLOCK);
	int rcv_len; // 임시!!!!!!!!!!!!!

	while(1) {
		if((nfds = epoll_wait(ep_fd, ep_evnts, EPOLL_SIZE, -1)) == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < nfds; i++) {
			if (ep_evnts[i].data.fd == clnt_sock) {
				rcv_len = read(clnt_sock, msg, BUF_SIZE - 1);
				msg[rcv_len] = 0;
				if (rcv_len >= 1) {
					printf("server: %s", msg);
				}
			} else if (ep_evnts[i].data.fd == 0) {
				rcv_len = read(0, msg, BUF_SIZE - 1);
				write(clnt_sock, msg, strlen(msg));
				msg[rcv_len] = 0;
				printf("client: %s", msg);
			}
			// clnt_sock에 읽을 데이터가 있다면
//			if(ep_evnts[i].data.fd == clnt_sock) {
//				if((read_tot = read(clnt_sock, msg, BUF_SIZE) <= 0)) {
//					perror("clnt_sock_read");
//					return -1;
//				}
//				if((write_tot = write(1, msg, read_tot)) != read_tot) {
//					perror("clnt_sock_write");
//					return -1;
//				}
//			} else { // stdin이 있다면
//				if(read_tot = read(0, msg, BUF_SIZE) <= 0) {
//					perror("stdin_read");
//					return -1;
//				}
//				printf("> %s", msg);
//				if((write_tot = write(clnt_sock, msg, read_tot)) != read_tot) {
//					perror("stdin_write");
//					return -1;
//				}
//			}
		}
	}


//	pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
//	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&clnt_sock);
//	pthread_join(snd_thread, &thread_rtn);
//	pthread_join(rcv_thread, &thread_rtn);
	close(clnt_sock);
	return 0;
}

void* send_msg(void* arg)
{
	int clnt_sock = *((int*)arg);

	while(1)
	{
		printf("%c[%d;%df", 0x1B, IN_CURSOR, 0);
		fgets(msg, BUF_SIZE, stdin);
		
		if(!strcmp(msg, "q\n"))
		{
			write(clnt_sock, msg, strlen(msg));
			close(clnt_sock);
			return NULL ;
		}

		printf("%c[%d;%df", 0x1B, ((cursor++) % LINE_SIZE) + 1, 25);
		printf("client: %s", msg);
		write(clnt_sock, msg, strlen(msg));
		printf("%c[%d;%df", 0x1B, IN_CURSOR, 0);
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
			printf("%c[%d;%df", 0x1B, ((cursor) % LINE_SIZE) + 1, 0);
			printf("server: %s", msg);
			int i = IN_CURSOR - ((cursor % LINE_SIZE) + 2);
			for(i; i > 0; i--) {
				printf("%s", "\n");
			}
			cursor++;
		}
	}

	return NULL;
}
