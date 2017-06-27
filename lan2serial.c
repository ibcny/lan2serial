/*
 * ----------------------------------------------------------------------------
 * lan2serial
 * Utility for the transport of data from serial interfaces to TCP/IP or vice versa.
 * 
 * Copyright (C) 2012 Ibrahim Can Yuce <canyuce@gmail.com>
 *
 * ----------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>

#include "lan2serial.h"
#include "serial.h"

/*
 * Listen and accept connections on port given as argument listen_port.
 * If parameter force is TRUE, SO_REUSEADDR option is set for socket *sockfd.
 * Otherwise SO_REUSEADDR option is not set.
 */
static BOOL start_listening(int* sockfd, int listen_port, int backlog,
		BOOL force) {
	int b = -1; 	        // holds return value of bind() call
	int l = -1; 	        // holds return value of listen() call
	struct sockaddr_in my_addr; // my address information

	*sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (sockfd == NULL) {
		fprintf(stderr, "sockfd pointer is unexpectedly null in "
				"lan2serial.c:start_listening(..)\n");
		return (FALSE );
	}

	my_addr.sin_family = AF_INET;              // host byte order
	my_addr.sin_port = htons(listen_port);     // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY;      // auto-fill with my IP
	memset(&(my_addr.sin_zero), EOS, 8);       // zero the rest of the struct

	if (force == TRUE) {
		int yes = 1;

		// set SO_REUSEADDR option before calling bind
		setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &yes,
				sizeof(yes));

		b = bind(*sockfd, (struct sockaddr *) &my_addr,
				sizeof(struct sockaddr));
		if (b < 0) {
			fprintf(stderr, "Couldn't force on bind!\n");
			return (FALSE );
		} else {
			printf("Bound with force option on!\n");
		}
	} else {
		// call bind without setting SO_REUSEADDR option
		b = bind(*sockfd, (struct sockaddr *) &my_addr,
				sizeof(struct sockaddr));

		if (b < 0) {
			fprintf(stderr, "Couldn't bind!\n");
			return (FALSE );
		}
	}

	l = listen(*sockfd, backlog);
	if (l == 0)
		return (TRUE );
	else {
		perror("listen");
		return (FALSE );
	}
}

BOOL modem__connect_server(const char* host, unsigned short port) {
	struct sockaddr_in servaddr;
	struct hostent *he;

	if (host == NULL) {
		printf("Can't connect to server: server host name not specified\n");
		return (FALSE );
	}

	he = gethostbyname(host); /* get the host info */
	if (he == NULL) {
		printf("gethostbyname():%s\n", strerror(errno));
		printf("Couldn't connect\n");
		return (FALSE );
	}

	lanfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lanfd < 0) {
		printf("socket():%s\n", strerror(errno));
		return (FALSE );
	}

	bzero((void *) &servaddr, sizeof(struct sockaddr_in)); /* zero the struct */
	servaddr.sin_family = AF_INET; /* host byte order */
	servaddr.sin_port = htons(port); /* short, network byte order */
	servaddr.sin_addr = *((struct in_addr *) he->h_addr); /* server address */

	if (connect(lanfd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr))
			< 0) {
		printf("connect() on %s:%d failed: %s\n", host, port, strerror(errno));
		close(lanfd);
		return (FALSE );
	}

	printf("connect() on %s:%d successfull (conn_fd=%d)\n", host, port, lanfd);
	return (TRUE );
}

/*
 * A kind of write() wrapper.
 * NOTICE: This is shared between two threads.
 */
