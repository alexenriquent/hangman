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

#define DEFAULT_PORT 12345		/* default port */
#define NUM_HANDLER_THREADS 10	/* number of threads */
#define MAX_NUM_WORDS 2048		/* maximum number of words */
#define MAX_WORD_LENGTH 256		/* maximum word length */
#define USERNAME_LENGTH 16		/* maximum word length for username */
#define PASSWORD_LENGTH 16		/* maximum word length for password */
#define NUM_USERS 10			/* number of users */
#define DATA_LENGTH	2048		/* general data length */

#define MIN(x, y) ((x) < (y) ? x : y) /* determin the minimum value */

typedef int (*compfn)(const void*, const void*);

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

/* global mutex */
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* global condition variable */
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

int num_requests = 0; /* number of pending request, initially none */

struct request* requests = NULL;		/* head of linked list of requests */
struct request* last_request = NULL;	/* pointer to the last request */

struct user users[NUM_USERS];			/* user records */

/* 
 * main function 
 */
int main(int argc, char** argv) {

	system("clear");

	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;

	int thr_id[NUM_HANDLER_THREADS];			/* thread IDs */
	pthread_t p_threads[NUM_HANDLER_THREADS];	/* thread's structures */

	signal(SIGINT, sigint_handler);

	/* create socket */
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_desc == -1) {
		perror("socket");
		exit(1);
	}
	puts("Socket created");

	/* configuring end point */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	/* Check command line arguments */
	if (argc == 2) {
		server.sin_port = htons(atoi(argv[1]));
		printf("Listening on port %d\n", atoi(argv[1]));
	} else if (argc == 1) {
		server.sin_port = htons(DEFAULT_PORT);
		printf("Listening on port 12345\n");
	} else {
		fprintf(stderr, "usage: port number\n");
		exit(1);
	}

	/* bind */
	if (bind(socket_desc, (struct sockaddr *)&server,
		sizeof(server)) < 0) {
		perror("bind");
		exit(1);
	}
	puts("Bind done");

	/* listen */
	listen(socket_desc, 10);

	/* create threads */
	for (int i = 0; i < NUM_HANDLER_THREADS; i++) {
		thr_id[i] = i;
		pthread_create(&p_threads[i], NULL, 
			handle_requests_loop, (void*)&thr_id[i]);
	}

	/* tokenise users */
	tokenise_auth(read_file("Authentication.txt").words);

	/* accept an incomming connection */
	puts("Waiting for incomming connection...");
	c = sizeof(struct sockaddr_in);

	/* accept connection from an incomming client */
	while (client_sock = accept(socket_desc,
		(struct sockaddr *)&client, (socklen_t*)&c)) {
		puts("Connection accepted");

		add_request(client_sock, &request_mutex, &got_request);
	}

	if (client_sock < 0) {
		perror("accept");
		exit(1);
	}
	
	close(socket_desc);

	return EXIT_SUCCESS;
}

/*
 * function add_request(): add a request to the requests list
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    none.
 */
void add_request(int client_socket,
            pthread_mutex_t* p_mutex,
            pthread_cond_t*  p_cond_var) {
  	int rc;                         /* return code of pthreads functions */
  	struct request* a_request;      /* pointer to newly added request */

  	/* create structure with new request */
 	 a_request = (struct request*)malloc(sizeof(struct request));
  	if (!a_request) { /* malloc failed?? */
     	fprintf(stderr, "add_request: out of memory\n");
      	exit(1);
  	}
 	a_request->socket = client_socket;
  	a_request->next = NULL;

  	/* lock the mutex, to assure exclusive access to the list */
  	rc = pthread_mutex_lock(p_mutex);

  	/* add new request to the end of the list, updating list */
  	/* pointers as required */
  	if (num_requests == 0) { /* special case - list is empty */
      	requests = a_request;
      	last_request = a_request;
  	} else {
      	last_request->next = a_request;
      	last_request = a_request;
  	}

  	/* increase total number of pending requests by one */
  	num_requests++;

  	/* unlock mutex */
  	rc = pthread_mutex_unlock(p_mutex);

  	/* signal the condition variable - there's a new request to handle */
  	rc = pthread_cond_signal(p_cond_var);
}

