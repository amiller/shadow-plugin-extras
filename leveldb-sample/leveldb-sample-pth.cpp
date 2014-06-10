/*
 * See LICENSE for licensing information
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <shd-library.h>
#include <sys/stat.h>

#include <pth.h>

typedef struct _Hello Hello;

extern "C" Hello* hello_new(int argc, char* argv[], ShadowLogFunc slogf);
extern "C" void hello_free(Hello* h);
extern "C" void hello_ready(Hello* h);
extern "C" int hello_getEpollDescriptor(Hello* h);
extern "C" int hello_isDone(Hello* h);

// Extern for the thread toy
extern void doWork(void);

#include <string.h>

static int *global_ed;

#define HELLO_PORT 12345

/* all state for hello is stored here */
struct _Hello {
	/* the function we use to log messages
	 * needs level, functionname, and format */
	ShadowLogFunc slogf;

	/* the epoll descriptor to which we will add our sockets.
	 * we use this descriptor with epoll to watch events on our sockets. */
	int ed;

	/* track if our client got a response and we can exit */
	int isDone;

	struct {
		int sd;
		char* serverHostName;
		in_addr_t serverIP;
	} client;

	struct {
		int sd;
	} server;
};

/* if option is specified, run as client, else run as server */
static const char* USAGE = "USAGE: hello [hello_server_hostname]\n";

static int _hello_startClient(Hello* h) {
	//h->client.serverHostName = strndup(serverHostname, (size_t)50);

	/* get the address of the server */
	struct addrinfo* serverInfo;
	int res = getaddrinfo(h->client.serverHostName, NULL, NULL, &serverInfo);
	if(res == -1) {
		h->slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"unable to start client: error in getaddrinfo");
		return -1;
	}

	h->client.serverIP = ((struct sockaddr_in*)(serverInfo->ai_addr))->sin_addr.s_addr;
	freeaddrinfo(serverInfo);
	
	/* create the client socket and get a socket descriptor */
	h->client.sd = socket(AF_INET, (SOCK_STREAM), 0);
	if(h->client.sd == -1) {
		h->slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"unable to start client: error in socket");
		return -1;
	}

	/* our client socket address information for connecting to the server */
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = h->client.serverIP;
	serverAddress.sin_port = htons(HELLO_PORT);

	/* connect to server. since we are non-blocking, we expect this to return EINPROGRESS */
	struct epoll_event ev = {};
	ev.events = EPOLLIN;
	ev.data.fd = h->client.sd;
	epoll_ctl(h->ed, EPOLL_CTL_ADD, h->client.sd, &ev);
	res = connect(h->client.sd,(struct sockaddr *)  &serverAddress, sizeof(serverAddress));
	if (res == -1 && errno != EINPROGRESS) {
		h->slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"unable to start client: error in connect");
		return -1;
	}
	//return 0;
	/* to keep things simple, there is explicitly no resilience here.
	 * we allow only one chance to send the message and one to receive the response.
	 */

	ssize_t numBytes = 0;
	char message[10];

	/* prepare the message */
	memset(message, 0, (size_t)10);
	snprintf(message, 10, "%s", "Hello?");
	
	/* send the message */
	numBytes = pth_send(h->client.sd, message, (size_t)6, 0);

	/* log result */
	if(numBytes == 6) {
		h->slogf(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__,
			 "successfully sent '%s' message", message);
	} else {
		h->slogf(SHADOW_LOG_LEVEL_WARNING, __FUNCTION__,
			 "unable to send message");
	}

	/* prepare to accept the message */
	memset(message, 0, (size_t)10);

	numBytes = pth_recv(h->client.sd, message, (size_t)6, 0);

	/* log result */
	if(numBytes > 0) {
		h->slogf(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__,
			 "successfully received '%s' message", message);
	} else {
		h->slogf(SHADOW_LOG_LEVEL_WARNING, __FUNCTION__,
			 "unable to receive message");
	}

	close(h->client.sd);
	h->client.sd = 0;
	h->isDone = 1;

	/* success! */
	return 0;
}

