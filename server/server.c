#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>//inet_addr
#include <unistd.h>//write
#include <pthread.h>//threading

#define MAX_USERS 10

static int uid = 0;

typedef struct
{
	struct sockaddr_in addr;
	int client_socket;
	char name[20];
	int uid;
}user_t;

user_t *users[MAX_USERS];

void error(const char *msg);
void add_client(user_t *usr);
void *connhandler(void *inc);
void send_broadcast(char *msg, int s);
void send_to_ALL(char *msg);
void strip_newline(char *s);
char* get_user_name(int sock);
int get_user_sock(char *n);
void send_message_to_user(char *msg, int s);
		
int main(int argc, char *argv[])
{
	int sock;
	int nsock;
	int *nsocket;
	int c;
	struct sockaddr_in server;
	struct sockaddr_in client;
	char *msg;

	/*creating socket*/
	//tcp/ip let system handle socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		error("Failed to create socket");
	}
	//printf("Socket created\n");
	
	//set server structure
	server.sin_family = AF_INET;//ip
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(9001);//port

	/*bind socket*/
	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		error("Failed to bind socket");
	}
	//printf("Binding socket\n");

	/*listen*/
	if(listen(sock, 5) < 0){
		error("Failed to listen to socket");
	}
	//printf("Listening to socket\n");

	printf("Server started\n");

	while((nsock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c))){
		printf("New connection accepted\n");
		msg = "Connected to server";
		write(nsock, msg, strlen(msg));
		user_t *usr = (user_t *)malloc(sizeof(user_t));
		usr->addr = client;
		usr->client_socket = nsock;
		usr->uid = uid++;
		sprintf(usr->name, "Guest%d", usr->uid);
		add_client(usr);
		
		pthread_t sniffer_thread;
		nsocket = malloc(1);
		*nsocket = nsock;
		
		if(pthread_create(&sniffer_thread, NULL, connhandler, (void *)usr) < 0){
			error("Failed to create thread");
		}
	}

	if(nsocket < 0){
		error("Failed to accept connection");
	}

	return 0;
}

/*
 * Prints error message and exits program
 * */
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void add_client(user_t *usr)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(!users[i]){
			users[i] = usr;
			return;
		}
	}
}

void remove_client(int id)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i]){
			if(users[i]->client_socket == id){
				char leavemsg[40];
				strcpy(leavemsg, get_user_name(users[i]->client_socket));
				strcat(leavemsg, " has disconnected");
				send_to_ALL(leavemsg);
				users[i] = NULL;
				return;
			}
		}
	}
}

char* get_user_name(int sock)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i]){
			if(users[i]->client_socket == sock){
				return users[i]->name;
			}		
		}
	}
}

int get_user_sock(char *n)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i]){
			if(strcmp(users[i]->name, n) == 0){
				return users[i]->client_socket;
			}
		}
	}
}

int check_user_online(char *n)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i]){
			if(strcmp(users[i]->name, n) == 0){
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Handles the connection for each client
 * */
void *connhandler(void *inc)
{
	//socket descriptor
	int nsock;
	int rs;
	int msgsize = 378;
	char clientmsg[msgsize];
	user_t *usr = (user_t *)inc;
	nsock = usr->client_socket;

	while((rs = recv(nsock, clientmsg, msgsize, 0)) > 0){
		if(clientmsg[0] == '/'){
			strip_newline(clientmsg);
			char *cmd;
			cmd = strtok(clientmsg, " ");
			if(strcmp(cmd, "/nick") == 0){
				char *newname = strtok(NULL, " ");
				if(newname != NULL){
					if(strlen(newname) > 20){
						char nameTooLong[45];
						strcpy(nameTooLong, "System: Usernames must be <21 characters");
						send_message_to_user(nameTooLong, nsock);
					}else if(check_user_online(newname) == 1 || strcmp(newname, "System") == 0){
						char nameTaken[45];
						strcpy(nameTaken, "System: That username is already in use");
						send_message_to_user(nameTaken, nsock);
					}else{
						char *oldname;
						oldname = strdup(usr->name);
						strcpy(usr->name, newname);
						char nameChangemsg[100];
						strcpy(nameChangemsg, "System: ");
						strcat(nameChangemsg, oldname);
						strcat(nameChangemsg, " has changed their name to ");
						strcat(nameChangemsg, newname);
						free(oldname);
						send_to_ALL(nameChangemsg);
					}
				}else{
					char nickUsage[55];
					strcpy(nickUsage, "System: Invalid use of command. Example: /nick NewName");
					send_message_to_user(nickUsage, nsock);
				}
			}else if(strcmp(cmd, "/query") == 0){
				char *to = strtok(NULL, " ");
				char *querymsg = strtok(NULL, "\n");
				if(to != NULL){
					if(check_user_online(to) == 1){
						if(querymsg != NULL){
							char queryIn[400];
							char queryOut[400];
							
							strcpy(queryIn, "To ");
							strcpy(queryOut, "From ");

							strcat(queryIn, to);
							strcat(queryOut, usr->name);

							strcat(queryIn, ": ");
							strcat(queryOut, ": ");

							strcat(queryIn, querymsg);
							strcat(queryOut, querymsg);

							send_message_to_user(queryIn, usr->client_socket);
							send_message_to_user(queryOut, get_user_sock(to));
						}
					}else{
						char usrNotOnline[50];
						strcpy(usrNotOnline, "System: That user is not online.");
						send_message_to_user(usrNotOnline, nsock);
					}
				}else{
					char queryUsage[55];
					strcpy(queryUsage, "System: Invalid use of command. Example: /query target Your message goes here");
					send_message_to_user(queryUsage, nsock);
				}
			}
		}else{
			char finalmsg[400];
			bzero(finalmsg, 400);
			strcpy(finalmsg, usr->name);
			strcat(finalmsg, ": ");
			strcat(finalmsg, clientmsg);
			printf("%s", finalmsg);
			strip_newline(finalmsg);	
			send_to_ALL(finalmsg);
		}	
	}

	if(rs == 0){
		printf("Client disconnected\n");
	}else if(rs == -1){
		error("recv() has failed");
	}

	close(nsock);
	remove_client(nsock);
	free(usr);
	return 0;
}

/*
 * https://github.com/yorickdewid/Chat-Server/blob/master/chat_server.c
 * */
void strip_newline(char *s)
{
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n'){
			*s = '\0';
		}
		s++;
	}
}

/*
 * Send a message to all clients
 * */
void send_broadcast(char *msg, int s)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i] && users[i]->client_socket != s){
			write(users[i]->client_socket, msg, strlen(msg));
		}
	}	
}

void send_to_ALL(char *msg)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i]){
			write(users[i]->client_socket, msg, strlen(msg));
		}
	}
}

void send_message_to_user(char *msg, int s)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i] && users[i]->client_socket == s){
			write(users[i]->client_socket, msg, strlen(msg));
		}
	}
}
