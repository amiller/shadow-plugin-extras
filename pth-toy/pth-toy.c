/*
 * See LICENSE for licensing information
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <shd-library.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include "pth-toy.h"

static ShadowLogFunc slogf;

#define LOG_MESSAGE(...) slogf(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__, __VA_ARGS__)
#define LOG_CRITICAL(...) slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__, __VA_ARGS__)

extern void toy_pthread_doWork();

#include <stdio.h>

static const char MESSAGE_STRING[] = "Hello World!";

int server(const char *path) {
    unlink(path);

    int sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sd == -1) {
	LOG_CRITICAL("unable to create socket: %s", strerror(errno));
	return -1;
    }

    /* setup the socket address info, client has outgoing connection to server */
    struct sockaddr_un bindAddress;
    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sun_family = AF_UNIX;
    strcpy(bindAddress.sun_path, path);

    /* bind the socket to the server port */
    int res = bind(sd, (struct sockaddr *) &bindAddress, sizeof(bindAddress));
    if (res == -1) {
	LOG_CRITICAL("unable to start server: error in bind");
	return -1;
    }

    /* set as server socket that will listen for clients */
    res = listen(sd, 100);
    if (res == -1) {
	LOG_CRITICAL("unable to start server: error in listen");
	return -1;
    }

    int client = accept(sd, 0, 0);
    if (client < 0) {
	LOG_CRITICAL("unable to start server: error in accept");
	return -1;
    }

    /* send a message */
    static char buf[sizeof(MESSAGE_STRING)];
    int rc = recv(client, buf, sizeof(MESSAGE_STRING), 0);
    if (strcmp(MESSAGE_STRING, buf)) {
	LOG_CRITICAL("server: unexpected message received: %s", buf);
    }

    LOG_MESSAGE("Message received: [%s]\n", buf);
    close(client);
    close(sd);
    return 0;
}

int client(const char *path) {
    usleep(100*1000);

    /* create the client socket and get a socket descriptor */
    int sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sd == -1) {
	LOG_CRITICAL("unable to create socket: %s", strerror(errno));
	return -1;
    }

    /* our client socket address information for connecting to the server */
    struct sockaddr_un serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sun_family = AF_UNIX;
    strcpy(serverAddress.sun_path, path);

    /* connect to server. since we are non-blocking, we expect this to return EINPROGRESS */
    int res = connect(sd,(struct sockaddr *)  &serverAddress, sizeof(serverAddress));
    if (res < 0){
	LOG_CRITICAL("unable to start client: error in connect");
	return -1;
    }

    /* send a message */
    int rc = send(sd, MESSAGE_STRING, sizeof(MESSAGE_STRING), 0);
    if (rc != sizeof(MESSAGE_STRING)) {
	LOG_CRITICAL("unable to send message");
	return -1;
    }
    close(sd);
    return 0;
}

int plugin_main(int argc, char* argv[], ShadowLogFunc slogf_) {
        slogf = slogf_;

	pthread_t serv, clnt;
	const char *path = "/tmp/pth-toy-socket";
	pthread_create(&serv, NULL, (void * (*) (void *)) server, (void *) path);
	pthread_create(&clnt, NULL, (void * (*) (void *)) client, (void *) path);
	pthread_join(serv, NULL);
	pthread_join(clnt, NULL);

	return 0;
}
