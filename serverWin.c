/* ENN523A1 - UDP Server (Window Version)
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<stdio.h>
#include<stdlib.h>
#include <windows.h>    /* winsock2.h has included windows.h */
#include <stdbool.h>
#include <winsock.h>

#define SERVER_IP_ADDR "127.0.0.1" /*loopback for testing */
#define PORT   8886  /* port number */
#define BUFLEN 1024  /* buffer length */
#define SLEEPTIME 3000  /* in milliseconds */
#define LOOPLIMIT 8  /* loop testing sendto()/recvfrom() */
#define QUITKEY 'E' /* ASCII code of ESC */

#pragma comment(lib,"ws2_32.lib")

// Function to get current timestamp
void getTimestamp(char* buffer) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buffer, "%02d:%02d:%02d:%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void sendFromServer(SOCKET sockfd, char buffer) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buffer, "%02d:%02d:%02d:%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

int main(void){
	WSADATA wsa_data;             /* type defined in winsock2.h */
	SOCKET sockfd;     /* type defined in winsock2.h */
	struct sockaddr_in servaddr, clientAddr;  /* struct in winsock2.h */
	int addrlen=sizeof(servaddr),recvStatus, i=0;
	char buffer[BUFLEN] = {0}, checkEnd[8] = "";
	int seq = 1;
	DWORD startTime, endTime;
	char *hello = "Hi from server";
	char cmd = QUITKEY; /* character ESC */
	bool stop = false;  /* bool tyle in stdbool.h. stop running */

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

	printf("UDP server running on port %d...\n", PORT);

	/* Step 6: Send/Receive Data in loop */
	int clientSize = sizeof(clientAddr);
	while (1){
		char seqstr[8], timestamp[32];

        // Send "R <seq> <timestamp>" to client
		sprintf(seqstr, "%05d", seq);
		startTime = GetTickCount();
        getTimestamp(timestamp);
        sprintf(buffer, "R %s %s", seqstr, timestamp);

		if ((sendto(sockfd,buffer,strlen(buffer),0, (struct sockaddr*) &clientAddr, clientSize)) == SOCKET_ERROR){
			printf("sendto() failed: \n",WSAGetLastError());
			exit(SOCKET_ERROR);
		}
		printf("Sent:     %s\n",buffer);

		// Receive
		recvStatus = recvfrom(sockfd,buffer,BUFLEN-1,0, (struct sockaddr*) &clientAddr, &clientSize);
	    if(recvStatus == 0)
		   	break;
	    if (recvStatus == SOCKET_ERROR){
		    printf("recvfrom() failed: %d\n",WSAGetLastError());
		    break;
		}
		endTime = GetTickCount();
		getTimestamp(timestamp);
		buffer[recvStatus] = 0x00; /* force ending with '\0' */
		printf("    Received: %s\n",buffer);

		// Process ACK R from client
		if (strncmp(buffer, "ACK R", 5) == 0) {
            printf("RTT for seq %s = %s ms\n", seqstr, endTime - startTime);
            sprintf(buffer, "ACK %s %s", seqstr, get_time_str());
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, clientSize);
            printf("Sent: %s\n", buffer);
        }

		int checkNum = scanf("%s", &checkEnd);
		if (checkNum == 1 && (checkEnd == QUITKEY || checkEnd == tolower(QUITKEY))) {
			sprintf(buffer, "ACK E %s %s", seqstr, get_time_str());
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*) &clientAddr, clientSize);
            printf("Sent Termination ACK: %s\n", buffer);

            // Final ACK from client
            recvStatus = recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr*) &clientAddr, &clientSize);
            if (recvStatus == SOCKET_ERROR) {
                printf("Final ACK receive failed: %d\n", WSAGetLastError());
            } else {
                buffer[recvStatus] = '\0';
                printf("Received Final ACK: %s\n", buffer);
				if (strncmp(buffer, "ACK E", 5) == 0) {
					printf("RTT for seq %s = %s ms\n", seqstr, endTime - startTime);
					sprintf(buffer, "ACK %s %s", seqstr, get_time_str());
					sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientAddr, clientSize);
					printf("Sent: %s\n", buffer);
				}
            }
            break;
		}

		if ((++i) == LOOPLIMIT)     /* LOOPLIMIT reached */
			break;  /* make sure i within integrer limit */
		
		seq++;
		Sleep(SLEEPTIME); /* SLEEPTIME in milliseconds */
		scanf("%s", &checkEnd);
	}

	/* Step 7: Close socket, in pair with socket() */
	// Sleep(2000); /* Allow client to recv last msg without WSAECONNREST err*/
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
