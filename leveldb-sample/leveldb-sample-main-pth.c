/*
 * See LICENSE for licensing information
 */

#include "leveldb-sample.h"
#include <pth.h>

/* our hello code only relies on the log part of shadowlib,
 * so we need to supply that implementation here since this is
 * running outside of shadow. */
static void _mylog(ShadowLogLevel level, const char* functionName, const char* format, ...) {
	va_list variableArguments;
	va_start(variableArguments, format);
	vprintf(format, variableArguments);
	va_end(variableArguments);
	printf("%s", "\n");
}
#define mylog(...) _mylog(SHADOW_LOG_LEVEL_INFO, __FUNCTION__, __VA_ARGS__)

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

/* this main replaces the hello-plugin.c file to run outside of shadow */
int main(int argc, char *argv[]) {
	mylog("Starting LevelDB-Sample program");

	/* create the new state according to user inputs */
	Hello* helloState = hello_new(argc, argv, &_mylog);
	if(!helloState) {
		mylog("Error initializing new LevelDB instance");
		return -1;
	}

	/* now we need to watch all of the hello descriptors in our main loop
	 * so we know when we can wait on any of them without blocking. */
	int mainepolld = epoll_create(1);
	if(mainepolld == -1) {
		mylog("Error in main epoll_create");
		close(mainepolld);
		return -1;
	}

	/* hello has one main epoll descriptor that watches all of its sockets,
	 * so we now register that descriptor so we can watch for its events */
	struct epoll_event mainevent;
	mainevent.events = EPOLLIN|EPOLLOUT;
	mainevent.data.fd = hello_getEpollDescriptor(helloState);
	if(!mainevent.data.fd) {
		mylog("Error retrieving hello epoll descriptor");
		close(mainepolld);
		return -1;
	}
	epoll_ctl(mainepolld, EPOLL_CTL_ADD, mainevent.data.fd, &mainevent);

	/* main loop - wait for events from the hello descriptors */
	struct epoll_event events[100];
	int nReadyFDs;
	mylog("entering main loop to watch descriptors");

	static int epd = -1;

	while(!hello_isDone(helloState)) {
		int ed = mainepolld;
		
		struct epoll_event ev = {};
		mylog("Master activated");
		
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
		mylog("Master exiting");
		
		/* Figure out when the next timer would be */
		struct timeval timeout = pth_waiting_timeout();
		if (!(timeout.tv_sec == 0 && timeout.tv_usec == 0)) {
			struct timeval now, delay;
			gettimeofday(&now, NULL);
			uint ms;
			if (_timeval_subtract(&delay, &timeout, &now)) ms = 0;
			else ms = 1 + delay.tv_sec*1000 + (delay.tv_usec+1)/1000;
			mylog("Sleeping for %d ms", ms);
			usleep(1000*ms); // REALLY sleep
			continue;
		}

		if (hello_isDone(helloState)) break;
		
		/* wait for some events */
		mylog("waiting for events");
		nReadyFDs = epoll_wait(mainepolld, events, 100, -1);
		if(nReadyFDs == -1) {
			mylog("Error in client epoll_wait");
			return -1;
		}

		/* activate if something is ready */
		mylog("processing event");
	}

	mylog("finished main loop, cleaning up");

	/* de-register the hello epoll descriptor */
	mainevent.data.fd = hello_getEpollDescriptor(helloState);
	if(mainevent.data.fd) {
		epoll_ctl(mainepolld, EPOLL_CTL_DEL, mainevent.data.fd, &mainevent);
	}

	/* cleanup and close */
	close(mainepolld);
	hello_free(helloState);

	mylog("exiting cleanly");

	return 0;
}
