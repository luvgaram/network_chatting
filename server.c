#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>

#define BUF_SIZE 1024
#define PORT 3000
#define IN_CURSOR 30 // 입력 커서 위치
#define EPOLL_SIZE 10 // 동시 접속 클라이언트 수 

void* send_msg(void* arg);
void* recv_msg(void* arg);
int kbhit(void);

char msg[BUF_SIZE];
int cursor = 20;

int main()
{
	int str_len = -1;
	int serv_sock, clnt_sock;

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	struct epoll_event *ep_evnts;
	struct epoll_event evnt;
	int ep_fd, evnt_cnt, i;


//	pthread_t snd_thread, rcv_thread;
	void* thread_rtn;

	if((serv_sock = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return str_len;
	}

	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(PORT);

	if(str_len = bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))) {
		perror("bind");
		close(serv_sock);
	}

	if(str_len = listen(serv_sock, 1)) {
		perror("listen");
		close(serv_sock);
	}

	evnt.events = EPOLLIN;
	evnt.data.fd = serv_sock;
	epoll_ctl(ep_fd, EPOLL_CTL_ADD, serv_sock, &evnt);

	while(1) {
		evnt_cnt = epoll_wait(ep_fd, ep_evnts, EPOLL_SIZE, -1); // 이벤트 발생까지 무한 대기
		if(evnt_cnt == -1) {
			perror("epoll_wait");
			break;
		}

		for(i = 0; i < evnt_cnt; i++) {
			if(ep_evnts[i].data.fd == serv_sock) {
				clnt_adr_sz = sizeof(clnt_adr);
				clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
				if(clnt_sock == -1) {
					perror("accept");
				}
				evnt.events = EPOLLIN;
				evnt.data.fd = clnt_sock;
				epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &evnt);
				printf("%c[%d;%df", 0x1B, ((cursor) % 20) + 1, 0);
				printf("new client connected : %d \n", clnt_sock);
			
//				pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
//				pthread_create(&rcv_thread, NULL, recv_msg, (void*)&clnt_sock);
//				pthread_join(snd_thread, &thread_rtn);
//			 	pthread_join(rcv_thread, &thread_rtn);
			}
			else {
				str_len = read(ep_evnts[i].data.fd, msg, BUF_SIZE);
				if(str_len == 0) {
					epoll_ctl(ep_fd, EPOLL_CTL_DEL, ep_evnts[i].data.fd, NULL);
					close(ep_evnts[i].data.fd);
					printf("%c[%d;%df", 0x1B, ((cursor) % 20) + 1, 0);
					printf("client closed : %d \n", ep_evnts[i].data.fd);
				} else {
					str_len = read(ep_evnts[i].data.fd, msg, BUF_SIZE);
					if(str_len == 0) {
						epoll_ctl(ep_fd, EPOLL_CTL_DEL, ep_evnts[i].data.fd, NULL);
					} else {
//						pthread_detach(snd_thread);
//						pthread_detach(rcv_thread);

						recv_msg((void*)&clnt_sock);
					}
				}
			}
		}

//	if(clnt_sock == -1) {
//		perror("accept");
//		str_len = -1;
//		close(serv_sock);
//	}
//	// 화면 지움
//	printf("\033[2J");
//
//	// 쓰레드 생성
//	pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
//	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&clnt_sock);
//	pthread_join(snd_thread, &thread_rtn);
//	pthread_join(rcv_thread, &thread_rtn);/	close(clnt_sock);
	close(serv_sock);
	close(ep_fd);
	return 0;
//	return str_len;

	}
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
			return NULL;;
		}

		printf("%c[%d;%df", 0x1B, ((cursor++) % 20) + 1, 25);
		printf("server: %s", msg);
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
			printf("%s", "client is gone\n");
			close(clnt_sock);
			exit(0);
		} 
		
		if(rcv_len > 0) {
			msg[rcv_len] = 0;

			printf("%c[%d;%df", 0x1B, ((cursor) % 20) + 1, 0);
			printf("client: %s", msg);
			int i = IN_CURSOR - ((cursor % 20) + 2);
			for(i; i > 0; i--) {
				printf("%s", "\n");
			}
			cursor++;
		}
	}
	return NULL;
}
