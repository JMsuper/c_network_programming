#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define RES_HEADER "%s %s \r\nContent-Type: %s \r\nContent-Length: %d\r\n\r\n"

char* create_content_type(char* uri);
void http_hanlder(int clnt_sock);
void error_handling(char *message);
void read_childproc(int sig);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	pid_t pid;
	struct sigaction act;
	socklen_t adr_sz;
	
	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	act.sa_handler=read_childproc;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	sigaction(SIGCHLD, &act, 0);
	
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock==-1)
		error_handling("socket() error");
		
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_sock, (struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	
	while(1)
	{
		adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
		if(clnt_sock==-1)
			continue;
		else
			puts("new clinet connected...");
			
		pid=fork();
		
		if(pid==0)
		{		
			close(serv_sock);
			http_hanlder(clnt_sock);
			close(clnt_sock);
			puts("client disconnected...");
			return 0;
		}else
			close(clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void http_hanlder(int clnt_sock){
	char buf[512];
	char http_request[1024];
	int str_len;
	char method[10], uri[30], protocol[10];
	char *status, *content_type;
	int content_length;
	
	while(read(clnt_sock, buf, sizeof(buf))!=0)
	{
		strcat(http_request,buf);
		
		// Confirm that client's request has arrived
		if(strstr(http_request,"\r\n\r\n") != 0){
		
			// The file path must start with "."
			char file_path[30] = ".";
			char res_header[300];
			char* buffer;
			int fread_cnt;
			FILE* file;
			
			strcpy(method,strtok(http_request," "));
			strcpy(uri,strtok(0," "));
			strcpy(protocol,strtok(0,"\r\n"));
			
			// If "/", convert to "/index.html"
			if(strcmp(uri,"/") == 0){
				strcpy(uri,"/index.html");
			}
			// If 'uri' don't have extention, deal with an error
			if(strstr(uri,".") == 0){
				strcat(uri,".");
			}
			
			strcat(file_path,uri);
			
			if(file = fopen(file_path,"rb")){
				status = "200 OK";
				content_type = create_content_type(uri);
			}else{
				file = fopen("./error.html","rb");
				status = "404 Not Found";
				content_type = "text/html";
			}
			
			// Calculate the length of the file
			fseek(file, 0, SEEK_END);
			content_length = ftell(file);
			rewind(file);
			
			if((buffer = (char*)malloc(sizeof(char) * content_length)) == 0){
				error_handling("Memory error");
			}
				
			sprintf(res_header,RES_HEADER,protocol,status,content_type,content_length);
			write(clnt_sock,res_header,strlen(res_header));

			while((fread_cnt = fread(buffer,1,content_length,file))>0)
				send(clnt_sock,(void *)(buffer),fread_cnt,0);
				
			http_request[0] = '\0';
			free(buffer);
			fclose(file);
		}
	}
}


char* create_content_type(char* uri){
	strtok(uri,".");
	char* f_extension = strtok(0,"\0");
	if(strcmp(f_extension,"html") == 0){
		return "text/html";
	}else if(strcmp(f_extension,"jpg") == 0){
		return "image/jpg";
	}else if(strcmp(f_extension,"png") == 0){
		return "image/png";
	}else{
		error_handling("file extension is not found!");
	}
}

void read_childproc(int sig)
{
	pid_t pid;
	int status;
	pid=waitpid(-1, &status, WNOHANG);
	printf("removed proc id: %d \n", pid);
}
void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}





