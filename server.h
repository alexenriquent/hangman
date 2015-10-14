#ifndef __SERVER__
#define __SERVER__

#define _GNU_SOURCE
#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <stdbool.h>	/* boolean */
#include <unistd.h>
#include <string.h>
#include <pthread.h>	/* pthread functions and data structure */
#include <sys/socket.h>	/* BSD socket */
#include <arpa/inet.h>	/* inet_addr */
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#define DEFAULT_PORT 12345		/* default port */
#define NUM_HANDLER_THREADS 10	/* number of threads */
#define MAX_NUM_WORDS 2048		/* maximum number of words */
#define MAX_WORD_LENGTH 256		/* maximum word length */
#define USERNAME_LENGTH 16		/* maximum word length for username */
#define PASSWORD_LENGTH 16		/* maximum word length for password */
#define NUM_USERS 10			/* number of users */
#define DATA_LENGTH	2048		/* general data length */

#define MIN(x, y) ((x) < (y) ? x : y) /* determin the minimum value */

/* typedef for struct comparison */
typedef int (*compfn)(const void*, const void*);
int login_status;
/* structure of a single request */
struct request {
	int socket;					/* socket descriptor */
	struct request* next;		/* pointer to next request */
};

/* structure of a user */
struct user {
	char username[USERNAME_LENGTH]; 	/* username */
	char password[PASSWORD_LENGTH];		/* password */
	int num_games_won;					/* number of games won */
	int num_games_played;				/* number of games played */
	int login_status;					/*user login status*/
};

/* structure of data read from file */
struct data {
	char words[MAX_NUM_WORDS][MAX_WORD_LENGTH]; 	/* data from file */ 
};

/* function prototypes */
void add_request(int client_socket, 
			pthread_mutex_t* p_mutex, 
			pthread_cond_t*  p_cond_var);
struct request* get_request(pthread_mutex_t* p_mutex);
void handle_request(struct request* a_request, int thread_id);
void* handle_requests_loop(void* data);
void sigint_handler(int sig);
void send_string(int client_socket, char buffer[]);
void send_int(int client_socket, int integer);
void receive_string(int client_socket, char buffer[]);
int receive_int(int client_socket);
void clear_buffer(char buffer[]);
struct data read_file(char* filename);
struct data read_words(char* filename);
void tokenise_auth(char word_list[][MAX_WORD_LENGTH]);
bool authenticate(int client_socket, char credential[]);
void play_hangman(int client_socket, char credential[]);
void leaderboard(int client_socket);
int compare(struct user* user1, struct user* user2);
int get_num_games_won(char credential[]);
int get_num_games_played(char credential[]);
void increment_num_games_won(char credential[]);
void increment_num_games_played(char credential[]);
bool statistic_available();
void lowercase(char word[]);
void clear_screen();
bool verify_user(char username[]);
void logout_verified(int client_socket);

#endif 