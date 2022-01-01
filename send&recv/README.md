# 코드설명
'tcp_ip_book_sample_code'폴더에 있는 소스파일는 TCP/IP 교재의 첨부된 예제코드이며,<br>
폴더 밖에 있는 소스파일은 예제코드에서 발견한 에러의 원인을 확인하기 위한 코드입니다.
<br>

예제코드를 수행하며 교재에 있는 실행결과와 실제 실행결과가 다른 것을 발견하였습니다.

- 교재의 실행결과
```
123
Urgent message: 4
567
Urgent message: 0
89
```

- 실제 실행결과
```
123456789
```

문제의 원인을 찾기위해 기존에 MSG_OOB를 전송하는 oob_send.c의 소스코드를 수정하였습니다.
- 수정 전 소스코드
```
	write(sock, "123", strlen("123"));
	send(sock, "4", strlen("4"), MSG_OOB);
	write(sock, "567", strlen("567"));
	send(sock, "890", strlen("890"), MSG_OOB);
```
- 수정 후 소스코드(write와 send사이에 sleep(1)을 추가)
```
	write(sock, "123", strlen("123"));
	sleep(1);
	send(sock, "4", strlen("4"), MSG_OOB);
	sleep(1);
	write(sock, "567", strlen("567"));
	sleep(1);
	send(sock, "890", strlen("890"), MSG_OOB);
```

sleep(1)을 추가했을 때는 교재의 예제코드와 동일하게 작동하였습니다.<br>
이러한 원인은 TCP 소켓인것과 Local로 동작하는 것이 원인으로 판단되어집니다.<br>
TCP는 Stream 덩어리로 판단하므로 보내는 횟수에 따라 읽는 횟수가 동일하지 않습니다.<br>
그래서 전송시에 나눠 보내는것처럼 보이지만 Read buffer에 한번에 저장되어 있다가<br>
버퍼 사이즈 만큼 읽어오기 때문에 한번에 읽어온것임을 알 수 있습니다.<br>
이 때문에 sleep(1)을 추가했을 때는 수신되는 패킷의 시간 차이가 존재하게 되어<br>
각각의 송신데이터들을 나눠서 수신하고 있다고 판단하였습니다.<br>

### send_org에서 recv()는 signal handler 호출이후 interrupt system call로 인해 -1을 리턴
에러의 원인을 찾기위한 소스코드인 recv_URG에서 정상적으로 종료되지 않음을 발견하였습니다.
- 예제 코드
```
	while((str_len=recv(recv_sock, buf, sizeof(buf), 0))!= 0) 
	{
		if(str_len==-1)
			continue;
		buf[str_len]=0;
		puts(buf);
	}
```
- 수정한 필자의 코드
```
	while((str_len=recv(recv_sock, buf, sizeof(buf),0))!= 0)
	{
		if(str_len==-1){
			fprintf(stderr,"Recv Error: %s\n", strerror(errno));
			return -1;		
		}
		buf[str_len]=0;
		printf("%s\n",buf);
	}
```
recv()가 -1을 리턴한다면 error가 발생한 것으로 판단하여 에러메시지를 출력하고 프로세스를 종료하도록 코드를 수정하였습니다.<br>
이 때문에 MSG_OOB를 수신한 이후에 recv()는 -1을 리턴하고 `Interrupt system call`이 발생하였습니다.<br><br>
처음에는 `Interrupt`는 프로세스에 문제가 있다는 신호라고 잘못판단하였습니다.<br>
하지만, signal handler가 호출되기 위해서는 커널에서 `interrupt`를 발생시켜 실행중이던 프로세스를<br>
잠시 중지시키고 signal handler를 호출하는 것이였습니다.<br>
recv()에서 -1을 호출하기 까지의 과정은 다음과 같습니다.<br><br>
참고링크 : https://www.gnu.org/software/libc/manual/html_node/Out_002dof_002dBand-Data.html<br>
`소켓이 recv()에서 MSG_OOB 발견 -> 소켓이 속한 프로세스에 SIGURG 시그널을 전달(signal() 시스템콜) -> SIGURG를 핸들링하는 urg_handler 호출`<br>
`-> 기존 실행중이던 recv()로 돌아옴 -> interrupt가 발생한 직후이기 때문에 recv()는 -1을 리턴`<br>
<br>
### 결론
교재에 있던 예제코드는 잘못된 것이 아니였습니다.<br>
MSG_OOB는 interrupt를 발생시키기 때문에 recv()는 -1을 리턴한 것이며,<br>
로컬환경에서 해당 코드를 실행했기 때문에 recv()에서 한번에 데이터를 읽어들인 것이였습니다.<br>

### 알게된 것
`interrupt`는 에러상황뿐만 아니라 시스템콜이 호출될때에도 발생한다.<br>
해당 interrupt가 에러인지 시스템콜인지는 커널이 판단할 일이다.<br>
TCP는 send측에서 데이터를 보낸 횟수만큼 recv하지 않고 buffer에 만큼 recv한다.<br>
따라서 send횟수와 recv횟수는 일치하지 않을 수 있다.(특히 로컬환경에서는 더더욱!)<br>
