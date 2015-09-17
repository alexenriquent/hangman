#define _GNU_SOURCE
#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>	/* pthread functions and data structure */
#include <sys/socket.h>
#include <arpa/inet.h>	/* inet_addr */

/* default port */
#define DEFAULT_PORT 12345
/* number of threads used to service requests */
#define NUM_HANDLER_THREADS 3

/* function prototype */
void add_request(int client_socket, 
			pthread_mutex_t* p_mutex, 
			pthread_cond_t*  p_cond_var);
struct request* get_request(pthread_mutex_t* p_mutex);
void handle_request(struct request* a_request, int thread_id);
void* handle_requests_loop(void* data);

/* global mutex */
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* global condition variable */
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

int num_requests = 0; /* number of pending request, initially none */

/* structure of a single request */
struct request {
	int socket;					/* socket descriptor 	   */
	struct request* next;		/* pointer to next request */
};

struct request* requests = NULL;		/* head of linked list of requests */
struct request* last_request = NULL;	/* pointer to the last request 	   */

/* 
 * main function 
 */
int main(int argc, char** argv) {

	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;

	int thr_id[NUM_HANDLER_THREADS];			/* thread IDs 		   */
	pthread_t p_threads[NUM_HANDLER_THREADS];	/* thread's structures */
	struct timespec delay;

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
	listen(socket_desc, 3);

	/* create threads */
	for (int i = 0; i < NUM_HANDLER_THREADS; i++) {
		thr_id[i] = i;
		pthread_create(&p_threads[i], NULL, 
			handle_requests_loop, (void*)&thr_id[i]);
	}

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

	return 0;
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
  	struct request* a_request;      /* pointer to newly added request 	 */

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
  	struct request* a_request;      /* pointer to request 				 */

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
	char* message = "Greeting from server\n";

	write(a_request->socket, message, strlen(message));

    if (a_request) {
      	printf("Thread '%d' handled request of socket '%d'\n
      			Current number of requests: %d\n",
            	thread_id, a_request->socket, num_requests);
      	fflush(stdout);
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
    struct request* a_request;      /* pointer to a request 			 */
    int thread_id = *((int*)data);  /* thread identifying number 	   	 */

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
            /* requests list.                                       */

            rc = pthread_cond_wait(&got_request, &request_mutex);
            /* and after returning from pthread_cond_wait, the mutex */
            /* is locked again.										 */
        }
    }
}
