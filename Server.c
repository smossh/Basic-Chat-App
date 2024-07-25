#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <pthread.h>
struct client_info{
    int clientSocket;
    char name[20];
};
struct recv_info{
    int aim;
    char message[1024];
    int person;
};

//functions
void* listening(void *fsocket);
void* reciving(void* ind);
void send_message(struct recv_info data);

//global variables
int serverSocket;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct client_info *Clients= NULL;
int client_size=0;


int main(){
    //create base elements winsock and server socket
    WSADATA server;
    if(WSAStartup(MAKEWORD(2,2), &server)!=0){
        printf("Eror winsock start\n");
        return 1;
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == 0){
        printf("Error creating server socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    struct sockaddr_in ptr_socket;
    memset(&ptr_socket, 0, sizeof(ptr_socket));
    ptr_socket.sin_family = AF_INET;
    ptr_socket.sin_addr.s_addr = INADDR_ANY;
    ptr_socket.sin_port = htons(2900);

    if(bind(serverSocket, (struct sockaddr*)&ptr_socket, sizeof(ptr_socket)) == SOCKET_ERROR){
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    Clients = (struct client_info*)malloc(12 * sizeof(struct client_info));
    pthread_t listen_thread;
    pthread_create(&listen_thread, NULL, listening, NULL);
    pthread_join(listen_thread, NULL);

    closesocket(serverSocket);
    WSACleanup();
    free(Clients);
    return 0;
}

void* listening(void *g){
    while(TRUE){
        if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR){
            printf("Listen failed: %d\n", WSAGetLastError());
            closesocket(serverSocket);
            WSACleanup();
            return NULL;
        }
        printf("Server is listening on port 2900...\n");

        //create new socket for client
        int cSocket;
        struct sockaddr_in cAddr;
        int cAddrSize = sizeof(cAddr);
        //accept client
        cSocket = accept(serverSocket, (struct sockaddr*)&cAddr, &cAddrSize);
        if (cSocket == 0) {
            printf("Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        if(send(cSocket, "Enter your nickname: ", 25, 0) == SOCKET_ERROR){
            printf("Send failed(88): %d\n", WSAGetLastError());
            closesocket(cSocket);
            continue;
        }
        printf("Client ");
        char temp_nick[20]={0};
        if(recv(cSocket, temp_nick, 19, 0) == SOCKET_ERROR){
            printf("Recv failed(94): %d\n", WSAGetLastError());
            closesocket(cSocket);
            continue;
        }else{
            printf("%s connected\n", temp_nick);
        }

        pthread_mutex_lock(&clients_mutex);//lock
        printf("%d\n", client_size);
        int client_code = client_size;
        client_size++;
        Clients=(struct client_info*)realloc(Clients, (client_size) * sizeof(struct client_info));
        strcpy(Clients[client_code].name, temp_nick);
        Clients[client_code].clientSocket = cSocket;
        printf("%d\n", client_code);
        pthread_mutex_unlock(&clients_mutex);//unlock

        pthread_t recv_thread;
        pthread_create(&recv_thread, NULL, reciving, (void*)client_code);

    }
    return NULL;
}
void del(int d_index){
    pthread_mutex_lock(&clients_mutex);
    closesocket(Clients[d_index].clientSocket);
    for(int i = d_index; i<client_size-1; i++){
        Clients[i].clientSocket = Clients[i+1].clientSocket;
        strcpy(Clients[i].name, Clients[i+1].name);
    }
    client_size--;
    Clients=(struct client_info*)realloc(Clients, client_size*sizeof(struct client_info));
    pthread_mutex_unlock(&clients_mutex);
}
void* reciving(void* ind){

    int client_index = client_size;
    client_index--;
    struct recv_info new;
    int flag = 0;
    char *rule=(char*)malloc(20 * sizeof(char));
    while(TRUE){
        Sleep(5);
        memset(rule, '\0', sizeof(rule));
        if(recv(Clients[client_index].clientSocket, rule, sizeof(rule), 0) == SOCKET_ERROR){
            printf("Recv failed(140): %d\n", WSAGetLastError());
            del(client_index);
            pthread_exit(NULL);
            return NULL;
        }
        rule[strlen(rule)]='\0';
        new.person = client_index;
        //komut
        if(strcmp(rule, "") != 0){

            for(int i=0; i<client_size; i++){
                if(strcmp(rule, Clients[i].name)==0){
                    new.aim = i;
                    flag=1;
                }
            }
            if(strcmp(rule, "\\list") == 0){
                printf("\n%s\n",rule);
                for(int i=0; i< client_size; i++){
                    printf("%s\n", Clients[i].name);
                    if( send(Clients[client_index].clientSocket, Clients[i].name , 20, 0) == SOCKET_ERROR){
                        printf("Conection LOST\n");
                        del(client_index);
                        return NULL;
                    }
                }
            }else if(strcmp(rule, "\\all")==0){
                while(TRUE){
                    if(recv(Clients[client_index].clientSocket, new.message, sizeof(new.message), 0) == SOCKET_ERROR){
                        printf("Conection LOST\n");
                        del(client_index);
                        return NULL;
                    }
                    if(strcmp(new.message, "\\exit") == 0){
                        break;
                    }
                    for(int i=0; i<client_size; i++){
                        new.aim=i;
                        if( i!= new.person){
                            send_message(new);
                        }
                    }
                }

            }else if(flag){

                while(TRUE){
                    if(recv(Clients[client_index].clientSocket, new.message, sizeof(new.message), 0) == SOCKET_ERROR){
                        printf("Conection LOST\n");
                        del(client_index);
                        return NULL;
                    }
                    if(strcmp(new.message, "\\exit") == 0){
                        break;
                    }
                    send_message(new);
                }
            }else{
                if(send(Clients[client_index].clientSocket, "invalid command\n", 20, 0) == SOCKET_ERROR){
                    printf("Conection LOST\n");
                    del(client_index);
                    return NULL;
                }
            }
        }

    }
}
void send_message(struct recv_info data){
    struct  recv_info aim_client = data;
    send(Clients[aim_client.aim].clientSocket, "\\new message", sizeof(aim_client.message), 0);
    send(Clients[aim_client.aim].clientSocket, Clients[aim_client.person].name, sizeof(aim_client.message), 0);
    send(Clients[aim_client.aim].clientSocket, aim_client.message, sizeof(aim_client.message), 0);
    return ;
}

