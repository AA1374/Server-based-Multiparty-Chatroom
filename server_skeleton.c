#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>

/***************************/
#include<pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/***************************/

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {0}; // list of users


/* Add user to userList */
void user_add(user_info_t *user);
/* Get user name from userList */
char * get_username(int sockfd);
/* Get user sockfd by name */
int get_sockfd(char *name);

/* Add user to userList */
void user_add(user_info_t *user){
	if(users_count ==  MAX_USERS){
		printf("sorry the system is full, please try again later\n");
		return;
	}
	/***************************/
	/* add the user to the list */
	/**************************/
	pthread_mutex_lock(&mutex);
	listOfUsers[users_count] = user;
	users_count++;
	pthread_mutex_unlock(&mutex);
	/********************************/
}

/* Determine whether the user has been registered  */
int isNewUser(char* name) {
	int i;
	int flag = -1;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/
	for (i = 0; i < users_count; i++) {
		if (strcmp(listOfUsers[i]->username, name) == 0) {
			flag = 0;
			break;
		}
	}
	/********************************/
	return flag;
}

/* Get user name from userList */
char * get_username(int ss){
	int i;
	static char uname[MAX];
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	for (i = 0; i < users_count; i++) {
		if (listOfUsers[i]->sockfd == ss) {
			strcpy(uname, listOfUsers[i]->username);
			break;
		}
	}
	/********************************/
	printf("get user name: %s\n", uname);
	return uname;
}

/* Get user sockfd by name */
int get_sockfd(char *name){
	int i;
	int sock = -1;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for (i = 0; i < users_count; i++) {
		if (strcmp(listOfUsers[i]->username, name) == 0) {
			sock = listOfUsers[i]->sockfd;
			break;
		}
	}
	/********************************/
	return sock;
}

// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}