/*
 * function get_request(): gets the first pending request from the requests list
 *                         removing it from the list.
 * algorithm: creates a request structure, adds to the list, and
 *            increases number of pending requests by one.
 * input:     request number, linked list mutex.
 * output:    pointer to the removed request, or NULL if none.
 * memory:    the returned request need to be freed by the caller.
 */
struct request* get_request(pthread_mutex_t* p_mutex) {
  	int rc;                         /* return code of pthreads functions */
  	struct request* a_request;      /* pointer to request */

  	/* lock the mutex, to assure exclusive access to the list */
  	rc = pthread_mutex_lock(p_mutex);

  	if (num_requests > 0) {
    	a_request = requests;
    	requests = a_request->next;
    	if (requests == NULL) { /* this was the last request on the list */
      		last_request = NULL;
   	 	}
      	/* decrease the total number of pending requests */
     	num_requests--;
    } else { /* requests list is empty */
      	a_request = NULL;
    }

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* return the request to the caller */
    return a_request;
}

/*
 * function handle_request(): handle a single given request.
 * algorithm: prints a message stating that the given thread handled
 *            the given request.
 * input:     request pointer, id of calling thread.
 * output:    none.
 */
void handle_request(struct request* a_request, int thread_id) {
    int client_signal;
    char credential[MAX_WORD_LENGTH];

    if (a_request) {
      	printf("Thread '%d' handled request of socket '%d'\n",
            	thread_id, a_request->socket);
      	printf("Current number of requests: %d\n", num_requests);
      	fflush(stdout);
    }

    if (authenticate(a_request->socket, credential)) {
    	do {
    		client_signal = receive_int(a_request->socket);
			switch (client_signal) {
				case 1:
					play_hangman(a_request->socket, credential);
					fflush(stdout);
					break;
				case 2:
					leaderboard(a_request->socket);
					fflush(stdout);
					break;
			}
    	} while (client_signal != 3);
    }
}

/*
 * function handle_requests_loop(): infinite loop of requests handling
 * algorithm: forever, if there are requests to handle, take the first
 *            and handle it. Then wait on the given condition variable,
 *            and when it is signaled, re-do the loop.
 *            increases number of pending requests by one.
 * input:     id of thread, for printing purposes.
 * output:    none.
 */
void* handle_requests_loop(void* data) {
    int rc;                         /* return code of pthreads functions */
    struct request* a_request;      /* pointer to a request */
    int thread_id = *((int*)data);  /* thread identifying number */

    /* lock the mutex, to access the requests list exclusively */
    rc = pthread_mutex_lock(&request_mutex);

    /* do forever */
    while (1) {
        if (num_requests > 0) { /* a request is pending */
            a_request = get_request(&request_mutex);
            if (a_request) { /* got a request - handle it and free it */
                /* unlock mutex - so other threads would be able to handle */
                /* other reqeusts waiting in the queue paralelly */
                rc = pthread_mutex_unlock(&request_mutex);
                handle_request(a_request, thread_id);
                free(a_request);
                /* and lock the mutex again */
                rc = pthread_mutex_lock(&request_mutex);
            }
        } else {
            /* wait for a request to arrive. the mutex will be 		*/
            /* unlocked here, thus allowing other threads access to */
            /* requests list.*/

            rc = pthread_cond_wait(&got_request, &request_mutex);
            /* and after returning from pthread_cond_wait, the mutex */
            /* is locked again. */
        }
    }
}

void sigint_handler(int sig) {
	puts("SIGINT\n");
	exit(sig);
}

void send_string(int client_socket, char buffer[]) {
	if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
		puts("send failed");
		return;
	}
}

void send_int(int client_socket, int integer) {
	if (send(client_socket, &integer, sizeof(integer), 0) < 0) {
		puts("send failed");
		return;
	}
}

