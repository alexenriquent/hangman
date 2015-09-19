#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <stdbool.h> 	/* boolean */
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	/* inet_addr */

#define USERNAME_LENGTH 16		/* maximum word length for username */
#define PASSWORD_LENGTH 16		/* maximum word length for password */
#define WORD_LENGTH 16			/* maximum word length */
#define DATA_LENGTH	2048		/* general data length */


#define MIN(x, y) ((x) < (y) ? x : y) /* determin the minimum value */

/* function prototypes */
void send_string(int socket, char message[]);
void send_int(int socket, int integer);
void receive_string(int socket, char message[]);
int receive_int(int socket);

void welcome();
int menu();
bool logon(int socket);
void play(int socket);

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
	welcome();

	if (logon(socket_desc)) {
		do {
			// system("clear");
			welcome();
			option = menu();
			send_int(socket_desc, option);

			switch(option) {
				case 1:
					play(socket_desc);
					break;
				case 2:
					printf("Option 2");
					break;
			}

		} while (option != 3);
	} else {
		printf("\nYou entered either an incorrect username or password - disconnecting\n");
	}

	close(socket_desc);

	return EXIT_SUCCESS;
}

void send_string(int socket, char message[]) {
	if (send(socket, message, strlen(message), 0) < 0) {
		puts("send failed");
		return;
	}
}

void send_int(int socket, int integer) {
	if (send(socket, &integer, sizeof(integer), 0) < 0) {
		puts("send failed");
		return;
	}
}

void receive_string(int socket, char message[]) {
	while (1) {
		if (recv(socket, message, DATA_LENGTH, 0) < 0) {
			puts("recv failed");
			break;
		} else {
			break;
		}
	}
}

int receive_int(int socket) {
	int integer;

	while (1) {
		if (recv(socket, &integer, sizeof(integer), 0) < 0) {
			puts("recv failed");
			break;
		} else {
			break;
		}
	}
	return integer;
}

void welcome() {
	// system("clear");
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
	int server_signal;
	char username[USERNAME_LENGTH];
	char password[PASSWORD_LENGTH];
	bool valid;

	printf("\n\nYou are required to logon with");
	printf("your registered username and password\n");

	printf("\nPlease enter your username: ");
	scanf("%s", username);
	send_string(socket, username);

	printf("Please enter your password: ");
	scanf("%s", password);
	send_string(socket, password);

	server_signal = receive_int(socket);

	if (server_signal == 1) {
		valid = true;
	} else {
		valid = false;
	}

	return valid;
} 

void play(int socket) {
	int word_length, num_guesses;
	char letter[DATA_LENGTH];
	char guessed_letters[DATA_LENGTH];
	char word[DATA_LENGTH];
}
