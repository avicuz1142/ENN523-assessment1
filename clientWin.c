#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <stdbool.h>
#include <time.h>

#define SERVER_IP_ADDR "127.0.0.1"
#define SERVPORT 8886
#define BUFLEN 1024

#pragma comment(lib,"ws2_32.lib")

// Function to get current timestamp
void getTimestamp(char* buffer) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buffer, "%02d:%02d:%02d:%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

int main() {
    WSADATA wsa_data;
    SOCKET sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
    serv_addr.sin_port = htons(SERVPORT);

    // Connect to server
    connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("Connected to server...\n");

    while (1) {
        // Receive message from server
        int recvStatus = recv(sockfd, buffer, BUFLEN - 1, 0);
        if (recvStatus <= 0) break;
        buffer[recvStatus] = '\0';
        printf("Received: %s\n", buffer);

        // Handle request message
        if (buffer[0] == 'R' || buffer[0] == 'r') {
            char seq[8], timestamp[32];
            sscanf(buffer, "%*c %s", seq);
            getTimestamp(timestamp);
            // Send ACK with client timestamp
            sprintf(buffer, "ACK R %s %s", seq, timestamp);
            send(sockfd, buffer, strlen(buffer), 0);
        }
        // Handle exit handshake
        else if (buffer[0] == 'E' || buffer[0] == 'e') {
            char seq[8], timestamp[32];
            sscanf(buffer, "%*c %s", seq);
            getTimestamp(timestamp);
            // Send ACK for exit
            sprintf(buffer, "ACK E %s %s", seq, timestamp);
            send(sockfd, buffer, strlen(buffer), 0);

            // Receive final ACK from server
            recv(sockfd, buffer, BUFLEN - 1, 0);
            buffer[BUFLEN - 1] = '\0';
            printf("Received Final ACK: %s\n", buffer);
            break;
        }
    }

    // Cleanup
    closesocket(sockfd);
    WSACleanup();
    return 0;
}