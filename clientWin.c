/* tcpclient.c
   This program implements a TCP client using socket programming.
   Use it as a sample program only.

   programmed by Glen Tian
              on 23 February 2018
              in Brisbane
   5 Sep 2021 Glen Tian:
		Modified in accordance the Linux version for book
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>     /* Note: winsock2.h has included windows.h */
#include <stdbool.h>

#define SERVER_IP_ADDR "127.0.0.1"  /* loopback for testing */
#define SERVPORT  8886 /* server port number */
#define BUFLEN    1024 /* buffer length */
#define QUITKEY   0x1b /* ASCII code of ESC */

#pragma comment(lib,"ws2_32.lib")

int main(void){
    WSADATA wsa_data;              /* defined in winsock2.h */
    SOCKET  sockfd;                /* defined in winsock2.h */
    struct  sockaddr_in serv_addr; /* defined in winsock2.h */
    struct  hostent  *host = NULL; /* defined in winsock2.h */
	char buffer[BUFLEN] = {0};
	char *ackmsg = "ACK from client";
	int sendStatus, i=0;
	bool stop = false;

	printf("======== TCP Client ========\n");

	/* Step 1: startup winsocket - this is for Windows only */
	/* in pair with WSACleanup()                    */
	if(WSAStartup(MAKEWORD(2,2), &wsa_data) != 0){
        printf("WSAStartup failed: %d\n",WSAGetLastError());
		exit(SOCKET_ERROR);
	}

	/* Step 2: Create socket and check it is successful */
	/* in pair with closesocket()               */
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0))==SOCKET_ERROR){
		printf("socket() failed: %d\n", WSAGetLastError());
		exit(INVALID_SOCKET);
	}

	/* Step 3: Connect to the TCP Server */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
    serv_addr.sin_port = htons(SERVPORT);

	printf("\nConnecting to Server....\n");
    if ((connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)))==SOCKET_ERROR){
        printf("connect() failed: %d\n", WSAGetLastError());
        exit(SOCKET_ERROR);
    }
	printf("....Connection established\n");

	/* Step 4: Send and receive data in loop */
	while (1){ /* loop for send()/recv() */
		/* receive message */
		if ((recv(sockfd,buffer,BUFLEN-1,0)) == SOCKET_ERROR){
			printf("recv() failed: %d\n", WSAGetLastError());
			break;
		}
		buffer[BUFLEN-1] = 0x00;  /* force ending with '\0' */
		if (buffer[0] == QUITKEY) // prepare termination
			break;
		printf("%2d Received: %s\n",i++, buffer);

		/* send message */
        sendStatus = send(sockfd, ackmsg, strlen(ackmsg), 0);
        if (sendStatus == 0)
            break;  /* nothing has been sent */
        if (sendStatus == SOCKET_ERROR){
			printf("send() failed: %d\n", WSAGetLastError());
            break;
        }
        printf("   Sent:     %s\n", ackmsg);
    }

	/* Step 5: Close socket, in pair with socket() */
	if ((closesocket(sockfd))== SOCKET_ERROR){
		printf("closesocket() failed: %d\n",WSAGetLastError());
		exit(SOCKET_ERROR);
	}

	/* Step 6: Clean up winsocket - this is for Windows only! */
	/* in pair with WSAStartup() */
    WSACleanup();
	printf("....Client returned!\n\n");

    return 0;
}