static int _hello_startServer(Hello* h) {
	/* create the socket and get a socket descriptor */
	h->server.sd = socket(AF_INET, (SOCK_STREAM), 0);

	if (h->server.sd == -1) {
		h->slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"unable to start server: error in socket");
		return -1;
	}

	/* setup the socket address info, client has outgoing connection to server */
	struct sockaddr_in bindAddress;
	memset(&bindAddress, 0, sizeof(bindAddress));
	bindAddress.sin_family = AF_INET;
	bindAddress.sin_addr.s_addr = INADDR_ANY;
	bindAddress.sin_port = htons(HELLO_PORT);

	/* bind the socket to the server port */
	int res = bind(h->server.sd, (struct sockaddr *) &bindAddress, sizeof(bindAddress));
	if (res == -1) {
		h->slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"unable to start server: error in bind");
		return -1;
	}

	/* set as server socket that will listen for clients */
	res = listen(h->server.sd, 100);
	if (res == -1) {
		h->slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"unable to start server: error in listen");
		return -1;
	}
	
	ssize_t numBytes = 0;
	char message[10];

	/* accept new connection from a remote client */
	struct epoll_event ev = {};
	ev.events = EPOLLIN;
	ev.data.fd = h->server.sd;
	epoll_ctl(h->ed, EPOLL_CTL_ADD, h->server.sd, &ev);
	int newClientSD = pth_accept(h->server.sd, NULL, NULL);
	//return 0;
	/* prepare to accept the message */
	memset(message, 0, (size_t)10);
	numBytes = pth_recv(newClientSD, message, (size_t)6, 0);

	/* log result */
	if(numBytes > 0) {
	  h->slogf(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__,
		   "successfully received '%s' message", message);
	} else if(numBytes == 0){
	  /* client got response and closed */
	  close(newClientSD);
	  printf("client closed socket\n");
	} else {
	  h->slogf(SHADOW_LOG_LEVEL_WARNING, __FUNCTION__,
		   "unable to receive message");
	}

	/* prepare the response message */
	memset(message, 0, (size_t)10);
	snprintf(message, 10, "%s", "World!");

	/* send the message */
	numBytes = pth_send(newClientSD, message, (size_t)6, 0);

	/* log result */
	if(numBytes == 6) {
	  h->slogf(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__,
		   "successfully sent '%s' message", message);
	} else {
	  h->slogf(SHADOW_LOG_LEVEL_WARNING, __FUNCTION__,
		   "unable to send message");
	}

	close(newClientSD);
	h->isDone = 1;

	/* success! */
	return 0;
}

Hello* hello_new(int argc, char* argv[], ShadowLogFunc slogf) {
	assert(slogf);

	if(argc < 1 || argc > 2) {
		slogf(SHADOW_LOG_LEVEL_WARNING, __FUNCTION__, USAGE);
		return NULL;
	}

	/* use epoll to asynchronously watch events for all of our sockets */
	int mainEpollDescriptor = epoll_create(1);
	if(mainEpollDescriptor == -1) {
		slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"Error in main epoll_create");
		close(mainEpollDescriptor);
		return NULL;
	}

	/* get memory for the new state */
	Hello* h = (Hello *) calloc(1, sizeof(Hello));
	assert(h);

	/* Set the global static epoll descriptor */
	global_ed = &h->ed;

	h->ed = mainEpollDescriptor;
	h->slogf = slogf;
	h->isDone = 0;


	// Both Client and Server will run the threading exercise
	slogf(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__, "Starting threads");
	pth_init();
	pth_yield(NULL);


	/* extract the server hostname from argv if in client mode */
	int isFail = 0;
	pth_t worker;
	if(argc == 2) {
	  /* client mode */
	  h->client.serverHostName = strndup(argv[1], (size_t)50);
	  pth_spawn(PTH_ATTR_DEFAULT, (void * (*) (void *)) _hello_startClient, h);
	} else {
	  /* server mode */
	  pth_spawn(PTH_ATTR_DEFAULT, (void * (*) (void *)) _hello_startServer, h);
	}

	// Jog the threads once
	hello_ready(h);

	if(isFail) {
		hello_free(h);
		return NULL;
	} else {
		return h;
	}
}

void hello_free(Hello* h) {
	assert(h);

	if(h->client.sd)
		close(h->client.sd);
	if(h->client.serverHostName)
		free(h->client.serverHostName);
	if(h->ed)
		close(h->ed);

	free(h);
}

void hello_ready(Hello* h) {
	assert(h);
	static int epd = -1;
	struct epoll_event ev = {};
	ev.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
	/*
	if (epd > -1) {
		ev.data.fd = epd;
		epoll_ctl(h->ed, EPOLL_CTL_DEL, epd, &ev);
		epd = -1;
		}*/
	pth_yield(NULL);
	return;
	fprintf(stderr, "Master activate\n");
	while (pth_ctrl(PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_NEW)) {
		pth_ctrl(PTH_CTRL_DUMPSTATE, stderr);
		pth_attr_set(pth_attr_of(pth_self()), PTH_ATTR_PRIO, PTH_PRIO_MIN);
		pth_yield(NULL);
	}
	//epd = pth_waiting_epoll();
	//ev.data.fd = epd;
	//epoll_ctl(h->ed, EPOLL_CTL_ADD, epd, &ev);
	fprintf(stderr, "Master exiting\n");
}

int hello_getEpollDescriptor(Hello* h) {
	assert(h);
	return h->ed;
}

int hello_isDone(Hello* h) {
	assert(h);
	return h->isDone;
}
