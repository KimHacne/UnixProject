#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 256  
#define MAX_CLNT 2  //Ŭ���̾�Ʈ 2�� ����
#define NAME_SIZE 10 //�̸� ũ��

char clnt_name[NAME_SIZE] = { 0 }; //Ŭ���̾�Ʈ �̸�
char clnt_names[MAX_CLNT][NAME_SIZE] = { 0 }; //Ŭ���̾�Ʈ���� �̸� ����
int clnt_socks[MAX_CLNT]; //Ŭ���̾�Ʈ ���� �迭(2��)

pthread_mutex_t mutex;

//���� : �������� ���� TCP/IP ���� ���α׷���

int now_clnt = 0; //������ ������ Ŭ���̾�Ʈ ��

void send(char* msg, int len); //Ŭ���̾�Ʈ�鿡�� �޼��� ����
void error(char* msg); //erroró��
void* clnt_handling(void* arg); //Ŭ���̾�Ʈ ó�� ������ �Լ�

int mani(int argc, char* argv[]) {
	
	struct sockaddr_in clnt_addr, serv_addr;
	int clnt_addr_size;			//Ŭ�� �ּ� ũ��
	int serv_sock, clnt_sock, i; //��������, Ŭ�����, �ε���
	pthread_t tid; //������
	pthread_mutex_init(&mutex, NULL); //���ؽ� �ʱ�ȭ
	
	serv_sock = socket(PF_INET, SOCK_STREAM, 0); //IP , TCP

	if (argc < 2) { //��Ʈ �Է� ���ҽ�
		printf("Usage : %s <port>\n", argv[0]);
		return 0;
	}
												 
	//���� ����ü ����
	memset(&serv_addr, 0, sizeof(serv_addr)); //�����ּ� �ʱ�ȭ
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
	serv_addr.sin_port = htons(atoi(argv[1]));  //�Է��� port�� ���� �ּ� ����

	//���� ���ε� ���н� ����ó��
	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error("bind() error");

	//���� ��û ���н� ����ó��
	if (listen(serv_sock, 5) == -1) //��α� ť�� 5�� ����, ���� ��� ���Ѽ� 
		error("listen() error");

	while (1) {
		clnt_addr_size = sizeof(clnt_addr); //Ŭ���̾�Ʈ �ּ� ũ�� ����
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size); //Ŭ����� ���� ��û ����
		
		if (now_clnt >= MAX_CLNT) { //2�� �Ѿ��
			printf("%d Ŭ���̾�Ʈ ���� ����\n", clnt_sock);
			write(clnt_sock, "����ڰ� ����á���ϴ�.", BUF_SIZE); //�ش� Ŭ���̾�Ʈ ������ ���ƹ���
			continue;
		}
		//����ڰ� ���ÿ� ������ �� Ŭ���̾�Ʈ �迭�� ���� ���°� ���� - ���ؽ� �̿�
		pthread_mutex_lock(&mutex);  //���ؽ� ��
		clnt_socks[now_clnt] = clnt_sock;  //Ŭ���̾�Ʈ �� ���ʴ�� �迭�� �־���
		read(clnt_sock, clnt_name, NAME); //Ŭ���̾�Ʈ �̸� ����	
		strcpy(clnt_names[now_clnt], clnt_name); //Ŭ���̾�Ʈ �̸��� �迭�� ����
		pthread_mutex_unlock(&mutex); // ���ؽ� �� ����

		pthread_create(&tid, NULL, clnt_handling, (void*)&clnt_sock); //Ŭ���̾�Ʈ �ڵ鸵 ������ ����
		pthread_detach(tid); // ������ ����� �ڿ� ����

		printf("%s >>>>>>> Connected \n", inet_ntoa(clnt_addr.sin_addr)); //����� Ŭ���̾�Ʈ IP���
	}

	close(serv_sock);
	
	return 0;
}






