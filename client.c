#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <stdbool.h> 	/* boolean */
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	/* inet_addr */

#define USERNAME_LENGTH 16		/* maximum word length for username */
#define PASSWORD_LENGTH 16		/* maximum word length for password */

/* function prototypes */
void welcome_message();
int menu();
bool logon(int socket);

/* 
 * main function 
 */
int main(int argc, char** argv) {

	int socket_desc, read_size;
	struct sockaddr_in server;
	int option;

	/* Check command line arguments */
	if (argc != 3) {
		fprintf(stderr, "usage: hostname and port number\n");
		exit(1);
	}

	/* craete socket */
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_desc == -1) {
		perror("socket");
		exit(1);
	}
	puts("Socket created");

	/* configuring end point */
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));

	/* connect to remote server */
	if (connect(socket_desc, (struct sockaddr *)&server,
		sizeof(server)) < 0) {
		perror("connect");
		exit(1);
	}
	puts("Connected\n");

	/* keep communicating with server */
	do {
		welcome_message();

		if (logon(socket_desc)) {
			system("clear");
			welcome_message();
			option = menu();
		} else {
			printf("\nYou entered either an incorrect username or password - disconnecting\n");
			break;
		}
	} while (option != 3);

	close(socket_desc);

	return EXIT_SUCCESS;
}

void welcome_message() {
	system("clear");
	printf("\n===========================================");
	printf("\n\nWelcome to the Online Hangman Game System\n\n");
	printf("===========================================\n");
}

int menu() {
	int opt, temp, status;

	printf("\n\nPlease enter a selection\n");
	printf("(1) Play Hangman\n");
	printf("(2) Show Leaderboard\n");
	printf("(3) Quit\n\n");

	printf("Selection option 1-3: ");
	status = scanf("%d", &opt);

	while (status != 1 || opt < 1 || opt > 3) {
		while ((temp = getchar()) != EOF && temp != '\n');
		puts("invalid input");
		printf("Selection option 1-3: ");
		status = scanf("%d", &opt);
	}

	return opt;
}

bool logon(int socket) {
	char username[USERNAME_LENGTH];
	char password[PASSWORD_LENGTH];
	int server_signal;
	bool valid;

	printf("\n\nYou are required to logon with your registered username and password\n");
	printf("\nPlease enter your username: ");
	scanf("%s", username);

	if (send(socket, username, strlen(username), 0) < 0) {
		puts("send failed");
		return false;
	}

	printf("Please enter your password: ");
	scanf("%s", password);

	if (send(socket, password, strlen(password), 0) < 0) {
		puts("send failed");
		return false;
	}

	while (1) {
		if (recv(socket, &server_signal, sizeof(server_signal), 0) < 0) {
			puts("recv failed");
			break;
		}
		if (server_signal == 1) {
			valid = true;
			break;
		} else {
			valid = false;
			break;
		}
	}

	return valid;
} 
