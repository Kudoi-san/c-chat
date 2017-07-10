#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>//hostent
#include <arpa/inet.h>//inet_addr
#include <pthread.h>//threading
#include <ncurses.h>
#include <libnotify/notify.h>
#include <glib.h>

//ncurses windows
WINDOW *chatwin;
WINDOW *sendwin;

int chatline = 0;//keeps track of what line the chat window is on
int sendline = 0;

//socket
int sock;
int msgsize = 378;

bool cont = 1;

void *sender();
void *listener();
void wordwrap(const int *nsock, const char *msg);

void error(const char *msg);

int main(int argc, char *argv[])
{
	//getting ip of hostname
	char *hostname = "localhost";
	char ip[100];
	char buffer[msgsize];
	struct hostent *he;
	struct in_addr **addr_list;
	int i;

	if((he = gethostbyname(hostname)) == NULL){
		herror("gethostbyname");
		return 1;
	}

	addr_list = (struct in_addr **)he->h_addr_list;
	for(int i = 0; addr_list[i] != NULL; i++){
		strcpy(ip, inet_ntoa(*addr_list[i]));
	}

	printf("Connecting to %s...\n", hostname);

	//int sock;
	struct sockaddr_in server;
	char *msg;
	char serv_reply[msgsize];

	//creating socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		error("Failed to create socket");
	}

	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(9001);

	//connect to the server
	if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0){
		error("Failed to connect to server");
	}

	printf("Connected to %s\n", hostname);

	//create the windows
	initscr();
	cbreak();

	int h, nh, w, starty,startx;

	h = LINES - 7;
	w = COLS - 4;
	starty = 0;
	startx = 2;

	chatwin = newwin(h, w, starty, startx);
	scrollok(chatwin, TRUE);
	box(chatwin, 0, 0);

	nh = 3;
	starty = h + 1;
	sendwin = newwin(nh, w, starty, startx);
	//scrollok(sendwin, TRUE);
	box(sendwin, 0, 0);

	wsetscrreg(chatwin, 1, h - 2);
	//wsetscrreg(sendwin, 1,i );

	refresh();
	wrefresh(chatwin);
	wrefresh(sendwin);

	//set up threads
	pthread_t threads[2];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	//create threads
	pthread_create(&threads[0], &attr, sender, NULL);
	pthread_create(&threads[1], &attr, listener, NULL);

	while(cont);//keep threads alive
	close(sock);
	endwin();
	return 0;
}

void error(const char *msg){
	perror(msg);
	exit(1);
}

void *sender()
{
	int nsock = sock;
	char buffer[msgsize];

	while(1){	
		//zero the buffer for the message
		bzero(buffer, msgsize);
		//get the input from the user in the sendwin window
		mvwgetnstr(sendwin, 1, 1, buffer, (msgsize - 1));	
		
		if(strcmp(buffer, "/quit") == 0){
			cont = 0;
			pthread_exit(NULL);
		}
		/*else{
			if(buffer[0] != '/'){
				if(chatline != LINES - 9){
					chatline++;
				}else{
					scroll(chatwin);
					box(chatwin, 0, 0);
				}			
				mvwprintw(chatwin, chatline, 1, buffer);
			}
		}*/
		strncat(buffer, "\n", (msgsize) - strlen(buffer));
		write(nsock, buffer, strlen(buffer));
		
		//scroll(sendwin);
		werase(sendwin);
		box(sendwin, 0, 0);
		refresh();
		wrefresh(sendwin);
		//wrefresh(chatwin);
	}
}

void *listener()
{
	int nsock = sock;
	int srvmsgsize = 400;//message size of the message from the server
	char buffer[srvmsgsize];
	NotifyNotification *n;
	notify_init("test");

	while(1){
		bzero(buffer, srvmsgsize);
		refresh();
		wrefresh(chatwin);
		wrefresh(sendwin);
		read(nsock, buffer, srvmsgsize);
		n = notify_notification_new("chat.nijiura", buffer, 0);
		notify_notification_set_timeout(n, 3000);
		//g_object_unref(G_OBJECT(n));
		
		if(!notify_notification_show(n, 0)){
			printf("Notification failed to show\n");
		}
		
		if(chatline != LINES - 9){
			chatline++;
		}else{
			scroll(chatwin);
			box(chatwin, 0, 0);
		}

		mvwprintw(chatwin, chatline, 1, buffer);
	}	
}

void wordwrap(const int *nsock, const char *msg)
{

}
