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
#include <fcntl.h>
#include "pth-toy.h"

static ShadowLogFunc slogf;

#define LOG_MESSAGE(...) slogf(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__, __VA_ARGS__)

extern void toy_pthread_doWork();

#include <stdio.h>

int plugin_main(int argc, char* argv[], ShadowLogFunc slogf_) {
        slogf = slogf_;
	pthread_t thr;
	LOG_MESSAGE("About to do multithread work");
	toy_pthread_doWork();
	LOG_MESSAGE("Done");

	LOG_MESSAGE("Creating file");
	FILE *f = fopen("/tmp/pth-toy-lock","w");
	assert(f);
	fclose(f);
	int hnd = open("/tmp/pth-toy-lock",O_RDWR);
	assert(hnd != -1);
	struct flock lock;
	lock.l_type    = F_WRLCK;
	lock.l_whence  = SEEK_SET;
	lock.l_start   = 0;
	lock.l_len     = 0;
	int ret = fcntl(hnd, F_SETLK, &lock);
	if (!ret) {
	}
	assert(ret == 0);

	return 0;
}
