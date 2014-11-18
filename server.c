#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>

#define BUF_SIZE 1024
#define PORT 3000
#define IN_CURSOR 22 // 입력 커서 위치
#define LINE_SIZE 20 // 채팅줄 수
#define EPOLL_SIZE 20 // epoll fd 수 
#define ACTIVATED 1

struct udata {
	int fd;
};

int user_fds[EPOLL_SIZE];
void send_msg(int fd, int clnt_max, char *msg);
int check_clnt_in(int clnt, int clnt_max);
int check_clnt_out(int clnt, int clnt_max);

char msg[BUF_SIZE];

int main()
{
	int serv_sock, clnt_sock, ep_fd, evnt_cnt, flag, i;
	int str_len = -1;
	int clnt_max = -1;

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	struct epoll_event *ep_evnts;
	struct epoll_event evnt;
	struct udata *user_data;

	if ((serv_sock = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(PORT);

	if (str_len = bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))) {
		perror("bind");
		close(serv_sock);
		exit(EXIT_FAILURE);
	}

	if (str_len = listen(serv_sock, EPOLL_SIZE)) {
		perror("listen");
		close(serv_sock);
		exit(EXIT_FAILURE);
	}

	ep_fd = epoll_create(EPOLL_SIZE);
	ep_evnts = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

	evnt.events = EPOLLIN;
	evnt.data.fd = serv_sock;
	epoll_ctl(ep_fd, EPOLL_CTL_ADD, serv_sock, &evnt);

	printf("\033[2J"); // 화면 clear

	while (1) {		
		evnt_cnt = epoll_wait(ep_fd, ep_evnts, EPOLL_SIZE, -1); // 이벤트 발생까지 무한 대기
		if (evnt_cnt == -1) {
			perror("epoll_wait");
			break;
		}

		for (i = 0; i < evnt_cnt; i++) {
            if (ep_evnts[i].data.fd == serv_sock) {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if (clnt_sock == -1) {
                    perror("accept");
                }

				// 가장 큰 클라 소켓 값 저장
				clnt_max = check_clnt_in(clnt_sock, clnt_max);

				//nonblocking
				flag = fcntl(clnt_sock, F_GETFL, 0);
				fcntl(clnt_sock, F_SETFL, flag | O_NONBLOCK);

				user_fds[clnt_sock] = ACTIVATED;
				user_data = malloc(sizeof(user_data));
				user_data->fd = clnt_sock;

                evnt.events = EPOLLIN;
				evnt.data.ptr = user_data;

                epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &evnt);
                printf("new client connected : %d\n", clnt_sock);
                
            } else {
				user_data = ep_evnts[i].data.ptr;
				str_len = read(user_data->fd, msg, BUF_SIZE);
				if (str_len <= 0) {
					if (errno == EAGAIN) {
						printf("nonblocking");
					} else {
               			epoll_ctl(ep_fd, EPOLL_CTL_DEL, user_data->fd, NULL);
	               		printf("client closed : %d\n", user_data->fd);
						clnt_max = check_clnt_out(user_data->fd, clnt_max);
						close(user_data->fd);
						user_fds[user_data->fd] = 0;
						free(user_data);
                	}
				} else
					send_msg(user_data->fd, clnt_max, msg);
            }
		}
	}
	close(serv_sock);
	close(ep_fd);
	return 0;
}

// 모든 클라이언트에 메시지 전송
void send_msg(int fd, int clnt_max, char *msg)
{
	int i;
	char buf[BUF_SIZE + 24];
	for (i = 0; i <= clnt_max; i++) {
		if ((user_fds[i] == ACTIVATED)) {
			sprintf(buf, "user %d: %s", fd, msg);
			write(i, buf, strlen(buf) + 1);
		}
	}
}

int check_clnt_in(int clnt, int clnt_max)
{
	if (clnt > clnt_max)
		return clnt;
	return clnt_max;
}

int check_clnt_out(int clnt, int clnt_max)
{
	if (clnt == clnt_max) {
		clnt = clnt - 1;
		for ( ; clnt >= 0; clnt--) {
			if (user_fds[clnt] != 0)
				return clnt;
		}
		return -1;
	}
	return clnt_max;
}
