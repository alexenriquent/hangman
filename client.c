#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	/* inet_addr */

int main(int argc, char** argv) {

	int socket_desc;
	struct sockaddr_in server;
	char server_message[2000];

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

	while (1) {
		if (recv(socket_desc, server_message, 2000, 0) < 0) {
			puts("reve failed");
			break;
		}
		puts(server_message);
	}

	close(socket_desc);

	return 0;
}