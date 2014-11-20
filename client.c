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
#define IP "127.0.0.1"
#define PORT 3000
#define IN_CURSOR 30 // 입력 커서 위치
#define LINE_SIZE 20 // 채팅줄 수
#define EPOLL_SIZE 2 // epoll fd 수, stdin & clnt_sock
#define STDIN 0

void get_and_send_msg(int clnt_sock);
void read_msg(int clnt_sock);
void move_cursor(void);

char msg[BUF_SIZE];
int cursor = LINE_SIZE;

int main()
{
	struct sockaddr_in serv_adr;
	struct epoll_event *ep_evnts;
	struct epoll_event evnt;
	int clnt_sock, read_tot, write_tot, ep_fd, evnt_cnt, fd_cnt, flag, rcv_len, i;

	if ((clnt_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}

	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(IP);
	serv_adr.sin_port = htons(PORT);

	if (connect(clnt_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		perror("connect");
		close(clnt_sock);
		exit(EXIT_FAILURE);
	}

	printf("\033[2J"); // 화면 clear
	printf("%c[%d;%df", 0x1B, ((cursor) % LINE_SIZE) + 1, 0);
	printf ("server connected\n");
	move_cursor();

	ep_fd = epoll_create(EPOLL_SIZE);
	ep_evnts = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

	evnt.events = EPOLLIN;
	evnt.data.fd = clnt_sock;

	if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, clnt_sock, &evnt) == -1) {
		perror("epoll_ctl_clnt_sock");
		exit(EXIT_FAILURE);
	}

	evnt.events = EPOLLIN | EPOLLET;
	evnt.data.fd = STDIN;

	if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, 0, &evnt) == -1){
		perror("epoll_ctli_stdin");
		exit(EXIT_FAILURE);
	}

	// nonblocking 
	flag = fcntl(clnt_sock, F_GETFL, 0);
	fcntl(clnt_sock, F_SETFL, flag | O_NONBLOCK);

	while(1) {
		if ((fd_cnt = epoll_wait(ep_fd, ep_evnts, EPOLL_SIZE, -1)) == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < fd_cnt; i++) {
			if (ep_evnts[i].data.fd == clnt_sock) {
				read_msg(clnt_sock);
			} else if (ep_evnts[i].data.fd == STDIN) {
				get_and_send_msg(clnt_sock);
			}
		}
	}

	close(clnt_sock);
	return 0;
}

void get_and_send_msg(int clnt_sock)
{
//	int clnt_sock = x*((int*)arg);

	fgets(msg, BUF_SIZE, stdin);
	msg[strlen(msg) + 1] = 0;
	
	if (!strcmp(msg, "q\n")) {
		close(clnt_sock);
		exit(0);
	}

	write(clnt_sock, msg, strlen(msg) + 1);
}

void read_msg(int clnt_sock)
{
//	int clnt_sock = *((int*)arg);
	int rcv_len;

	rcv_len = read(clnt_sock, msg, BUF_SIZE);

	if (rcv_len == -1) {
		if (errno != EAGAIN) {
			perror("receive");
			return;
		}
	}

	if (rcv_len > 0) {
		printf("%c[%d;%df", 0x1B, ((cursor) % LINE_SIZE) + 1, 0);

		if (msg[rcv_len - 1] == 0)
			printf("%s", msg);
		else fprintf(stderr, "invalid string");

		move_cursor();
	}
}

void move_cursor()
{
	int i = IN_CURSOR - ((cursor % LINE_SIZE) + 2);
	for( ; i > 0; i--) {
		printf("\n");
	}
	cursor++;
}
