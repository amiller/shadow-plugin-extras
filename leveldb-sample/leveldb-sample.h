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

#include <glib.h>
#include <gmodule.h>

#include <shd-library.h>

typedef enum _ExecutionContext ExecutionContext;
enum _ExecutionContext {
    EXECTX_NONE, EXECTX_BITCOIN, EXECTX_PTH, EXECTX_SHADOW,
};

void leveldbpreload_init(GModule* handle);
void leveldbpreload_setContext(ExecutionContext ctx);

typedef struct _Hello Hello;

Hello* hello_new(int argc, char* argv[], ShadowLogFunc slogf);
void hello_free(Hello* h);
void hello_ready(Hello* h);
int hello_getEpollDescriptor(Hello* h);
int hello_isDone(Hello* h);

#endif /* HELLO_H_ */
