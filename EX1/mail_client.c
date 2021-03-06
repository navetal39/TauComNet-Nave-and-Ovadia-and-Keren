/*Includes*/
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

/*Constants*/
#define TOTAL_TO 20
#define MAXMAILS 32000
#define MAX_USERNAME 50
#define MAX_PASSWORD 30
#define MAX_SUBJECT 100
#define MAX_CONTENT 2000
#define NUM_OF_CLIENTS 20
#define WELCOME_LENGTH 32
#define DEFAULT_PORT 6423
#define INBOX_SIZE (9+MAX_USERNAME+MAX_SUBJECT)*MAXMAILS
#define MAIL_SIZE 28+(MAX_USERNAME*(TOTAL_TO+1))+MAX_SUBJECT+MAX_CONTENT

/*Macros and other general functions*/
void handleError(char *m) { printf("an error occured during %s\n", m);}
#define sendMgetOK(message) sendRet = sendall(sockDes, (message), strlen(message)+1); validate(sendRet, "getOK 1"); recvRet = recvall(sockDes, ok); validate(recvRet, "getOK 2"); //TODO handle? x2
#define validate(var, m) if((var)==-1){handleError(m);}

/* Slightly modified code from recitation 2's slides */
int sendall(int sd, char* buf, int len)
{
	int total = 0;
	int n = -1, m = 0, bytesleft = len;

	char strnum[11], retnum[11]; // 10 characters (+null terminator) is enough to hold int32's max value. It SHOULD be safe enough...
	memset(strnum, 0, 11);
	memset(retnum, 0, 11);
	sprintf(strnum, "%d", len);
	m = send(sd, strnum, 11, 0) - 11;
	validate(-(!(!m)), "data sending") else {
		m = recv(sd, retnum, 11, 0);
		validate(m, "data sending") else {
			validate(-(!(!strcmp(retnum, strnum))), "data sending") else {
				while (total < len)
				{
					n = send(sd, buf + total, bytesleft, 0);
					validate(n, "data sending") else {
						total += n;
						bytesleft -= n;
					}
				}
			}
		}
	}
	return n == -1 ? -1 : 0;
}

int recvall(int sd, char* buf)
{
	char strnum[11];
	memset(strnum, 0, 11);
	int m, m2, n = -1, len, total = 0;
	m = recv(sd, strnum, 11, 0) - 11;
	validate(m, "data receiving") else {
		len = atoi(strnum);
		m2 = send(sd, strnum, m + 11, 0) - (m + 11);
		validate(-(!(!m2)), "data receiving") else {
			while (total < len)
			{
				n = recv(sd, buf + total, len - total, 0);
				validate(n, "data receiving") else {
					total += n;
				}
			}
		}
	}
	return n == -1 ? -1 : 0;
}

int startsWith(char str[], char prefix[])
{
	int r, len = strlen(prefix);
	char head[18];		// because we only send clientReq to this method.
	memcpy(head, str, len);
	head[len + 1] = '\0';
	r = strcmp(head, prefix) == 0 ? 0 : 1;
	return r;
}

int readField(const char field[])
{
	int index, r = 0, len = strlen(field);
	for (index = 0; index < len; index++)
	{
		if (getchar() != field[index])
		{
			r = -1;
		}
	}
	return r;
}

void readInto(char buff[], int size)
{
	char* r = fgets(buff, size, stdin);
	validate(-(!r), "readInto")else {
		int len = strlen(buff);
		if (*buff && buff[len - 1] == '\n')
			buff[len - 1] = '\0';
	}
}

