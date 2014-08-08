/*
 * See LICENSE for licensing information
 */

#ifndef PTH_TOY_H_
#define PTH_TOY_H_

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

#include "plugin-preload.h"

int plugin_main(int argc, char* argv[], ShadowLogFunc slogf);

#endif /* PTH_TOY_H_ */
