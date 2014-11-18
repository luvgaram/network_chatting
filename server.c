#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define BUF_SIZE 1024
#define PORT 3000
#define IN_CURSOR 22 // 입력 커서 위치
#define LINE_SIZE 20 // 채팅줄 수
#define EPOLL_SIZE 20 // epoll fd 수 
#define ERROR -1
#define ACTIVATED 1

struct udata {
	int fd;
	char name[80];
};

int user_fds[EPOLL_SIZE];
void send_msg(struct epoll_event evnt, char *msg);

char msg[BUF_SIZE];

int main()
{
	int str_len = ERROR;
	int serv_sock, clnt_sock, ep_fd, evnt_cnt, i;

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	struct epoll_event *ep_evnts;
	struct epoll_event evnt;
	struct udata *user_data;

	if ((serv_sock = socket (PF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("socket");
		return ERROR;
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
		evnt_cnt = epoll_wait(ep_fd, ep_evnts, EPOLL_SIZE, ERROR); // 이벤트 발생까지 무한 대기
		if (evnt_cnt == ERROR) {
			perror("epoll_wait");
			break;
		}

		for (i = 0; i < evnt_cnt; i++) {
            if (ep_evnts[i].data.fd == serv_sock) {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if (clnt_sock == ERROR) {
                    perror("accept");
                }
				user_fds[clnt_sock] = ACTIVATED;
				user_data = malloc(sizeof(user_data));
				user_data->fd = clnt_sock;
				sprintf(user_data->name, "user(%d)", clnt_sock);
				
                evnt.events = EPOLLIN;
				evnt.data.ptr = user_data;

                epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &evnt);
                printf("new client connected : %d\n", clnt_sock);
                
            } else {
				user_data = ep_evnts[i].data.ptr;
				str_len = read(user_data->fd, msg, BUF_SIZE);
				if (str_len <= 0) {
                    epoll_ctl(ep_fd, EPOLL_CTL_DEL, user_data->fd, NULL);
					close(user_data->fd);
                    printf("client closed : %d\n", user_data->fd);
					user_fds[user_data->fd] = ERROR;
					free(user_data);
                } else send_msg(ep_evnts[i], msg);
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
	for (i = 0; i < EPOLL_SIZE; i++) {
		sprintf(buf, "%s %s", user_data->name, msg);
		if ((user_fds[i] == ACTIVATED)) {
			write(i, buf, strlen(buf) + 1);
			printf("%s", buf);
		}
	}
}
