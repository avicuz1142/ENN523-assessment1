/* ENN523A1 - UDP Server (Window Version)
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<stdio.h>
#include<stdlib.h>
#include <windows.h>    /* winsock2.h has included windows.h */
#include <stdbool.h>

#define SERVER_IP_ADDR "127.0.0.1" /*loopback for testing */
#define PORT   8886  /* port number */
#define BUFLEN 1024  /* buffer length */
#define PERIOD 1000  /* in milliseconds */
#define LOOPLIMIT 8  /* loop testing sendto()/recvfrom() */
#define QUITKEY 0x1b /* ASCII code of ESC */

#pragma comment(lib,"ws2_32.lib")

int main(void){
	WSADATA wsa_data;             /* type defined in winsock2.h */
	SOCKET sockfd, acptdsock;     /* type defined in winsock2.h */
	struct sockaddr_in servaddr, clientAddr;  /* struct in winsock2.h */
	int addrlen=sizeof(servaddr),recvStatus, i=0;
	char buffer[BUFLEN] = {0};
	char *hello = "Hi from server";
	char cmd = QUITKEY; /* character ESC */
	bool stop = false;  /* bool tyle in stdbool.h. stop running */

	struct sockaddr &serverAddrCast = (struct sockaddr &) servaddr;
	struct sockaddr &clientrAddrCast = (struct sockaddr &) clientAddr;

	printf("======== UDP Server ========\n");
	/* Step 1: startup winsocket - this is for Windows only */
	/* in pair with WSACleanup() */
	if(WSAStartup(MAKEWORD(2,2), &wsa_data) != 0){
        printf("WSAStartup failed: %d\n",WSAGetLastError());
		exit(SOCKET_ERROR);
	}

	/* Step 2: Create socket and check it is successful */
	/* in pair with closesocket()   */
	/* SOCK_STREAM forTCP, SOCK_DGRAM for UDP*/
	if ((sockfd = socket(AF_INET,SOCK_DGRAM,0))==SOCKET_ERROR){
		printf("socket() failed: %d\n", WSAGetLastError());
		exit(INVALID_SOCKET);
	}

	/* Step 3: Bind to local TCP Server*/
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
	servaddr.sin_port = htons(PORT);

	if(bind(sockfd,(struct sockaddr *)&servaddr,
			sizeof(servaddr)) == SOCKET_ERROR){
		printf("bind() failed: %d\n", WSAGetLastError());
		exit(SOCKET_ERROR);
	}

	/* Listen and Accept step are for TCP
	// Step 4: Listen
    if ((listen(sockfd,8)) == SOCKET_ERROR){
		printf("listen() failed: %d\n", WSAGetLastError());
		exit(SOCKET_ERROR);
	}

	// Step 5: Accept
	printf("\nServer awaiting connection....\n");
	if ((acptdsock = accept(sockfd,(struct sockaddr *)&servaddr,&addrlen))==INVALID_SOCKET){
		printf("accept() failed: %d\n", WSAGetLastError());
		exit(SOCKET_ERROR);
	}
	printf("....Connection established\n");
	*/

	/* Step 6: Send/Receive Data in loop */
	while (1){
		int clientSize = sizeof(clientAddr);
		if ((sendto(acptdsock,hello,strlen(hello),0, clientrAddrCast, clientSize)) == SOCKET_ERROR){
			printf("sendto() failed: \n",WSAGetLastError());
			exit(SOCKET_ERROR);
		}
		printf("%2d Sent:     %s\n",i,hello);

		recvStatus = recvfrom(acptdsock,buffer,BUFLEN-1,0, , clientrAddrCast, &clientSize);
	    if(recvStatus == 0)
		   	break;
	    if (recvStatus == SOCKET_ERROR){
		    printf("recvfrom() failed: %d\n",WSAGetLastError());
		    break;
		}
		buffer[recvStatus] = 0x00; /* force ending with '\0' */
		printf("    Received: %s\n",buffer);

		if ((++i) == LOOPLIMIT)     /* LOOPLIMIT reached */
			break;  /* make sure i within integrer limit */

		while ((kbhit()) && (!stop)){
			cmd = getch();
			fflush(stdout);
			if (cmd == QUITKEY){
				stop = true;
				printf("Terminating by kbd cmd 0x%x...\n",cmd);
				break;
			}
		}
		if (stop)
			break;


		Sleep(PERIOD); /* PERIOD in milliseconds */
	}

	if ((sendto(acptdsock,&cmd,1,0, clientrAddrCast, clientSize)) == SOCKET_ERROR){
		printf("sendto() failed: \n",WSAGetLastError());
		closesocket(sockfd);
		WSACleanup();
		exit(SOCKET_ERROR);
	}

	/* Step 7: Close socket, in pair with socket() */
	Sleep(2000); /* Allow client to recv last msg without WSAECONNREST err*/
	if ((closesocket(sockfd))== SOCKET_ERROR){
		printf("closesocket() failed: %d\n",WSAGetLastError());
		exit(SOCKET_ERROR);
	}

	/* Step 8: Clean up winsocket - this is for Windows only! */
	/* in pair with WSAStartup() */
    WSACleanup();
	printf("....Server returned!\n\n");

	return 0;
}
