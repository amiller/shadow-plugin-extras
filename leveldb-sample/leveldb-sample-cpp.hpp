/*
 * See LICENSE for licensing information
 */

#ifndef HELLO_H_
#define HELLO_H_

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

typedef struct _Hello Hello;

extern "C" Hello* hello_new(int argc, char* argv[], ShadowLogFunc slogf);
extern "C" void hello_free(Hello* h);
extern "C" void hello_ready(Hello* h);
extern "C" int hello_getEpollDescriptor(Hello* h);
extern "C" int hello_isDone(Hello* h);

#endif /* HELLO_H_ */