int main(){
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof * pfds * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, u, rv;

    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Socket creation failed...\n");
		exit(0);   //Should it be exit(0) or exit(1)?
	}
	else
		printf("Socket successfully created..\n");
	
	bzero(&server_addr, sizeof(server_addr));

	// assign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
		printf("socket bind failed..\n");
		exit(0); //Should it be exit(0) or exit(1)?
	}
	else
		printf("Socket successfully binded..\n");

	/********************************/

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener
	
	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);
		if (poll_count == -1) {
			perror("poll");
			exit(1);
		}
		/**************************************/

		// run through the existing connections looking for data to read
        	for(i = 0; i < fd_count; i++) {
            	  if (pfds[i].revents & POLLIN) { // we got one!!
                    if (pfds[i].fd == listener) {
                      /**************************/
					  /* we are the listener and we need to handle new connections from clients */
					  /****************************/
						addr_size = sizeof(client_addr);
						newfd = accept(listener, (struct sockaddr*)&client_addr, &addr_size);

						if (newfd == -1) {
							perror("accept");
						}
						else {
							add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
							printf("pollserver: new connection from %s on socket %d\n", inet_ntoa(client_addr.sin_addr), newfd);
						}
					  /********************************/
						// send welcome message
						bzero(buffer, sizeof(buffer));
						strcpy(buffer, "Welcome to the chat room!\nPlease enter a nickname.\n");
						if (send(newfd, buffer, sizeof(buffer), 0) == -1)
							perror("send");
                      
                    } else {
                        // handle data from a client
						bzero(buffer, sizeof(buffer));
                        if ((nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0)) <= 0) {
                          // got error or connection closed by client
                          if (nbytes == 0) {
                            // connection closed
                            printf("pollserver: socket %d hung up\n", pfds[i].fd);
                          } else {
                            perror("recv");
                          }
						  close(pfds[i].fd); // Bye!
						  del_from_pfds(pfds, i, &fd_count);
                        } else {
                            // we got some data from a client
							if (strncmp(buffer, "REGISTER", 8)==0){
								printf("Got register/login message\n");
								/********************************/
								/* Get the user name and add the user to the userlist*/
								/**********************************/
								// print buffer which contains the client contents
								char name[C_NAME_LEN];
								//int j = 0;

								for (int ii=0; ii<C_NAME_LEN; ii++){
									if(buffer[ii+8] == '\n'){
										name[ii] = '\0';
										break;
									}
									else
										name[ii] = buffer[ii+8];
								}
								//name[C_NAME_LEN] = '\0'; // add the null character at the end
								
                        		printf("The name of the user of incoming connection is : %s\n", name);
								/**********************************/

								if (isNewUser(name) == -1) {
									/********************************/
									/* it is a new user and we need to handle the registration*/
									/**********************************/
									// add the user to the userlist
									user_info_t *new_user = malloc(sizeof(user_info_t));
									strcpy(new_user->username, name);
									new_user->sockfd = newfd;
									new_user->state = 1;
									printf("add user name: %s\n", new_user->username);
									user_add(new_user);
									/********************************/
									/* create message box (e.g., a text file) for the new user */
									/**********************************/
									// create a file for the user
									FILE *fp;
									char filename[MAX];
									strcpy(filename, new_user->username);
									strcat(filename, ".txt");
									fp = fopen(filename, "w");	
									fclose(fp);
									/**********************************/

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Welcome ");
									strcat(buffer, new_user->username);
									//printf("DEBUG: %s\n", buffer);
									strcat(buffer, " to join the chat room!\n");
									//printf("DEBUG: %s\n", buffer);
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									pthread_mutex_lock(&mutex);
									for (int j = 0; j < MAX_USERS; j++) {
										if(listOfUsers[j] != NULL && listOfUsers[j]->state == 1 && listOfUsers[j]->sockfd != listener) {
											if (send(listOfUsers[j]->sockfd, buffer, sizeof(buffer), 0) == -1)
												perror("send");
										}
									}
									pthread_mutex_unlock(&mutex);
									/*****************************/
								
									/*****************************/
									/* send registration success message to the new user*/
									/*****************************/
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "A new account has been created.\n");
									if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1)
										perror("send");
									/*****************************/
								}
								else {
									/********************************/
									/* it's an existing user and we need to handle the login. Note the state of user,*/
									/**********************************/
									for(int k = 0; k < users_count; k++) {
										if (strcmp(listOfUsers[k]->username, name) == 0) {
											listOfUsers[k]->state = 1;
											listOfUsers[k]->sockfd = newfd;
										}
									}

									/********************************/
									/* send the offline messages to the user and empty the message box*/
									/**********************************/
									// send offline messages to the user
									FILE *fp;
									char filename[C_NAME_LEN];
									strcpy(filename, name);
									strcat(filename, ".txt");
									fp = fopen(filename, "r");
									char line[MAX];	
									while (fgets(line, sizeof(line), fp)) {
										bzero(buffer, sizeof(buffer));
										strcpy(buffer, line);
										if (send(newfd, buffer, sizeof(buffer), 0) == -1)
											perror("send");
									}
									fclose(fp);
									// empty the message box
									fp = fopen(filename, "w");
									fclose(fp);
									/**********************************/
								


									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcat(buffer, name);
									strcat(buffer, " is online!\n");
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									for(int k = 0; k < users_count; k++) {
										if (listOfUsers[k]->sockfd != newfd && listOfUsers[k]->sockfd != listener) {
											printf("%d", listOfUsers[k]->sockfd);
											if (send(listOfUsers[k]->sockfd, buffer, sizeof(buffer), 0) == -1)
												perror("send");
										}
									}
								}
							}
							else if (strncmp(buffer, "EXIT", 4)==0){
								printf("Got exit message. Removing user from system\n");
								// send leave message to the other members
                                bzero(buffer, sizeof(buffer));
								strcpy(buffer, get_username(pfds[i].fd));
								strcat(buffer, " has left the chatroom\n");
								/*********************************/
								/* Broadcast the leave message to the other users in the group*/
								/**********************************/
								pthread_mutex_lock(&mutex);
								for (int j = 0; j < MAX_USERS; j++) {
									if(listOfUsers[j] != NULL && listOfUsers[j]->sockfd != listener) {
										if (send(listOfUsers[j]->sockfd, buffer, sizeof(buffer), 0) == -1)
											perror("send");
									}
								}
								pthread_mutex_unlock(&mutex);

								/*********************************/
								/* Change the state of this user to offline*/
								/**********************************/
								for(int k = 0; k < users_count; k++) {
									if(strcmp(listOfUsers[k]->username, get_username(pfds[i].fd)) == 0) {
										listOfUsers[k]->state = 0;
									}
								}
								/*********************************/

								//close the socket and remove the socket from pfds[]
								close(pfds[i].fd);
								del_from_pfds(pfds, i, &fd_count);
							}
							else if (strncmp(buffer, "WHO", 3)==0){
								// concatenate all the user names except the sender into a char array
								printf("Got WHO message from client.\n");
								char ToClient[MAX];
								bzero(ToClient, sizeof(ToClient));
								/***************************************/
								/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
								/* The state of each user (online or offline)should be labelled.*/
								/***************************************/
								for(int k = 0; k < MAX_USERS; k++){
									if(listOfUsers[k] != NULL && strcmp(listOfUsers[k]->username, get_username(pfds[i].fd)) != 0) {
										strcat(ToClient, listOfUsers[k]->username);
										if(listOfUsers[k]->state == 1) {
											strcat(ToClient, "*\t");
										}
										else {
											strcat(ToClient, "\t");
										}
									}
								}

								strcat(ToClient, "\n");
								if (send(pfds[i].fd, ToClient, sizeof(ToClient), 0) == -1)
									perror("send");
								/***************************************/

							}
							else if (strncmp(buffer, "#", 1)==0){
								// send direct message 
								// get send user name:
								printf("Got direct message.\n");
								// get which client sends the message
								char sendname[MAX];
								// get the destination username
								char destname[MAX];
								// get sender sock
								int sendsock;
								// get dest sock
								int destsock;
								// get the message
								char msg[MAX];
								/**************************************/
								/* Get the source name xx, the target username and its sockfd*/
								/*************************************/
								// get the source name                     //Most likely wrong
								strcpy(sendname, get_username(pfds[i].fd));
								// get the sender sock
								sendsock = get_sockfd(sendname);
								int buffer_len = strlen(buffer);
								//int destname_end = -1; //Try another approach

								

								for (int i = 1; i < buffer_len; i++){
									if(buffer[i] == ':'){
										strncpy(destname, buffer+1, i-1);
										destname[i-1] = '\0';
										//destsock = get_sockfd(destname); //Newly Added
										strncpy(msg, buffer+i+1, MAX);
										break;
									}
								}

								// if (destname_end >= 0) {
								// 	strncpy(destname, buffer+1, destname_end);
								// 	destname[destname_end] = '\0';

								// 	strncpy(msg, buffer+destname_end+2, MAX);
								
								// for(int x = 0; x < MAX; x++){
								// 	if(msg[x] == '\n'){
								// 		msg[x] = '\0';
								// 		break;
								// 	}
								// }

								// 	destsock = get_sockfd(destname);
								// }

								destsock = get_sockfd(destname);

								printf("destname: %s\n", destname);
								printf("msg: %s\n", msg);
								printf("\ndestsock: %d\n", destsock);
								/**************************************/

								if (destsock == -1) {
									/**************************************/
									/* The target user is not found. Send "no such user..." messsge back to the source client*/
									/**************************************/
									printf("sendsock: %d\n", sendsock);
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "No such user...\n");
									
									//DEBUGGINGGGG
									//printf("sendsock: %d\n", sendsock);

									// if(send(sendsock, buffer, sizeof(buffer), 0) == -1)
									// 	perror("send");
									send(sendsock, buffer, sizeof(buffer), 0);

									/**************************************/
								}
								else {
									// The target user exists.
									// concatenate the message in the form "xx to you: msg"
									char sendmsg[MAX];
									strcpy(sendmsg, sendname);
									strcat(sendmsg, " to you: ");
									strcat(sendmsg, msg);

									printf("sendmsg: %s\n", sendmsg);
									/**************************************/
									/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
									/* For the offline case, send "...Leaving message successfully" message to the source client*/
									/*************************************/
									for(int k = 0; k < MAX_USERS; k++) {
										if(listOfUsers[k] != NULL && strcmp(listOfUsers[k]->username, destname) == 0) {
											if(listOfUsers[k]->state == 1) {
												if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
													perror("send");
												}
											}
											else if(listOfUsers[k]->state == 0) {
												bzero(buffer, sizeof(buffer));
												strcpy(buffer, destname);
												strcat(buffer, " is offline. Leaving message successfully\n");
												if (send(sendsock, buffer, sizeof(buffer), 0) == -1){
													perror("send");
												}
												// write the message into the message box
												char msgbox[MAX];
												strcpy(msgbox, destname);
												strcat(msgbox, ".txt");
												FILE *fp = fopen(msgbox, "a");
												fprintf(fp, "%s\n", sendmsg);
												fclose(fp);
											}
										}
									}
								}
								
								

								if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
									perror("send");
								}

							}
							else{
								printf("Got broadcast message from user\n");
								/*********************************************/
								/* Broadcast the message to all users except the one who sent the message*/
								/*********************************************/
								char sendmsg[MAX];
								strcpy(sendmsg, get_username(pfds[i].fd));
								strcat(sendmsg, ": ");
								strcat(sendmsg, buffer);
								int sender = pfds[i].fd;

								pthread_mutex_lock(&mutex);
								for(int k = 0; k < MAX_USERS; k++) {
									if(listOfUsers[k] != NULL && listOfUsers[k]->sockfd != sender) {
										if (send(listOfUsers[k]->sockfd, sendmsg, sizeof(sendmsg), 0) == -1){
											perror("send");
										}
									}
								}
								pthread_mutex_unlock(&mutex);
								/*********************************************/
							}   

                        }
                    } // end handle data from client
                  } // end got new incoming connection
                } // end looping through file descriptors
        } // end for(;;) 	

	return 0;
}
