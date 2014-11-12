#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>

#define BUF_SIZE 1024
#define PORT 3000
#define IN_CURSOR 30

void* send_msg (void* arg);
void* recv_msg (void* arg);
int kbhit (void);

char msg[BUF_SIZE];
int cursor = 20;

int main ()
{
	int str_len = -1;
	int serv_sock, clnt_sock;

	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

	pthread_t snd_thread, rcv_thread;
	void* thread_rtn;

	if ((serv_sock = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return str_len;
	}

	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = INADDR_ANY;
	serv_adr.sin_port = htons(PORT);

	if (str_len = bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))) {
		perror("bind");
		close(serv_sock);
	}

	if (str_len = listen(serv_sock, 1)) {
		perror("listen");
		close(serv_sock);
	}

	clnt_adr_sz = sizeof(clnt_adr);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

	if (clnt_sock == -1) {
		perror("accept");
		str_len = -1;
		close(serv_sock);
	}
	// 화면 지움
	printf("\033[2J");

	// 쓰레드 생성
	pthread_create(&snd_thread, NULL, send_msg, (void*)&clnt_sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&clnt_sock);
	pthread_join(snd_thread, &thread_rtn);
	pthread_join(rcv_thread, &thread_rtn);
	close(clnt_sock);
	close(serv_sock);

	return str_len;
}

int kbhit (void) {
	struct termios oldt, newt;
	int ch;
	int oldf;
	
	tcgetattr (STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr (STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl (STDIN_FILENO, F_GETFL, 0);
	fcntl (STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	// 터미널 설정 복원
	tcsetattr (STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
}

void* send_msg (void* arg)
{
	int clnt_sock = *((int*)arg);

	while (1)
	{
		printf("%c[%d;%df", 0x1B, IN_CURSOR, 0);
		fgets(msg, BUF_SIZE, stdin);

		if (!strcmp(msg, "q\n"))
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

void* recv_msg (void* arg)
{
	int clnt_sock = *((int*)arg);
	int rcv_len;

	while (1)
	{
//		printf("%c[%d;%df", 0x1B, IN_CURSOR, 0);
		rcv_len = read(clnt_sock, msg, BUF_SIZE - 1);

		if (rcv_len == -1) {
			perror("receive");
			return (void*) -1;
		} 
		
		if (!strcmp(msg, "q\n")) {
			printf("%s", "client is gone\n");
			close(clnt_sock);
			exit(0);
		} 
		
		if (rcv_len > 0) {
			msg[rcv_len] = 0;

			printf("%c[%d;%df", 0x1B, ((cursor++) % 20) + 1, 0);
			printf("client: %s", msg);
			int i = IN_CURSOR - ((cursor % 20) + 1);
			for (i; i > 0; i--) {
				printf("%s", "\n");
			}
		}
	}

	return NULL;
}
