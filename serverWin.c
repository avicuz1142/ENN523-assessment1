#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include<stdio.h>      // standard input/output functions
#include<stdlib.h>     // general utilities (e.g., malloc, exit)
#include<string.h>     // string handling functions
#include<windows.h>    // Windows-specific functions and types
#include<stdbool.h>    // boolean type support
#include<time.h>       // time functions

#define BUFLEN 1024
#define QUITKEY 0x1b
#define CONFIG_FILE "server_config.txt"

#pragma comment(lib,"ws2_32.lib") // link with Winsock library

// Get current system time in formatted string hh:mm:ss:ms
void getTimestamp(char* buffer) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buffer, "%02d:%02d:%02d:%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
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

            @@ - 98, 34 + 102, 37 @@
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