static ssize_t writen(int fd, const void *vptr, size_t n) {
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if (fd == comfd) {
			rts_on(comfd);
			nwritten = write2port(fd, ptr, nleft);
			//rts_off(comfd);
		} else {
			nwritten = write(fd, ptr, nleft);
		}
		if ((nwritten <= 0 && fd == lanfd) || (nwritten < 0 && fd == comfd)) {
			if (errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

/*
 * Thread function that writes data to 
 * serial port read from socket
 */
static void * process_lan2serial() {
	fd_set wfdset, rfdset;
	int maxfd = 1;
	int count;
	struct timeval timeout;
	static char buffer[MAX_READ_SIZE];

	while (TRUE ) {
		bzero(buffer, MAX_READ_SIZE);
		timeout.tv_sec = 0L;
		timeout.tv_usec = 0L;

		FD_ZERO(&wfdset);
		FD_ZERO(&rfdset);
		FD_SET(lanfd, &rfdset);

		maxfd += lanfd;
		/* Serial port is opened as non-blocking, so 
		 * no need to check if it will block */
		select(maxfd, &rfdset, 0, 0, NULL);

		if (FD_ISSET(lanfd, &rfdset) != FALSE) {
			count = read(lanfd, buffer, MAX_READ_SIZE);
			if (count == 0) {
				fprintf(stderr, "EOF received from lan!\n");
				exit(-1);
			} else if (count < 0) {
				perror("process_lan2serial()");
			} else {
				if (writen(comfd, buffer, count) < 0)
					perror("process_lan2serial()");
			}
		}
	} // while (TRUE)
	return NULL;
}

/*
 * Thread function that writes data to 
 * socket read from serial port
 */
static void * process_serial2lan() {
	fd_set wfdset, rfdset;
	int maxfd = 1;
	int count;
	struct timeval timeout;
	static char buffer[MAX_READ_SIZE];

	while (TRUE ) {
		bzero(buffer, MAX_READ_SIZE);

		timeout.tv_sec = 0L;
		timeout.tv_usec = 0L;

		FD_ZERO(&wfdset);
		FD_ZERO(&rfdset);
		FD_SET(comfd, &rfdset);

		maxfd += comfd;

		// Socket isn't non-blocking, so why don't we check if it will block?
		// select(maxfd, &rfdset, &wfdset, 0, &timeout);
		select(maxfd, &rfdset, 0, 0, NULL);
		bzero(buffer, MAX_READ_SIZE);

		if (FD_ISSET(comfd, &rfdset) != FALSE) {
			count = read_from_port(comfd, buffer, MAX_READ_SIZE);
			if (count == 0) {
				fprintf(stderr, "EOF received from port!\n");
				exit(-1);
			} else if (count < 0) {
				perror("serial2lan");
			} else {
				if (writen(lanfd, buffer, count) < 0)
					perror("serial2lan");
			}
		}
	} // while (TRUE)
	return NULL;
}

int main(int argc, char **argv) {
	int srv_socket, port;
	char *endptr = NULL;
	struct sockaddr their_addr;
	int sin_size = -1;
	char * serialDevice = NULL;

	if (argc < 3 || argc > 5) {
		USAGE;
		exit(-1);
	}

	if (argc == 3) {
		port = strtol(argv[1], &endptr, 10);
		if (endptr == NULL || *endptr != 0) {
			printf("Invalid port number: %s\n", argv[1]);
			exit(-1);
		}

		if (start_listening(&srv_socket, port, 1, TRUE) == FALSE) {
			perror("start_listening");
			exit(-1);
		}

		sin_size = sizeof(struct sockaddr_in);
		lanfd = accept(srv_socket, &their_addr, &sin_size);

		if (socket < 0) {
			perror("accept");
			exit(-1);
		}

		printf("Connection accepted on port %d\n", port);
		serialDevice = argv[2];
	} else {
		char * host = "127.0.0.1";

		if (strcmp(argv[1], "-c") != 0) {
			USAGE;
			exit(-1);
		}

		if (argc == 4) {
			port = strtol(argv[2], &endptr, 10);
			if (endptr == NULL || *endptr != 0) {
				printf("Invalid port number: %s\n", argv[2]);
				exit(-1);
			}
			serialDevice = argv[3];
		}

		else if (argc == 5) {
			host = argv[2];
			port = strtol(argv[3], &endptr, 10);
			if (endptr == NULL || *endptr != 0) {
				printf("Invalid port number: %s\n", argv[3]);
				exit(-1);
			}
			serialDevice = argv[4];
		}

		if (modem__connect_server(host, port) == FALSE)
			exit(-1);
	}

	open_port(&comfd, serialDevice);
	pthread_create(&t_lan2serial, NULL, process_lan2serial, NULL);
	pthread_create(&t_serial2lan, NULL, process_serial2lan, NULL);

	/* Wait for threads to exit */
	pthread_join(t_lan2serial, NULL);
	pthread_join(t_serial2lan, NULL);

	return 0;
}
