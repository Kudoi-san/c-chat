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
	char *name;
	int uid;
}user_t;

user_t *users[MAX_USERS];

void error(const char *msg);
void add_client(user_t *usr);
void *connhandler(void *usr);
void send_broadcast(char *msg, int s);
void strip_newline(char *s);
void change_user_name(int sock, char *n);
char* get_user_name(int sock);

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
	if(sock == -1)
	{
		error("Failed to create socket");
	}
	//printf("Socket created\n");
	
	//set server structure
	server.sin_family = AF_INET;//ip
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(9001);//port

	/*bind socket*/
	if(bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		error("Failed to bind socket");
	}
	//printf("Binding socket\n");

	/*listen*/
	if(listen(sock, 5) < 0)
	{
		error("Failed to listen to socket");
	}
	//printf("Listening to socket\n");

	printf("Server started\n");

	while((nsock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		printf("New connection accepted\n");
		msg = "Connected to server\0";
		write(nsock, msg, strlen(msg));
		user_t *usr = (user_t *)malloc(sizeof(user_t));
		usr->addr = client;
		usr->client_socket = nsock;
		usr->uid = uid++;
		//char *nm;
		//strcpy(nm, "Guest");
		//strcat(nm, (char *)usr->uid);
		//char nm[6];
		//nm = "Guest" + usr->uid;
		//usr->name = nm;
		//strcpy(usr->name, nm);
		add_client(usr);
		
		pthread_t sniffer_thread;
		nsocket = malloc(1);
		*nsocket = nsock;
		
		if(pthread_create(&sniffer_thread, NULL, connhandler, (void *)nsocket) < 0)
		{
			error("Failed to create thread");
		}
	}

	if(nsocket < 0)
	{
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
	for(int i = 0; i < MAX_USERS; i++)
	{
		if(!users[i])
		{
			users[i] = usr;
			return;
		}
	}
}

void remove_client(int id)
{
	for(int i = 0; i < MAX_USERS; i++)
	{
		if(users[i])
		{
			if(users[i]->client_socket == id)
			{
				users[i] = NULL;
				printf("Client left\n");
				return;
			}
		}
	}
}

void change_user_name(int sock, char *n)
{
	for(int i = 0; i < MAX_USERS; i++){
		if(users[i]){
			if(users[i]->client_socket == sock){
				
				users[i]->name = n;
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

/*
 * Handles the connection for each client
 * */
void *connhandler(void *sock)
{
	//socket descriptor
	int nsock = *(int *)sock;
	int rs;
	int msgsize = 400;
	char clientmsg[msgsize];

	//user_t *usr = (user_t *)sock;

	while((rs = recv(nsock, clientmsg, msgsize, 0)) > 0)
	{
		if(clientmsg[0] == '/'){
			char *cmd;
			cmd = strtok(clientmsg, " ");
			if(strcmp(cmd, "/nick") == 0){
				//char *nn;
				//strncpy(nn, clientmsg + 6, strlen(clientmsg) - 6);
				//nn[6] = '\0';
				//printf("%s\n", nn);
			}
		}else{
			//char subbuff[7];
			//memcpy(subbuff, &clientmsg[0], 6);
			//subbuff[6] = '\0';

			strip_newline(clientmsg);
			/*
			char *nn = get_user_name(nsock);
			char *nmsg = malloc(strlen(clientmsg) + strlen(nn) + 3);
			strcpy(nmsg, nn);
			strcat(nmsg, ": ");
			strcat(nmsg, clientmsg);
			*/
			send_broadcast(clientmsg, nsock);
		}	
	}

	if(rs == 0)
	{
		printf("Client disconnected\n");
	}else if(rs == -1)
	{
		error("recv() has failed");
	}

	close(*(int *)sock);
	remove_client(*(int *)sock);
	free(sock);
	return 0;
}

/*
 * https://github.com/yorickdewid/Chat-Server/blob/master/chat_server.c
 * */
void strip_newline(char *s)
{
	while(*s != '\0')
	{
		if(*s == '\r' || *s == '\n')
		{
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
	for(int i = 0; i < MAX_USERS; i++)
	{
		if(users[i] && users[i]->client_socket != s)
		{
			write(users[i]->client_socket, msg, strlen(msg));
		}
	}	
}