int main(int argc, char* argv[])
{
	char welcomeMessage[WELCOME_LENGTH], username[MAX_USERNAME + 1], password[MAX_PASSWORD + 1], inbox[INBOX_SIZE + 1];
	char recps[TOTAL_TO * (MAX_USERNAME + 1)], subj[MAX_SUBJECT + 1], ctnt[MAX_CONTENT + 1], inMail[MAIL_SIZE + 1];
	char connected[2], clientReq[18], reqNum[3], nmclReq[8], ok[3];
	int sockDes, recvRet, sendRet, success, numIndex, ar, isGet, portNum;
	/* Initialize address struct: */
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	if (argc > 1) // got ip addr
	{
		inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);
	} else {
		inet_aton("127.0.0.1", &serverAddr.sin_addr);
	}
	if (argc > 2) // got port number
	{
		portNum = htons(atoi(argv[2]));
	} else {
		portNum = htons(DEFAULT_PORT);
	}
	serverAddr.sin_port = portNum;
	/* Socket initialization */
	sockDes = socket(PF_INET, SOCK_STREAM, 0);
	validate(sockDes, "socket creation");


	success = connect(sockDes, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	validate(success, "connection");
	recvRet = recvall(sockDes, welcomeMessage);
	validate(recvRet, "connection"); //TODO handle?
	printf("%s\n", welcomeMessage);

	/* Authentication */
	readField("User: ");
	readInto(username, MAX_USERNAME);
	readField("Password: ");
	readInto(password, MAX_PASSWORD);
	sendMgetOK(username);
	sendRet = sendall(sockDes, password, strlen(password) + 1); //TODO handle?
	validate(sendRet, "data sending"); //TODO handle?
	recvRet = recvall(sockDes, connected);
	validate(recvRet, "data receiving"); //TODO handle?
	if (connected[0] != 'Y')
	{
		printf("Could not connect to the server.\nUsername and password combination is incorrect\n");//Error message on incorrect User/Pass combination
		success = close(sockDes);
		validate(success, "socket closing");
		return 0;
	} else {
		printf("Connected to server\n");
		do
		{
			readInto(clientReq, 18);
			if (strcmp(clientReq, "SHOW_INBOX") == 0) // Show request
			{
				memset(inbox, 0, INBOX_SIZE + 1);
				sendRet = sendall(sockDes, "1", 2);
				validate(sendRet, "data sending"); //TODO handle?
				recvRet = recvall(sockDes, inbox);
				validate(recvRet, "data receiving"); //TODO handle?
				if (strcmp(inbox, "EMPTY"))
					printf("%s\n", inbox);
			}
			else if (strcmp(clientReq, "QUIT") == 0) // Quit request
			{
				sendRet = sendall(sockDes, "4", 2);
				validate(sendRet, "data sending"); //TODO handle?
			}
			else if (strcmp(clientReq, "COMPOSE") == 0) // Compose request
			{
				success = readField("To: ");
				validate(success, "input reading");
				readInto(recps, TOTAL_TO * (MAX_USERNAME + 1));
				success = readField("Subject: ");
				validate(success, "input reading");
				readInto(subj, MAX_SUBJECT + 1);
				success = readField("Text: ");
				validate(success, "input reading");
				readInto(ctnt, MAX_CONTENT + 1);
				memset(ok, '\0', 3);
				sendMgetOK("5");
				sendMgetOK(recps);
				sendMgetOK(subj);
				sendMgetOK(ctnt);
				printf("Mail sent\n");
			}
			else if (startsWith(clientReq, "GET_MAIL") || startsWith(clientReq, "DELETE_MAIL"))
			{
				if (clientReq[0] == 'G') // Get request
				{
					reqNum[0] = '2'; reqNum[1] = ' '; reqNum[2] = '\0';
					numIndex = 9;
					ar = 1;
					isGet = 1;
				}
				else if (clientReq[0] == 'D') // Delete request
				{
					reqNum[0] = '3'; reqNum[1] = ' '; reqNum[2] = '\0';
					numIndex = 12;
					ar = 1;
					isGet = 0;
				}
				else
				{
					handleError("operation loop");
					ar = 0;
					isGet = 0;
				}
				if (ar)
				{
					memset(nmclReq, '\0', 8);
					strcat(nmclReq, reqNum);
					strcat(nmclReq, &clientReq[numIndex]); //TODO check if this work
					sendRet = sendall(sockDes, nmclReq, 8);
					validate(sendRet, "data sending"); //TODO handle?
					recvRet = recvall(sockDes, inMail);
					validate(recvRet, "data receiving"); //TODO handle?
					if (isGet)
					{
						printf("%s\n", inMail);
					}
				}
			}
			else
			{
				handleError("operation loop");
			}
		} while (strcmp(clientReq, "QUIT"));
		success = close(sockDes);
		validate(success, "socket closing");
	}
	return 0;
}