void* clnt_handling(void* arg) {

	int clnt_sock =*((int*)arg);
	int str_len = 0;
	int i;

	int file_size = 0; //���� ũ�� 


	//int signal = 0; // 0 -> Nothing , 1 -> send file clnt -> serv, 2 -> send finish clnt -> serv

	//Ŭ�� ���Ͽ��� ��ȣ�� �޾� ó���ϱ� ����
	char sig_sendfile[BUF_SIZE] = { "send file(c->s)" }; //Ŭ���̾�Ʈ���� �������۽� ��ȣ
	char sig_finisih[BUF_SIZE] = { "finish(c->s)" }; // Ŭ���̾�Ʈ���� �������� �Ϸ�� ��ȣ
	
	char msg[BUF_SIZE] = { NULL }; //�޼��� ���� ����
	char file[BUF_SIZE] = { NULL }; //���� ���� ����

	while ((str_len = read(clnt_sock, msg, BUF_SIZE) != 0) //Ŭ����Ͽ��� ��ȣ�� �о��
	{ 
		if (!strcmp(msg, sig_sendfile)) 
		{
			int j; //Ŭ���̾�Ʈ �ε���
			int exist = 0; //Ŭ���̾�Ʈ �����ϴ��� üũ 0 -> O, 1 -> X
			int dest = NULL;  //������ ���� �ε���
			char cname[NAME_SIZE] = { NULL }; //Ŭ���̾�Ʈ �̸�
			
			read(clnt_sock, cname, NAME_SIZE);

			pthread_mutex_lock(&mutex); //���ÿ� ��������

			for (j = 0; j < now_clnt; j++) { //Ŭ�� �̸� �����ϴ��� üũ
				if (!strcmp(cname, clnt_names[j])) {
					exist = 1;  //������
					dest = j;
					break;
				}
				else if (j == now_clnt - 1) {
					exist = 0; //�������� ����
					break;
				}
			}

			if (exist == 0) { //�������� ���� ��
				write(clnt_sock, "����ڰ� �������� �ʽ��ϴ�.\n", BUF_SIZE);
				pthread_mutex_unlock(&mutex);
				continue;
			}
			else if (exist == 1) { //�����ϸ� 
				write(clnt_sock, "�������� ����.....", BUF_SIZE);
			}


			write(clnt_socks[dest], "send file(s->c)", BUF_SIZE); //Ŭ���̾�Ʈ���� send ��ȣ ����
			read(clnt_sock, &file_size, sizeof(int)); //���� ������ ����
			write(clnt_sock[dest], &file_size, sizeof(int)); //���� ũ������ ����

			while (1) {
				read(clnt_sock, file, BUF_SIZE);
				if (!strcmp(file, sig_finish))  //���� ���϶�����
					break;
				
				write(clnt_socks[dest], file, BUF_SIZE);
			}

			write(clnt_socks[dest], "finish(s->c)", BUF_SIZE); //Ŭ���̾�Ʈ���� ���� ���ۿϷ� ��ȣ ����

			pthread_mutex_unlock(&mutex);

			printf("���� ���� �Ϸ�\n");


		}
		
		else {
			printf("�޼��� ���۵�\n");
			send_msg(msg, str_len);
		}
	}

	//���� ������ Ŭ���̾�Ʈ ó���ϱ� ����
	pthread_mutex_lock(&mutex);
	for (i = 0; i < now_clnt; i++)
	{
		if (clnt_sock == clnt_socks[i]) {
			while (i++ < now_clnt - 1) {
				clnt_socks[i] = clnt_socks[i + 1];		//Ŭ���̾�Ʈ ���� ��ĭ ����
				strcpy(clnt_names[i], clnt_names[i + 1]);		//�̸��� ��ĭ ����
			}
			break;
		}
	}
	now_clnt--;
	pthread_mutex_unlock(&mutex);

	close(clnt_sock);
	return NULL;

}
void send(char* msg, int len) { 
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