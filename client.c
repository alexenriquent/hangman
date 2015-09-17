#include <stdio.h>		/* standard I/O routines */
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>	/* inet_addr */

int main(int argc, char** argv) {

	int socket_desc;
	struct sockaddr_in server;
	char server_message[2000];

	/* craete socket */
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_desc == -1) {
		puts("Could not create socket");
	}
	puts("Socket created");

	/* configuring end point */
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(8888);

	/* connect to remote server */
	if (connect(socket_desc, (struct sockaddr *)&server,
		sizeof(server)) < 0) {
		perror("connect");
		return 1;
	}
	puts("Connected\n");

	while (1) {
		if (recv(socket_desc, server_message, 2000, 0) < 0) {
			puts("reve failed");
			break;
		}
		puts(server_message);
	}

	// close(socket_desc);

	return 0;
}