void receive_string(int client_socket, char buffer[]) {
	while (1) {
		if (recv(client_socket, buffer, DATA_LENGTH, 0) < 0) {
			puts("recv failed");
			break;
		} else {
			break;
		}
	}
}

int receive_int(int client_socket) {
	int integer;

	while (1) {
		if (recv(client_socket, &integer, sizeof(integer), 0) < 0) {
			puts("recv failed");
			break;
		} else {
			break;
		}
	}
	return integer;
}

void clear_buffer(char buffer[]) {
	memset(buffer, 0, sizeof(buffer));
}

struct data read_file(char* filename) {
	int line_index = 0;
	struct data lines;
    
    FILE *file = fopen(filename, "r");
    char line[MAX_WORD_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
    	char* pc;
        while (pc = strpbrk(line, "\n\r")) {
            *pc = 0;
        }
    	if (line_index != 0) {
    		strcpy(lines.words[line_index - 1], line);
    	}
        line_index++;
    }

    fclose(file);
    return lines;
}

struct data read_words(char* filename) {
	int line_index = 0;
	struct data lines;
    
    FILE *file = fopen(filename, "r");
    char line[MAX_WORD_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
    	char* pc;
        while (pc = strpbrk(line, "\n\r")) {
            *pc = 0;
        }
    	strcpy(lines.words[line_index], line);
        line_index++;
    }

    fclose(file);
    return lines;
}

void tokenise_auth(char word_list[][MAX_WORD_LENGTH]) {
	for (int i = 0; i < NUM_USERS; i++) {
    	int separater = 0;
    	char* token = strtok(word_list[i], " \t");
    	while(token != NULL) {
    		if (separater == 0) {
    			strcpy(users[i].username, token);
    		} else {
    			strcpy(users[i].password, token);
    		}
    		separater++;
    		token = strtok(NULL, " \t");
    	}
    }
}

bool authenticate(int client_socket, char credential[]) {
	int response;
	char username[USERNAME_LENGTH];
	char client_username[DATA_LENGTH];
	char client_password[DATA_LENGTH];
	bool valid;

	// for (int i = 0; i < NUM_USERS; i++) {
 //    	printf("username %d: %s\t", i, users[i].username);
 //    	printf("password %d: %s\n", i, users[i].password);
 //    }

    receive_string(client_socket, client_username);
	receive_string(client_socket, client_password);
	strcpy(credential, client_username);
	printf("%s\n", client_username);
	printf("%s\n", client_password);

	for (int i = 0; i < NUM_USERS; i++) {
		if (strcmp(client_username, users[i].username) == 0 &&
			strcmp(client_password, users[i].password) == 0) {
			valid = true;
			break;
		} else {
			valid = false;
		}
	}
	if (valid) {
		response = 1;
	} else {
		response = 0;
	}

	send_int(client_socket, response);
	clear_buffer(client_username);
	clear_buffer(client_password);

	return valid;
}

void play_hangman(int client_socket, char credential[]) {
	int word_length, num_guesses, sig;
	char rand_word[MAX_WORD_LENGTH];
	char guessed_letters[DATA_LENGTH];
	char word[DATA_LENGTH];
	char letter[DATA_LENGTH];
	char* found;

	strcpy(rand_word, read_words("hangman_text.txt").words[0]);
	word_length = strlen(rand_word);

	num_guesses = MIN(word_length + 10, 26);
	send_int(client_socket, num_guesses);

	while (sig != 1 && num_guesses > 0) {
        word[0] = '\0';
		for (int i = 0; i < word_length; i++) {
        	bool flag = false;
        	for (int j = 0; j < strlen(guessed_letters); j++) {
            	if (rand_word[i] == guessed_letters[j]) {
                	rand_word[i] = guessed_letters[j];
                	char c[2] = {rand_word[i], '\0'};
                	strcat(word, c);
                	strcat(word, " ");
                	flag = true;
                	break;
            	}
       		}
        	if (!flag) {
            	if (rand_word[i] != ',') {
                	strcat(word, "_ ");
            	} else {
                	strcat(word, "  ");
            	}
        	}
    	}

    	printf("%s\n", word);
    	printf("%s\n", guessed_letters);  

    	found = strchr(word, '_');
    	if (found) {
    		sig = 0;
    	} else {
    		sig = 1;
    	}
    	
    	send_int(client_socket, sig);
    	send_string(client_socket, word);

    	if (sig != 1) {
    		receive_string(client_socket, letter);
    		if (strcmp(letter, "") == 0) {
    			break;
    		}
    		strcat(guessed_letters, letter);
    		clear_buffer(letter); 
    		num_guesses--;
    	}
	}

	if (sig == 1) {
		increment_num_games_won(credential);
	}

	sig = 0;
	increment_num_games_played(credential);
	clear_buffer(guessed_letters);
	clear_buffer(word);
}

