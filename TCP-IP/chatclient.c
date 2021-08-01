/*Edward Yaroslavsky and Jared Follet
I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1; 
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() {
	int msg = get_string(outbuf, MAX_MSG_LEN);
	if (msg == TOO_LONG) {
		fprintf(stderr, "Sorry, limit your message to %d characters.\n", MAX_MSG_LEN);
		return EXIT_FAILURE;
	}
	else if (send(client_socket, outbuf, strlen(outbuf), 0) < 0) {
		fprintf(stderr, "Error: Failed to send message to server. %s.\n", strerror(errno));
	}

	if (strcmp(outbuf, "bye") == 0) {
		printf("%s\n", "Goodbye.");
		close(client_socket);
		exit(-1);
		return -1;
	}

	return EXIT_SUCCESS;
} 

int handle_client_socket() {
	ssize_t bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0);
	if (bytes_recvd < 0) {
		fprintf(stderr, "Warning: Failed to receive incoming message. %s.", strerror(errno));
	}
	else if (bytes_recvd == 0) {
		fprintf(stderr, "\nConnection to server has been lost.\n");
		close(client_socket);
		return EXIT_FAILURE;
	}
	else {
		inbuf[bytes_recvd] = '\0';
		if (strcmp(inbuf, "bye") == 0) {
			printf("\nServer initiated shutdown.\n");
			return -1;
		}
		
		printf("\n");
		printf("%s\n", inbuf);
	}

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage: %s <server IP> <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	struct sockaddr_in serv_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, addrlen);
	serv_addr.sin_family = AF_INET;

	int ip_conversion = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
	if (ip_conversion <= 0) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    int port_num = 0;
    if (parse_int(argv[2], &port_num, "port number")) {
    	if (!(port_num >= 1024 && port_num <= 65535)) {
    		fprintf(stderr, "Error: Port must be in range [1024, 65535].\n");
    		return EXIT_FAILURE;
    	}
    }
    else {
    	return EXIT_FAILURE;
    }
    serv_addr.sin_port = htons(port_num);

    //memset(&username, 0, MAX_NAME_LEN);
    while (strlen(username) <= 1) {
    	printf("Enter your username: ");
		fflush(stdout);
		int usernameLength = get_string(username, MAX_NAME_LEN + 1); 
    	/*if (strlen(username) > MAX_NAME_LEN + 1) {
    		printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);    	
    		memset(&username, 0, MAX_NAME_LEN);	
    	}*/
    	if (usernameLength == NO_INPUT) {
    		continue;
    	}
    	else if (usernameLength == TOO_LONG) {
    		fprintf(stderr, "Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
			memset(&username, 0, MAX_NAME_LEN+1);
    		continue;
    	}
    }
    username[strlen(username)] = '\0';

    printf("Hello, %s. Let's try to connect to the server.\n", username);

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n",
            strerror(errno));
        return EXIT_FAILURE;
    }

    if (connect(client_socket, (struct sockaddr *)&serv_addr, addrlen) < 0) {
		fprintf(stderr, "Error: Failed to connect to server. %s.\n",
			strerror(errno));
		return EXIT_FAILURE;
	}

	int bytes_recvd;
	if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN-1, 0)) < 0) {
		fprintf(stderr, "Error: Failed to receive message from server. %s.\n",
			strerror(errno));
		return EXIT_FAILURE;
	}
	else if (bytes_recvd == 0) {
		fprintf(stderr, "All connections are busy. Try again later.\n");
		return EXIT_FAILURE;
	}
	inbuf[bytes_recvd] = '\0';

	printf("\n");
	printf("%s\n", inbuf);
	printf("\n");

	if (send(client_socket, username, strlen(username), 0) < 0) {
		fprintf(stderr, "Error: Failed to send username to server. %s.\n",
			strerror(errno));
		return EXIT_FAILURE;
	}

	fd_set chatOutput;
	//fd_set chatInput;

	FD_ZERO(&chatOutput);
	FD_SET(STDIN_FILENO, &chatOutput);

	//FD_ZERO(&chatInput);
	//FD_SET(client_socket, &chatInput);
	
	//int intErr;
	int selectVal;
	while (true) {		
		printf("[%s]: ", username);
		fflush(stdout);

		FD_ZERO(&chatOutput);
		FD_SET(STDIN_FILENO, &chatOutput);
		FD_SET(client_socket, &chatOutput);
		if ((selectVal = select(client_socket + 1, &chatOutput, NULL, NULL, NULL)) == -1) {
			//fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
			return EXIT_FAILURE;
		}
		
		//select(client_socket + 1, &chatOutput, NULL, NULL, NULL);

		int handle;
		if (FD_ISSET(STDIN_FILENO, &chatOutput)) {
			if ((handle = handle_stdin()) == EXIT_FAILURE) {
				break;
			}
		}
		else if (FD_ISSET(client_socket, &chatOutput)) {
			if ((handle = handle_client_socket()) == -1 || handle == EXIT_FAILURE) {
				break;
			}
		}
	}

	/*if (fcntl(client_socket, F_GETFD) >= 0) {
		close(client_socket);
	}*/

	close(client_socket);
    return EXIT_SUCCESS;
} 