#ifndef __CLIENT__
#define __CLIENT__

#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <stdbool.h> 	/* boolean */
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	/* inet_addr */
#include <signal.h>
#include <ctype.h>

#define USERNAME_LENGTH 16		/* maximum word length for username */
#define PASSWORD_LENGTH 16		/* maximum word length for password */
#define WORD_LENGTH 16			/* maximum word length */
#define DATA_LENGTH	2048		/* general data length */
#define CLIENT_NO 10			/* number of clients */

/* function prototypes */
void sigint_handler(int sig);
void send_string(int socket, char buffer[]);
void send_int(int socket, int integer);
void receive_string(int socket, char buffer[]);
int receive_int(int socket);
void clear_buffer(char buffer[]);
void welcome();
int menu();
bool logon(int socket, char credential[]);
void play(int socket, char credential[]);
void leaderboard(int socket);
void clear_screen();

#endif 