#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <pthread.h>
#include <stdio.h>

#define size 1024

void* recv_func(void* r);
void* send_func(void* s);

int clientSocket;
char *temp;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


int main() {
    // Winsock'i başlat
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Socket creation failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(2900);
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connection failed: %ld\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    printf("Connected to the server!\n");
    temp=(char*)malloc(100*sizeof(char));
    recv(clientSocket, temp, 99, 0);
    temp[strlen(temp)]='\0';
    printf("%s", temp);

    memset(temp, '\0', sizeof (temp));
    scanf(" %99s", temp);

    int bytesSent = send(clientSocket, temp, strlen(temp), 0);
    if (bytesSent == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        free(temp);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    } else if (bytesSent == 0) {
        printf("Connection closed by peer.\n");
        free(temp);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    pthread_t recv_thread, send_thread;
    pthread_create(&recv_thread, NULL, recv_func, NULL);
    pthread_create(&send_thread, NULL, send_func, NULL);
    pthread_join(recv_thread, NULL);

    return 0;
}

void* recv_func(void* r){
    char *income = (char*)malloc(size * sizeof(char));
    while(TRUE){
        memset(income, '\0', size);
        int bytesRecv = recv(clientSocket, income, size, 0);
        if (bytesRecv == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
        } else if (bytesRecv == 0) {
            printf("Connection closed by peer.\n");
            free(income);
            closesocket(clientSocket);
            WSACleanup();
            break;
            return NULL;
        }else{
            printf("%s\n", income);
        }
    }

}
void* send_func(void* s){
    char *output = (char*)malloc(size * sizeof(char));
    while(TRUE){
        memset( output, '\0', size);
        if (fgets(output, size, stdin) != NULL) {
            int len = strlen(output);
            if (len > 0 && output[len-1] == '\n') {
                output[len-1] = '\0';
            }
        }
        int bytesSend = send(clientSocket, output, size, 0);
        if (bytesSend == SOCKET_ERROR) {
            printf("Send failed: %d\n", WSAGetLastError());
        } else if (bytesSend == 0) {
            printf("Connection closed by peer.\n");
            free(output);
            closesocket(clientSocket);
            WSACleanup();
            return NULL;
        }
    }

}




