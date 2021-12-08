#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 256  
#define MAX_CLNT 2  //클라이언트 2명만 받음
#define NAME_SIZE 10 //이름 크기

char clnt_name[NAME_SIZE] = { 0 }; //클라이언트 이름
char clnt_names[MAX_CLNT][NAME_SIZE] = { 0 }; //클라이언트들의 이름 저장
int clnt_socks[MAX_CLNT]; //클라이언트 소켓 배열(2개)

pthread_mutex_t mutex;

//참고 : 윤성우의 열혈 TCP/IP 소켓 프로그래밍

int now_clnt = 0; //서버에 접속한 클라이언트 수

void send_msg(char* msg, int len); //클라이언트들에게 메세지 전송
void error(char* msg); //error처리
void* clnt_handling(void* arg); //클라이언트 처리 스레드 함수

int mani(int argc, char* argv[]) {
	
	struct sockaddr_in clnt_addr, serv_addr;
	int clnt_addr_size;			//클라 주소 크기
	int serv_sock, clnt_sock, i; //서버소켓, 클라소켓, 인덱스
	pthread_t tid; //쓰레드
	pthread_mutex_init(&mutex, NULL); //뮤텍스 초기화
	
	serv_sock = socket(PF_INET, SOCK_STREAM, 0); //IP , TCP

	if (argc < 2) { //포트 입력 안할시
		printf("Usage : %s <port>\n", argv[0]);
		return 0;
	}
												 
	//서버 구조체 설정
	memset(&serv_addr, 0, sizeof(serv_addr)); //서버주소 초기화
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
	serv_addr.sin_port = htons(atoi(argv[1]));  //입력한 port로 서버 주소 설정

	//소켓 바인딩 실패시 에러처리
	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error("bind() error");

	//접속 요청 실패시 에러처리
	if (listen(serv_sock, 5) == -1) //백로그 큐는 5로 설정, 연결 대기 제한수 
		error("listen() error");

	while (1) {
		clnt_addr_size = sizeof(clnt_addr); //클라이언트 주소 크기 설정
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size); //클라소켓 접속 요청 받음
		
		if (now_clnt >= MAX_CLNT) { //2명 넘어가면
			printf("%d 클라이언트 접속 실패\n", clnt_sock);
			write(clnt_sock, "user full", BUF_SIZE); //해당 클라이언에게 꽉 찼다고 신호 보냄
			continue;
		}
		//사용자가 동시에 접속할 때 클라이언트 배열에 같이 들어가는거 방지 - 뮤텍스 이용
		pthread_mutex_lock(&mutex);  //뮤텍스 락
		clnt_socks[now_clnt] = clnt_sock;  //클라이언트 를 차례대로 배열에 넣어줌
		read(clnt_sock, clnt_name, NAME_SIZE); //클라이언트 이름 배정	
		strcpy(clnt_names[now_clnt], clnt_name); //클라이언트 이름들 배열에 저장
		pthread_mutex_unlock(&mutex); // 뮤텍스 락 해제

		pthread_create(&tid, NULL, clnt_handling, (void*)&clnt_sock); //클라이언트 핸들링 스레드 생성
		pthread_detach(tid); // 스레드 종료시 자원 해제

		printf("%s >>>>>>> Connected \n", inet_ntoa(clnt_addr.sin_addr)); //연결된 클라이언트 IP출력
	}

	close(serv_sock);
	
	return 0;
}

void* clnt_handling(void* arg) {

	int clnt_sock =*((int*)arg);
	int str_len = 0;
	int i;

	int file_size = 0; //파일 크기 


	//int signal = 0; // 0 -> Nothing , 1 -> send file clnt -> serv, 2 -> send finish clnt -> serv

	//클라 소켓에서 신호를 받아 처리하기 위해
	char sig_sendfile[BUF_SIZE] = { "send file(c->s)" }; //클라이언트에서 파일전송시 신호
	char sig_finisih[BUF_SIZE] = { "finish(c->s)" }; // 클라이언트에서 파일전송 완료시 신호
	
	char msg[BUF_SIZE] = { NULL }; //메세지 담을 버퍼
	char file[BUF_SIZE] = { NULL }; //파일 담을 버퍼

	while ((str_len = read(clnt_sock, msg, BUF_SIZE) != 0))//클라소켓에서 신호를 읽어옴
	{ 

		if (!strcmp(msg, sig_sendfile)) 
		{
			int j; //클라이언트 인데스
			int exist = 0; //클라이언트 존재하는지 체크 0 -> O, 1 -> X
			int dest = NULL;  //전송할 소켓 인덱스
			char cname[NAME_SIZE] = { NULL }; //클라이언트 이름
			
			read(clnt_sock, cname, NAME_SIZE);

			pthread_mutex_lock(&mutex); //동시에 못보내게

			for (j = 0; j < now_clnt; j++) { //클라 이름 존재하는지 체크
				if (!strcmp(cname, clnt_names[j])) {
					exist = 1;  //존재함
					dest = j;
					break;
				}
				else if (j == now_clnt - 1) {
					exist = 0; //존재하지 않음
					break;
				}
			}

			if (exist == 0) { //존재하지 않을 때
				write(clnt_sock, "No user", BUF_SIZE);   //클라이언트에게 NO user 신호 보냄
				pthread_mutex_unlock(&mutex);
				continue;
			}
			else if (exist == 1) { //존재하면 
				write(clnt_sock, "On going", BUF_SIZE);  //클라이언트에게 On going 신호 보냄
			}


			write(clnt_socks[dest], "send file(s->c)", BUF_SIZE); //클라이언트에게 send 신호 보냄
			read(clnt_sock, &file_size, sizeof(int)); //파일 사이즈 받음
			write(clnt_sock[dest], &file_size, sizeof(int)); //파일 크기정보 전송

			while (1) {
				read(clnt_sock, file, BUF_SIZE);
				if (!strcmp(file, sig_finish))  //파일 끝일때까지
					break;
				
				write(clnt_socks[dest], file, BUF_SIZE);
			}
			write(clnt_socks[dest], "finish(s->c)", BUF_SIZE); //클라이언트에게 파일 전송완료 신호 보냄
			pthread_mutex_unlock(&mutex);
			printf("파일 전송 완료\n");

		}else 
		{
			printf("메세지 전송됨\n");
			send_msg(msg, str_len);
		}
	}

	//접속 해제된 클라이언트 처리하기 위해
	pthread_mutex_lock(&mutex);
	for (i = 0; i < now_clnt; i++)
	{
		if (clnt_sock == clnt_socks[i]) {
			while (i++ < now_clnt - 1) {
				clnt_socks[i] = clnt_socks[i + 1];		//클라이언트 소켓 한칸 땡김
				strcpy(clnt_names[i], clnt_names[i + 1]);		//이름도 한칸 땡김
			}
			break;
		}
	}

	now_clnt--;
	pthread_mutex_unlock(&mutex);

	close(clnt_sock);
	return NULL;
}



void send_msg(char* msg, int len) { 
	int i;
	pthread_mutex_lock(&mutex);
	for (i = 0; i < now_clnt; i++) {
		write(clnt_socks[i], msg, BUF_SIZE);
	}
	pthread_mutex_unlock(&mutex);
}

void error(char* msg) {
	fputs(msg, stderr);
	fputs('\n', stderr);
	exit(1);
}
