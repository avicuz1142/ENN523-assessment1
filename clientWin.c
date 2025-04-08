#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>      // standard input/output
#include <stdlib.h>     // general utilities
#include <windows.h>    // Windows-specific API
#include <stdbool.h>    // boolean support
#include <time.h>       // time functions

#define SERVER_IP_ADDR "127.0.0.1"
#define SERVPORT 8886
#define BUFLEN 1024

#pragma comment(lib,"ws2_32.lib") // link Winsock library

// Get current time in formatted string
void getTimestamp(char* buffer) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(buffer, "%02d:%02d:%02d:%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

int main() {
    // === Initialization ===
    WSADATA wsa_data;
    SOCKET sockfd;
    struct sockaddr_in serv_addr, from_addr;
    int from_len = sizeof(from_addr);
    char buffer[BUFLEN];

    WSAStartup(MAKEWORD(2, 2), &wsa_data);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Server address config
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP_ADDR);
    serv_addr.sin_port = htons(SERVPORT);

    // Send dummy INIT message so server knows client's address
    sendto(sockfd, "INIT", 4, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    printf("UDP Client started...\n");

    // === Main Communication Loop ===
    while (1) {
        // Wait for incoming message from server
        int recvStatus = recvfrom(sockfd, buffer, BUFLEN - 1, 0, (struct sockaddr*)&from_addr, &from_len);
        if (recvStatus <= 0) break;
        buffer[recvStatus] = '\0';
        printf("Received: %s\n", buffer);

        // Handle request message (R)
        if (buffer[0] == 'R' || buffer[0] == 'r') {
            char seq[8], timestamp[32];
            sscanf(buffer, "%*c %s", seq);
            getTimestamp(timestamp);
            sprintf(buffer, "ACK R %s %s", seq, timestamp);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        }
        // Handle termination message (E)
        else if (buffer[0] == 'E' || buffer[0] == 'e') {
            char seq[8], timestamp[32];
            sscanf(buffer, "%*c %s", seq);
            getTimestamp(timestamp);
            sprintf(buffer, "ACK E %s %s", seq, timestamp);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

            // Receive final ACK
            recvfrom(sockfd, buffer, BUFLEN - 1, 0, (struct sockaddr*)&from_addr, &from_len);
            buffer[BUFLEN - 1] = '\0';
            printf("Received Final ACK: %s\n", buffer);
            break;
        }
    }

    // === Cleanup ===
    closesocket(sockfd);
    WSACleanup();
    return 0;
}