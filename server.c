#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
//#include <pthread.h>
#include <fcntl.h>
//#include <termios.h>

#define BUF_SIZE 1024
#define PORT 3000
#define IN_CURSOR 22 // 입력 커서 위치
#define LINE_SIZE 20 // 채팅줄 수
#define EPOLL_SIZE 20 // epoll fd 수 

struct udata {
	int fd;
	char name[80];
};

int user_fds[8];
void send_msg(struct epoll_event evnt, char *msg);
//void* send_msg(void* arg);
//void* recv_msg(void* arg);
//void move_cursor(void);

char msg[BUF_SIZE];
//int cursor = LINE_SIZE;

int main()
{
	int str_len = -1;
	int serv_sock, clnt_sock;

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	struct epoll_event *ep_evnts;
	struct epoll_event evnt;
	struct udata *user_data;

	int ep_fd, evnt_cnt, i;

//	pthread_t snd_thread, rcv_thread;
//	void* thread_rtn;

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

	if(str_len = listen(serv_sock, EPOLL_SIZE)) {
		perror("listen");
		close(serv_sock);
	}

	ep_fd = epoll_create(EPOLL_SIZE);
	ep_evnts = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

	evnt.events = EPOLLIN;
	evnt.data.fd = serv_sock;
	epoll_ctl(ep_fd, EPOLL_CTL_ADD, serv_sock, &evnt);

	printf("\033[2J");

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
				user_fds[clnt_sock] = 1;
				user_data = malloc(sizeof(user_data));
				user_data->fd = clnt_sock;
				sprintf(user_data->name, "user(%d)", clnt_sock);
				
                evnt.events = EPOLLIN;
				evnt.data.ptr = user_data;
//              evnt.data.fd = clnt_sock; 

                epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &evnt);
//              printf("%c[%d;%df", 0x1B, ((cursor++) % LINE_SIZE) + 1, 25);
                printf("new client connected : %d \n", clnt_sock);
                
//              pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
//              pthread_create(&rcv_thread, NULL, recv_msg, (void*)&clnt_sock);
//              pthread_join(snd_thread, &thread_rtn);
//              pthread_join(rcv_thread, &thread_rtn);
            } else {
				user_data = ep_evnts[i].data.ptr;
				str_len = read(user_data->fd, msg, BUF_SIZE);
//              str_len = read(ep_evnts[i].data.fd, msg, BUF_SIZE);
//              if(str_len == 0) {
				if(str_len <= 0) {
                    epoll_ctl(ep_fd, EPOLL_CTL_DEL, user_data->fd, NULL);
					close(user_data->fd);
                    printf("client closed : %d \n", user_data->fd);
					user_fds[user_data->fd] = -1;
					free(user_data);
//                  epoll_ctl(ep_fd, EPOLL_CTL_DEL, ep_evnts[i].data.fd, NULL);
//                  close(ep_evnts[i].data.fd);
//                  printf("%c[%d;%df", 0x1B, ((cursor) % LINE_SIZE) + 1, 0);
//                  printf("client closed : %d \n", ep_evnts[i].data.fd);
                } else {
					send_msg(ep_evnts[i], msg);
//                  pthread_detach(snd_thread);
//                  pthread_detach(rcv_thread);
//					write(ep_evnts[i].data.fd, msg, str_len);
                }
            }
		}
	}
	close(serv_sock);
	close(ep_fd);
	return 0;
}

// 모든 클라이언트에 메시지 전송
void send_msg(struct epoll_event evnt, char *msg)
{
	int i;
	char buf[BUF_SIZE + 24];
	struct udata *user_data;
	user_data = evnt.data.ptr;
	for(i = 0; i < 8; i++) {
//		memset(buf, 0x00, BUF_SIZE + 24);
		sprintf(buf, "%s %s", user_data->name, msg);
		if((user_fds[i] == 1)) {
			write(i, buf, strlen(buf) + 1);
			printf("%s", buf);
		}
	}
}
//void* send_msg(void* arg)
//{
//	int clnt_sock = *((int*)arg);
//
//	while(1)
//	{
//		printf("%c[%d;%df", 0x1B, IN_CURSOR, 0);
//		fgets(msg, BUF_SIZE, stdin);
//
//		if(!strcmp(msg, "q\n"))
//		{
//			write(clnt_sock, msg, strlen(msg));
//			close(clnt_sock);
//			exit(0);
//		}
//
//		printf("%c[%d;%df", 0x1B, ((cursor) % LINE_SIZE) + 1, 25);
//		printf("server: %s", msg);
//		write(clnt_sock, msg, strlen(msg));
//		move_cursor();
//		printf("%c[%d;%df", 0x1B, IN_CURSOR, 0);
//	}
//	
//	return NULL;
//}

//void* recv_msg(void* arg)
//{
//	int clnt_sock = *((int*)arg);
//	int rcv_len;

//	while(1)
//	{
//		rcv_len = read(clnt_sock, msg, BUF_SIZE - 1);
//
//		if(rcv_len == -1) {
//			perror("receive");
//			return (void*) -1;
//		} 
//		
//		if(!strcmp(msg, "q\n")) {
//			printf("client %d is gone\n", clnt_sock);
//			close(clnt_sock);
//			return 0;	
//		} 
//		
//		if(rcv_len > 0) {
//			msg[rcv_len] = 0;
//			printf("%c[%d;%df", 0x1B, ((cursor) % LINE_SIZE) + 1, 0);
//			printf("client: %s", msg);
//			move_cursor();
//			int i = IN_CURSOR - ((cursor % LINE_SIZE) + 2);
//			for(i; i > 0; i--) {
//				printf("%s", "\n");
//			}
//			cursor++;
//		}
//	}
//	return NULL;
//}

//void move_cursor()
//{
//	int i = IN_CURSOR - ((cursor % LINE_SIZE) + 2);
//	for (i; i > 0; i--) {
//		printf("\n");
//	}
//	cursor++;
//}
