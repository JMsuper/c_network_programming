#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <sys/socket.h>

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock, str_len;
	struct sockaddr_in serv_adr;
	
	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	// PF_INET : IPv4 protocol
	// SOCK_STREAM : TCP protocol
	sock=socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");
		
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect() error!");

	str_len = write(sock, "123",strlen("123"));
	if(str_len == -1)
        	error_handling("write() error!");
        sleep(1);
	send(sock,"4",strlen("4"),MSG_OOB);
	sleep(1);
	write(sock,"567",strlen("567"));
	sleep(1);
	send(sock,"890",strlen("890"),MSG_OOB);
	
	close(sock);
	return 0;
	
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
