#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 256
#define NAME_SIZE 10

	
void * send(void * arg);
void * recv(void * arg);
void error(char * msg);
void menu();

char name[NAME_SIZE]= {NULL}; //자신의 이름
char msg[BUF_SIZE] = {NULL};

int other = 0;  //-1 일시 상대 존재 X
int recvName = 0;
int canWrite = 1;
	

pthread_mutex_t mutex;


int main(int argc, char *argv[])
{
	if (argc < 4) {
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		return 0;
	}

	sprintf(name, "%s", argv[3]); //name 받아옴

	struct sockaddr_in serv_addr;  //서버 구조체
	int sock = socket(PF_INET, SOCK_STREAM, 0); //IP , TCP

	pthread_t t_send , t_recv; //send실행 스레드와 recv실행 스레드;
	void * thread_return;

	pthread_mutex_init(&mutx, NULL);
		
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);	//argv[1] 서버 IP
	serv_addr.sin_port=htons(atoi(argv[2]));	//argv[2] 서버 port
	  
	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	{
		error_handling("connect() error");
	}else{
		write(sock, name, NAME_SIZE);  // 이름 서버로 보냄
		printf("\n서버에 연결되었습니다.\n");
	}

	pthread_create(&t_send, NULL, send, (void*)&sock); //send 실행 스레드 생성
	pthread_create(&t_recv, NULL, recv, (void*)&sock);	//recv 실행 스레드 생성
	pthread_join(snd_thread, &thread_return);		
	pthread_join(rcv_thread, &thread_return);		//둘이 묶어서 관리
	
	close(sock);  
	return 0;
}
void * send(void * sock)   // send 스레드 함수
{
	int sock=*((int*)sock);
	int file_size = 0;

	char name_msg[NAME_SIZE+BUF_SIZE] = {NULL};
	char text[BUF_SIZE] = {NULL};    
	char chat_log[BUF_SIZE] = {NULL};   


	while(1) 
	{
		if(canWrite == 0) { //쓸수 있는 상태X
			sleep(1);
		}

		menu();

		fgets(msg, BUF_SIZE, stdin);

		else if(!strcmp(msg,"0\n")) 
		{
			close(sock);
			exit(0);
		}
		else if (!strcmp(msg, "1\n"))
		{

			FILE* f;

			char myfile[BUF_SIZE];  //보낼 파일 경로
			char who[NAME_SIZE];
			

			printf("보낼 파일의 경로를 입력하세요 >> ");
			scanf("%s", myfile);
			
			
			if ((f = fopen(myfile, "rb") == NULL) {		//파일 존재여부 확인
				printf("파일이 존재하지 않습니다.\n");
				continue;
			}

			write(sock, "send file(c->s)", BUF_SIZE);  //서버에게 파일전송 신호 전달

			printf("(보낼 상대의 ID : ");
			scanf("%s", who);
			write(sock, who, NAME_SIZE);  //상대방 아이디를 전송

			while(other == 0){  //상대 있을 때 까지 sleep
				sleep(1);
			}
			if (other == -1 ) {	//클라이언트가 없음
				printf("유저가 존재하지 않습니다. \n");
				other = 0;
				continue;
			}

			//파일 크기 얻어냄
			fseek(f, 0, SEEK_END); 
			file_size = ftell(f);
			char *buff;

			printf("전송 시작 \n파일크기는 %d 입니다.\n", file_size);
			write(sock, &file_size, sizeof(int)); // 서버에게 파일크기 전송

			//파일 크기  + 1바이트 만큼 동적 메모리 할당 후 0으로 초기화
			buff = malloc(file_size + 1);
			memset(buff,0,file_size + 1);

			//파일 포인터를 파일의 처음으로 이동시킴
			fseek(f,0,SEEK_SET);
			fread(buff,file_size,1,f); //버퍼에 파일 내용 입력

			write(sock, buff, BUF_SIZE); //서버에 파일 내용 보냄
		
			fclose(f);
			free(buff);

			printf("파일 전송을 완료하였습니다. \n");
			other = 0;
		}
		else if(recvName == 1) { //받는 파일 이름 설정경우
			if(strcmp(msg, "\n")) {
				recvName = 0;
			}
		} 
		else  //메세지 보내기
		{
			strcpy(text, "\n");
			sprintf(chat_log,"[%s] %s", name, text);
			sprintf(name_msg,"[%s] %s", name, msg);

			if(strcmp(name_msg, chat_log) != 0)  //입력없을시 출력 안함
				write(sock, name_msg, BUF_SIZE);
		}

	}
	return NULL;
}
void * recv(void * arg)   // read thread main
{
	int sock=*((int*)arg);

	char name_msg[BUF_SIZE] = {NULL};
	char context[BUF_SIZE] = {NULL};

	char sig_recv[BUF_SIZE] = {"send file(s->c)"};  //서버에서 파일 보낼때 받는 신호
	char sig_finish[BUF_SIZE] = {"finish(s->c)"};		//서버에서 파일 전송 완료시 보내는 신호

	const char yescl_msg[BUF_SIZE] = {"[continue_ok_nowgo]"};
	const char noConnect[BUF_SIZE] = {"too many users. sorry"};
	
	
	int str_len = 0;
	int file_size = 0;

	while(1)
	{
		str_len=read(sock, name_msg, BUF_SIZE);

		
		if(!strcmp(name_msg, sig_recv)) {
			
			recvName = 1;

			canWrite = 0;  //쓰기모드 OFF

			printf("파일 수신 대기중");
			read(sock, &fils_size, sizeof(int));
			printf("파일크기는 %d 입니다.\n", file_size); //파일 크기 알려주고 받을지 물어봄
			printf("수신할 파일 이름을 설정해 주세요 >> ");
			
			
			canWrite = 1;  //쓰기모드 ON

			while(recvName == 1) {  //파일 이름 설정경우
				sleep(1);
			}


			msg[strlen(msg)-1] = '\0';
			
			FILE *f;
			f = fopen(msg, "wb"); 
		
			while(1)
			{		
				read(sock, context, BUF_SIZE);
				
				if(!strcmp(context, sig_finish)) 
					break;
				fwrite(context, 1, BUF_SIZE, f);
			}

			fclose(f);
			
			printf("파일 수신이 끝났습니다. \n");
			// ㄴ send_msg 쓰레드의 활동 재개


		}
		else if(strcmp(name_msg, "On going") == 0) { //서버로부터 진행 가능할때 받는신호

			other = 1;

		}
		else if(strcmp(name_msg, "No user") == 0) { //

			other = -1; 
		}
		else if(!strcmp(name_msg, "user full")) {
			printf("사용자가 가득찼습니다..\n");
			exit(0);
		}
		else {
			fputs(name_msg, stdout);
		}
	}
	return NULL;
}
void error(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
void menu() {
	printf("\n==============================\n");
	printf("1.파일 보내기\n");
	printf("0.프로그램 종료\n");
	printf("==============================\n");
	printf(" >> ");
}
