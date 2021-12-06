#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_SIZE 30

void error_handling(char *message);
void urg_handler(int signo);
int accept_sock,recv_sock;

int main(int argc, char *argv[])
{

	struct sockaddr_in serv_adr, clnt_adr;
	struct sigaction act;
	socklen_t adr_sz;
	int str_len;
	char buf[BUF_SIZE];
	
	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	act.sa_handler=urg_handler;
	// act.sa_mask : set of signal that is blocked when dealing with signal 
	// sigemptyset : delete set signal
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	
	accept_sock = socket(PF_INET,SOCK_STREAM,0);
	if(accept_sock==-1)
		error_handling("socket() error");
		
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(accept_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(accept_sock,5) == -1)
		error_handling("listen() error");
	
	recv_sock = accept(accept_sock, (struct sockaddr*)&clnt_adr, &adr_sz);	
	
	// fcntl : controls the properties of an already open file
	// F_SETOWN :  command that set process receiving SIGIO, SIGURG
	fcntl(recv_sock,F_SETOWN,getpid());
	sigaction(SIGURG, &act, 0);

	while((str_len=recv(recv_sock, buf, sizeof(buf),0))!= 0)
	{
		if(str_len==-1){
			fprintf(stderr,"Recv Error: %s\n", strerror(errno));
			return -1;		
		}
		buf[str_len]=0;
		printf("%s\n",buf);
	}
	close(accept_sock);
	close(recv_sock);
	return 0;
}

// If urg_handler does not call return,
// urg_handler wouldn't be implemented properly.
void urg_handler(int signo)
{
	int str_len;
	char buf[BUF_SIZE];
	str_len = recv(recv_sock,buf,sizeof(buf)-1,MSG_OOB);
	buf[str_len]=0;
	printf("Urgent message: %s \n", buf);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