void leaderboard(int client_socket) {
	char leaderboard_data[DATA_LENGTH * 10];
	char games_won[MAX_WORD_LENGTH];
	char games_played[MAX_WORD_LENGTH];

	qsort((void *)&users, NUM_USERS, sizeof(struct user), (compfn)compare);

	leaderboard_data[0] = ' ';
	for (int i = 0; i < NUM_USERS; i++) {
		if (users[i].num_games_played > 0) {
			sprintf(games_won, "%d", users[i].num_games_won);
			sprintf(games_played, "%d", users[i].num_games_played);
			strcat(leaderboard_data, "\n========================================\n\n");
			strcat(leaderboard_data, "Player - ");
			strcat(leaderboard_data, users[i].username);
			strcat(leaderboard_data, "\nNumber of games won - ");
			strcat(leaderboard_data, games_won);
			strcat(leaderboard_data, "\nNumber of games played - ");
			strcat(leaderboard_data, games_played);
			strcat(leaderboard_data, "\n\n========================================\n");
		}
	}

	if (strcmp(leaderboard_data, " ") == 0) {
		strcat(leaderboard_data, "\n\nThere is no information currently stored in the Leader Board.\n");
		strcat(leaderboard_data, "Try again later.\n\n");
	}

	// printf("%s\n", leaderboard_data);

	send_string(client_socket, leaderboard_data);
	clear_buffer(leaderboard_data);
}

int compare(struct user* user1, struct user* user2) {
	double percentage1, percentage2;

	percentage1 = (double)user1->num_games_won / (double)user1->num_games_played;
	percentage2 = (double)user2->num_games_won / (double)user2->num_games_played;

	if (user1->num_games_won < user2->num_games_won) {
		return -1;
	} else if (user1->num_games_won > user2->num_games_won) {
		return 1;
	} else {
		if (percentage1 < percentage2) {
			return -1;
		} else if (percentage1 > percentage2) {
			return 1;
		} else {
			return 0;
		}
	}
}

int get_num_games_won(char credential[]) {
	int result;
	for (int i = 0; i < NUM_USERS; i++) {
		if (strcmp(credential, users[i].username) == 0) {
			result = users[i].num_games_won;
			break;
		}
	}
	return result;
}

int get_num_games_played(char credential[]) {
	int result;
	for (int i = 0; i < NUM_USERS; i++) {
		if (strcmp(credential, users[i].username) == 0) {
			result = users[i].num_games_played;
			break;
		}
	}
	return result;
}

void increment_num_games_won(char credential[]) {
	for (int i = 0; i < NUM_USERS; i++) {
		if (strcmp(credential, users[i].username) == 0) {
			users[i].num_games_won++;
			break;
		}
	}
}

void increment_num_games_played(char credential[]) {
	for (int i = 0; i < NUM_USERS; i++) {
		if (strcmp(credential, users[i].username) == 0) {
			users[i].num_games_played++;
			break;
		}
	}
}

bool statistic_available() {
	for (int i = 0; i < NUM_USERS; i++) {
		if (users[i].num_games_played > 0) {
			return true;
		}
	}
	return false;
}
