/*
 * See LICENSE for licensing information
 */

#include "leveldb-sample.h"
#include <pth.h>

/* functions that interface into shadow */
ShadowFunctionTable shadowlib;

/* our opaque instance of the hello node */
Hello* helloNodeInstance = NULL;


/* shadow is freeing an existing instance of this plug-in that we previously
 * created in leveldbplugin_new()
 */
static void leveldbplugin_free() {
	/* shadow wants to free a node. pass this to the lower level
	 * plug-in function that implements this for both plug-in and non-plug-in modes.
	 */
	hello_free(helloNodeInstance);
}

/* Subtract the `struct timeval' values X and Y,
        storing the result in RESULT.
        Return 1 if the difference is negative, otherwise 0. */
static int _timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	
	/* Compute the time remaining to wait.
	   tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/* shadow is notifying us that some descriptors are ready to read/write */
static void leveldbplugin_ready() {
	static int epd = -1;
	int ed = hello_getEpollDescriptor(helloNodeInstance);

	struct epoll_event ev = {};
	shadowlib.log(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__, "Master activated");
	epoll_wait(ed, &ev, 1, 0); // try to consume an event

	if (epd > -1) {
		epoll_ctl(ed, EPOLL_CTL_DEL, epd, NULL);
		epd = -1;
	}

	ev.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
	pth_attr_set(pth_attr_of(pth_self()), PTH_ATTR_PRIO, PTH_PRIO_MIN);
	pth_yield(NULL); // go visit the scheduler at least once
	while (pth_ctrl(PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_NEW)) {
		//pth_ctrl(PTH_CTRL_DUMPSTATE, stderr);
		pth_attr_set(pth_attr_of(pth_self()), PTH_ATTR_PRIO, PTH_PRIO_MIN);
		pth_yield(NULL);
	}
	epd = pth_waiting_epoll();
	if (epd > -1) {
		ev.data.fd = epd;
		epoll_ctl(ed, EPOLL_CTL_ADD, epd, &ev);
	}
	shadowlib.log(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__, "Master exiting");

	/* Figure out when the next timer would be */
	struct timeval timeout = pth_waiting_timeout();
	if (!(timeout.tv_sec == 0 && timeout.tv_usec == 0)) {
		struct timeval now, delay;
		gettimeofday(&now, NULL);
		uint ms;
		if (_timeval_subtract(&delay, &timeout, &now)) ms = 0;
		else ms = 1 + delay.tv_sec*1000 + (delay.tv_usec+1)/1000;
		shadowlib.log(SHADOW_LOG_LEVEL_DEBUG, __FUNCTION__, "Registering a callback for %d ms", ms);
		shadowlib.createCallback((ShadowPluginCallbackFunc) leveldbplugin_ready, NULL, ms);
	}
}

/* shadow is creating a new instance of this plug-in as a node in
 * the simulation. argc and argv are as configured via the XML.
 */
static void leveldbplugin_new(int argc, char* argv[]) {
	/* shadow wants to create a new node. pass this to the lower level
	 * plug-in function that implements this for both plug-in and non-plug-in modes.
	 * also pass along the interface shadow gave us earlier.
	 *
	 * the value of helloNodeInstance will be different for every node, because
	 * we did not set it in __shadow_plugin_init__(). this is desirable, because
	 * each node needs its own application state.
	 */
	helloNodeInstance = hello_new(argc, argv, shadowlib.log);

	// Jog the threads once
	leveldbplugin_ready();
}


/* plug-in initialization. this only happens once per plug-in,
 * no matter how many nodes (instances of the plug-in) are configured.
 *
 * whatever state is configured in this function will become the default
 * starting state for each node.
 *
 * the "__shadow_plugin_init__" function MUST exist in every plug-in.
 */
void __shadow_plugin_init__(ShadowFunctionTable* shadowlibFuncs) {
	assert(shadowlibFuncs);

	/* locally store the functions we use to call back into shadow */
	shadowlib = *shadowlibFuncs;

	/*
	 * tell shadow how to call us back when creating/freeing nodes, and
	 * where to call to notify us when there is descriptor I/O
	 */
	int success = shadowlib.registerPlugin(&leveldbplugin_new, &leveldbplugin_free, &leveldbplugin_ready);

	/* we log through Shadow by using the log function it supplied to us */
	if(success) {
		shadowlib.log(SHADOW_LOG_LEVEL_MESSAGE, __FUNCTION__,
				"successfully registered leveldb-sample plug-in state");
	} else {
		shadowlib.log(SHADOW_LOG_LEVEL_CRITICAL, __FUNCTION__,
				"error registering leveldb-sample plug-in state");
	}
}
