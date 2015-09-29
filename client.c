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

int socket_desc;	/* socket descriptor */

/* 
 * main function 
 */
int main(int argc, char** argv) {

	int read_size;
	struct sockaddr_in server;
	int option;
	char credential[DATA_LENGTH];

	/* SIGINT handler */
	signal(SIGINT, sigint_handler);

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
	clear_screen();
	welcome();

	if (logon(socket_desc, credential)) {
		clear_screen();
		welcome();
		do {
			option = menu();
			send_int(socket_desc, option);
			switch(option) {
				case 1:
					clear_screen();
					play(socket_desc, credential);
					break;
				case 2:
					clear_screen();
					leaderboard(socket_desc);
					break;
			}
		} while (option != 3);
	} else {
		printf("\nYou entered either an incorrect username or password - disconnecting\n");
	}

	close(socket_desc);

	return EXIT_SUCCESS;
}

/*
 * function sigint_handler(): the SIGINT handler.
 * algorithm: gracefully exit the program after receiving
 			  SIGINT (Ctrl-C).
 * input:     a signal received.
 * output:    none.
 */
void sigint_handler(int sig) {
	puts("SIGINT\n");
	close(socket_desc);
	exit(sig);
}

/*
 * function send_string(): send a string to a connected client.
 * algorithm: send a string to a client specified by the socket 
 			  descriptor. If there is an error, return immediately.
 * input:     client socket descriptor, string buffer.
 * output:    none.
 */
void send_string(int socket, char buffer[]) {
	if (send(socket, buffer, strlen(buffer), 0) < 0) {
		puts("send failed");
		return;
	}
}

/*
 * function send_int(): send an integer to a connected client.
 * algorithm: send an integer to a client specified by the socket 
 			  descriptor. If there is an error, return immediately.
 * input:     client socket descriptor, integer.
 * output:    none.
 */
void send_int(int socket, int integer) {
	if (send(socket, &integer, sizeof(integer), 0) < 0) {
		puts("send failed");
		return;
	}
}

/*
 * function receive_string(): receive a string from a connected client.
 * algorithm: receive a string from a client specified by the socket 
 			  descriptor. The function will wait indefinitely until it 
 			  gets a reply. If there is an error, break out of the loop 
 			  and return immediately.
 * input:     client socket descriptor, string buffer.
 * output:    none.
 */
void receive_string(int socket, char buffer[]) {
	int read_size;

	while (1) {
		if ((read_size = recv(socket, buffer, DATA_LENGTH - 1, 0)) < 0) {
			puts("recv failed");
			break;
		} else {
			break;
		}
	}

	/* put the null teminator at the end of the string */
	if (read_size > 0) {
		buffer[read_size] = '\0';
	}
}

/*
 * function receive_int(): receive an integer from a connected client.
 * algorithm: receive an integer from a client specified by the socket 
 			  descriptor. The function will wait indefinitely until it 
 			  gets a reply. If there is an error, break out of the loop 
 			  and return immediately.
 * input:     client socket descriptor.
 * output:    integer.
 */
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

/*
 * function clear_buffer(): clear a buffer.
 * algorithm: cleanup a specified buffer variable.
 * input:     string buffer.
 * output:    none.
 */
void clear_buffer(char buffer[]) {
	memset(buffer, 0, sizeof(buffer));
}

/*
 * function welcome(): welcome message.
 * algorithm: display a welcome message.
 * input:     none.
 * output:    none.
 */
void welcome() {
	printf("\n===========================================");
	printf("\n\nWelcome to the Online Hangman Game System\n\n");
	printf("===========================================\n");
}

/*
 * function menu(): main menu.
 * algorithm: display the main menu.
 * input:     none.
 * output:    selected option.
 */
int menu() {
	int opt, temp, status;

	printf("\nPlease enter a selection\n");
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

/*
 * function logon(): authentication process.
 * algorithm: authenticate the current user by sending username 
 			  and password to the server.
 * input:     server socket descriptor, credential.
 * output:    true if the current user is successfully authenticated,
 			  false otherwise.
 */
bool logon(int socket, char credential[]) {
	int server_signal;
	char username[USERNAME_LENGTH];
	char password[PASSWORD_LENGTH];
	bool valid;

	printf("\n\nYou are required to logon with ");
	printf("your registered username and password\n");

	printf("\nPlease enter your username: ");
	scanf("%s", username);
	send_string(socket, username);
	strcpy(credential, username);

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

/*
 * function play_hangman(): play the game of Hangman.
 * algorithm: process the game of Hangman.
 * input:     server socket descriptor, credential.
 * output:    none.
 */
void play(int socket, char credential[]) {
	int num_guesses, sig;
	char letter[DATA_LENGTH];
	char guessed_letters[DATA_LENGTH];
	char word[DATA_LENGTH];
	char* found;

	num_guesses = receive_int(socket);

	while (sig != 1 && num_guesses > 0) {
		printf("\nGuessed letters: %s\n\n", guessed_letters);
		printf("Number of guesses left: %d\n\n", num_guesses);
		sig = receive_int(socket);
		receive_string(socket, word);
		printf("Word: %s\n\n", word);
		if (sig != 1) {
			printf("Enter your guess: ");
        	scanf("%s", letter);
        	while (strlen(letter) > 1) {
        		printf("Please enter only one charecter.\n");
        		printf("Enter your guess: ");
        		scanf("%s", letter);
        	}
        	send_string(socket, letter);
        	strcat(guessed_letters, letter);
        	printf("\n-----------------------------\n\n");
        	num_guesses--;
		}
	}

	printf("Game over\n\n");
	if (sig == 1) {
		printf("Well done %s! You won this round of Hangman!\n\n", credential);
	} else {
		printf("Bad luck %s! You have run out of guesses. ", credential);
		printf("The Hangman got you!\n");
	}

	sig = 0;
	clear_buffer(guessed_letters);
}

/*
 * function leaderboard(): receive leaderboard.
 * algorithm: receive and display the current leaderboard.
 * input:     server socket descriptor.
 * output:    none.
 */
void leaderboard(int socket) {
	char leaderboard_data[DATA_LENGTH * 10];

	receive_string(socket, leaderboard_data);
	printf("%s\n", leaderboard_data);
	clear_buffer(leaderboard_data);
}

/*
 * function clear_screen(): clear the console terminal screen.
 * algorithm: clear the console terminal screen.
 * input:     none.
 * output:    none.
 */
void clear_screen() {
	system("clear");
}
