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

#define MIN(x, y) ((x) < (y) ? x : y) /* determin the minimum value */

/* function prototypes */
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
			system("clear");
			welcome();
			option = menu();

			switch(option) {
				case 1:
					printf("Option 1");
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

void welcome() {
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

void play(int socket) {
	char letter[WORD_LENGTH];
	char guessed_letters[2048];
	char word[2048];
	int word_length, num_guesses;
	bool win = false;
	char* pch;

	while (1) {
		if (recv(socket, &word_length, sizeof(word_length), 0) < 0) {
			puts("recv failed");
			break;
		} else {
			num_guesses = MIN(word_length + 10, 26);
			break;
		}
	}

	while (num_guesses > 0 || !win) {
		while (1) {
			if (recv(socket, guessed_letters, 2048, 0) < 0) {
				puts("recv failed");
				break;
			} else {
				printf("\nGuessed letters: %s\n", guessed_letters);
				break;
			}
		}

		printf("Number of guesses left: %d\n", num_guesses);

		while (1) {
			if (recv(socket, word, 2048, 0) < 0) {
				puts("recv failed");
				break;
			} else {
				printf("\nWord: %s\n", word);
				break;
			}
		}

		pch = strchr(word, '_');
		if (pch == NULL) {
			win = true;
		}

		printf("Enter your guess: ");
		scanf("%s", letter);
		num_guesses--;

		if (send(socket, letter, strlen(letter), 0) < 0) {
			puts("send failed");
			break;
		}
	}
}
