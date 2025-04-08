#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<stdio.h>      // standard input/output functions
#include<stdlib.h>     // general utilities (e.g., malloc, exit)
#include<string.h>     // string handling functions
#include<windows.h>    // Windows-specific functions and types
#include<stdbool.h>    // boolean type support
#include<time.h>       // time functions
#include <winsock.h>

#define SERVER_IP_ADDR "127.0.0.1" /*loopback for testing */
#define PORT   8886  /* port number */
#define BUFLEN 1024  /* buffer length */
#define SLEEPTIME 3000  /* in milliseconds */
#define LOOPLIMIT 8  /* loop testing sendto()/recvfrom() */
#define QUITKEY 'E' /* ASCII code of ESC */
#define CONFIG_FILE "server_config.txt"

#pragma comment(lib,"ws2_32.lib") // link with Winsock library

// Get current system time in formatted string hh:mm:ss:ms
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

// Load IP address and port from configuration file
bool loadConfig(char* ip, int* port) {
    FILE* fp = fopen(CONFIG_FILE, "r");
    if (!fp) return false;
    fscanf(fp, "%s %d", ip, port);
    fclose(fp);
    return true;
}

int main() {
    // === Initialization ===
    WSADATA wsa_data;
    SOCKET sockfd;
    struct sockaddr_in servaddr, clientaddr;
    int addrlen = sizeof(clientaddr);
    char buffer[BUFLEN], cmd;
    bool stop = false;
    int seqno = 1;

    char ip[50];
    int port;

    // Try to load server IP and port from config file, fallback to user input
    if (!loadConfig(ip, &port)) {
        printf("Config file not found. Enter IP address to bind: ");
        scanf("%s", ip);
        printf("Enter port: ");
        scanf("%d", &port);
        getchar(); // clear newline
    }
    else {
        printf("Loaded config: IP = %s, Port = %d\n", ip, port);
    }

    // === Socket Setup ===
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);
    bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    printf("UDP Server is running...\nWaiting for first message to capture client address...\n");
    // Initial dummy receive to learn client's address
    recvfrom(sockfd, buffer, BUFLEN - 1, 0, (struct sockaddr*)&clientaddr, &addrlen);

    // === Main Communication Loop ===
    DWORD lastTick = GetTickCount();
    while (!stop) {
        DWORD now = GetTickCount();

        // Every 3 seconds send a request to client
        if (now - lastTick >= 3000) {
            char timestamp[32];
            getTimestamp(timestamp);
            sprintf(buffer, "R %05d %s", seqno, timestamp);

            DWORD sendTime = GetTickCount(); // Start time for RTT
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientaddr, addrlen);
            printf("Sent: %s\n", buffer);

            // Receive ACK from client
            int recvStatus = recvfrom(sockfd, buffer, BUFLEN - 1, 0, (struct sockaddr*)&clientaddr, &addrlen);
            DWORD ackReceiveTime = GetTickCount(); // End time for RTT

            if (recvStatus <= 0) break;
            buffer[recvStatus] = '\0';

            char clientAckTime[32];
            sscanf(buffer, "ACK R %*d %s", clientAckTime);
            printf("Received: %s\n", buffer);

            // Respond with final ACK and server's timestamp
            char serverRecvTime[32];
            getTimestamp(serverRecvTime);
            sprintf(buffer, "ACK %05d %s", seqno, serverRecvTime);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientaddr, addrlen);
            printf("Sent: %s\n", buffer);

            DWORD rtt = ackReceiveTime - sendTime;
            printf("RTT for seq %05d: %lu ms\n", seqno, rtt);

            seqno++;
            lastTick = now;
        }

        // Check for termination command from keyboard
        if (kbhit()) {
            cmd = getch();
            if (cmd == 'e' || cmd == 'E') {
                char timestamp[32];
                getTimestamp(timestamp);
                sprintf(buffer, "E %05d %s", seqno, timestamp);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientaddr, addrlen);

                // Wait for ACK from client
                recvfrom(sockfd, buffer, BUFLEN - 1, 0, (struct sockaddr*)&clientaddr, &addrlen);
                buffer[BUFLEN - 1] = '\0';
                printf("Received: %s\n", buffer);

                // Send final ACK
                getTimestamp(timestamp);
                sprintf(buffer, "ACK %05d %s", seqno, timestamp);
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clientaddr, addrlen);
                stop = true;
            }
        }
    }

    // === Cleanup ===
    closesocket(sockfd);
    WSACleanup();
    return 0;
}