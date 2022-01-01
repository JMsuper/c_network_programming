#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void error_handling(char *message);

int main(int argc, char* argv[])
{
    int sock, html_file;
    int str_len, recv_len;
    struct sockaddr_in serv_addr;
    char recv_buf[100000];
    int remain_buf_len = 100000;
    char request_msg[300] = "GET /webhp HTTP/1.1\r\nUser-Agent: Mozilla/4.0\r\ncontent-type:text/html\r\nConnection: close\r\n\r\n";
    
    if(argc!=3){
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    
    sock=socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handling("socket() error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));
    
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error!");
    
    str_len=write(sock, request_msg, sizeof(request_msg)-1);
    if(str_len == -1)
        error_handling("write() error!");
    
    str_len=0;
    do{
        recv_len=read(sock, &recv_buf[str_len], remain_buf_len);
        if(recv_len==-1){
            error_handling("read() error!");
        }
        str_len+=recv_len;
        remain_buf_len-=recv_len;
    }
    while(recv_len != 0 && remain_buf_len > 0);
    
    html_file=open("hw1.html",O_CREAT|O_RDWR|O_TRUNC);
    if(html_file == -1)
        error_handling("file open() error!");
    if(write(html_file, recv_buf, sizeof(recv_buf)) == -1)
        error_handling("file write() error!");
    
    close(html_file);
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
