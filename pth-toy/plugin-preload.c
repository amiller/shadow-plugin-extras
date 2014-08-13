/*
 * The Shadow Simulator
 * Copyright (c) 2010-2011, Rob Jansen
 * See LICENSE for licensing information
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <execinfo.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <openssl/crypto.h>
#include <errno.h>
#include <assert.h>
#include <glib.h>
#include <gmodule.h>

#include "plugin-preload.h"
//#include "bitcoind.h"


#include "pth.h"

#define SHADOW_TIMER_OFFSET 1404101800

typedef int (*close_fp)(int);
typedef ssize_t (*read_fp)(int, void*, size_t);
typedef ssize_t (*write_fp)(int, const void*, size_t);
typedef int (*nanosleep_fp)(const struct timespec *, struct timespec *);
typedef int (*usleep_fp)(unsigned int);
typedef int (*sleep_fp)(unsigned int);
typedef int (*system_fp)(const char *);
typedef int (*sigprocmask_fp)(int, const sigset_t *, sigset_t *);
typedef int (*sigwait_fp)(const sigset_t *, int *);
typedef pid_t (*waitpid_fp)(pid_t, int *, int);
typedef int (*connect_fp)(int, const struct sockaddr *, socklen_t);
typedef int (*accept_fp)(int, struct sockaddr *, socklen_t *);
//typedef int (*select_fp)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
//typedef int (*pselect_fp)(int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *);
typedef int (*poll_fp)(struct pollfd *, nfds_t, int);
typedef ssize_t (*readv_fp)(int, const struct iovec *, int);
typedef ssize_t (*writev_fp)(int, const struct iovec *, int);
typedef ssize_t (*pread_fp)(int, void *, size_t, off_t);
typedef ssize_t (*pwrite_fp)(int, const void *, size_t, off_t);
typedef ssize_t (*recv_fp)(int, void *, size_t, int);
typedef ssize_t (*send_fp)(int, const void *, size_t, int);
typedef ssize_t (*recvfrom_fp)(int, void *, size_t, int, struct sockaddr *, socklen_t *);
typedef ssize_t (*sendto_fp)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
typedef int (*__cxa_atexit_fp)(void (*f)(void*), void *arg, void *dso_handle);

/* socket/io family */
typedef int (*socket_fp)(int, int, int);
typedef int (*socketpair_fp)(int, int, int, int[]);
typedef int (*bind_fp)(int, __CONST_SOCKADDR_ARG, socklen_t);
typedef int (*getsockname_fp)(int, __SOCKADDR_ARG, socklen_t*);
//typedef int (*connect_fp)(int, __const_sockaddr_arg, socklen_t);
typedef int (*getpeername_fp)(int, __SOCKADDR_ARG, socklen_t*);
//typedef size_t (*send_fp)(int, const void*, size_t, int);
//typedef size_t (*sendto_fp)(int, const void*, size_t, int, __const_sockaddr_arg, socklen_t);
typedef size_t (*sendmsg_fp)(int, const struct msghdr*, int);
//typedef size_t (*recv_fp)(int, void*, size_t, int);
//typedef size_t (*recvfrom_fp)(int, void*, size_t, int, __sockaddr_arg, socklen_t*);
typedef size_t (*recvmsg_fp)(int, struct msghdr*, int);
typedef int (*getsockopt_fp)(int, int, int, void*, socklen_t*);
typedef int (*setsockopt_fp)(int, int, int, const void*, socklen_t);
typedef int (*listen_fp)(int, int);
//typedef int (*accept_fp)(int, __sockaddr_arg, socklen_t*);
typedef int (*accept4_fp)(int, __SOCKADDR_ARG, socklen_t*, int);
typedef int (*shutdown_fp)(int, int);
typedef int (*pipe_fp)(int [2]);
typedef int (*pipe2_fp)(int [2], int);
typedef int (*eventfd_fp)(unsigned int, int);
//typedef size_t (*read_fp)(int, void*, int);
//typedef size_t (*write_fp)(int, const void*, int);
//typedef int (*close_fp)(int);
typedef int (*fcntl_fp)(int, int, ...);
typedef int (*ioctl_fp)(int, int, ...);

/* memory allocation family */
typedef void* (*malloc_fp)(size_t);
typedef void* (*calloc_fp)(size_t, size_t);
typedef void* (*realloc_fp)(void*, size_t);
typedef int (*posix_memalign_fp)(void**, size_t, size_t);
typedef void* (*memalign_fp)(size_t, size_t);
typedef void* (*aligned_alloc_fp)(size_t, size_t);
typedef void* (*valloc_fp)(size_t);
typedef void* (*pvalloc_fp)(size_t);
typedef void (*free_fp)(void*);
typedef void *(*mmap_fp)(void*, size_t, int, int, int, off_t);

/* time family */
typedef int (*gettimeofday_fp)(struct timeval *tv, struct timezone *tz);
typedef time_t (*time_fp)(time_t*);
typedef int (*clock_gettime_fp)(clockid_t, struct timespec *);

/* random family */
typedef int (*rand_fp)();
typedef int (*rand_r_fp)(unsigned int *seedp);
typedef void (*srand_fp)(unsigned int seed);
typedef long int (*random_fp)();
typedef int (*random_r_fp)(struct random_data *buf, int32_t *result);
typedef void (*srandom_fp)(unsigned int seed);
typedef int (*srandom_r_fp)(unsigned int seed, struct random_data *buf);

/* crypto family */
typedef void* (*CRYPTO_get_locking_callback_fp)();
typedef void* (*CRYPTO_get_id_callback_fp)();
typedef int (*SSL_library_init_fp)();
typedef void (*RAND_seed_fp)(const void *buf, int num);
typedef void (*RAND_add_fp)(const void *buf, int num, double entropy);
typedef int (*RAND_poll_fp)();
typedef int (*RAND_bytes_fp)(unsigned char *buf, int num);
typedef int (*RAND_pseudo_bytes_fp)(unsigned char *buf, int num);
typedef void (*RAND_cleanup_fp)();
typedef int (*RAND_status_fp)();
typedef const void *(*RAND_get_rand_method_fp)();
typedef const void *(*RAND_SSLeay_fp)();


/* file specific */
struct statfs;
struct stat;
typedef int (*fileno_fp)(FILE *stream);
typedef int (*open_fp)(const char *pathname, int flags, ...);
typedef int (*open64_fp)(const char *pathname, int flags, ...);
typedef int (*creat_fp)(const char *pathname, mode_t mode);
typedef FILE *(*fopen_fp)(const char *path, const char *mode);
typedef FILE *(*fdopen_fp)(int fd, const char *mode);
typedef int (*dup_fp)(int oldfd);
typedef int (*dup2_fp)(int oldfd, int newfd);
typedef int (*dup3_fp)(int oldfd, int newfd, int flags);
typedef int (*fclose_fp)(FILE *fp);
typedef int (*__fxstat_fp) (int ver, int fd, struct stat *buf);
typedef int (*fstatfs_fp)(int fd, struct statfs *buf);
typedef off_t (*lseek_fp)(int fd, off_t offset, int whence);
typedef int (*flock_fp)(int fd, int operation);
typedef int (*ftruncate_fp)(int fd, int length);
typedef int (*posix_fallocate_fp)(int fd, off_t offset, off_t len);
typedef int (*fsync_fp)(int fd);


/* name/address family */
struct addrinfo;
struct hostent;
typedef int (*gethostname_fp)(char*, size_t);
typedef int (*getaddrinfo_fp)(const char*, const char*, const struct addrinfo*, struct addrinfo**);
struct gaicb;
typedef int (*getaddrinfo_a_fp)(int mode, struct gaicb *list[], int nitems, struct sigevent *sevp);
typedef int (*freeaddrinfo_fp)(struct addrinfo*);
typedef int (*getnameinfo_fp)(const struct sockaddr *, socklen_t, char *, size_t, char *, size_t, int);
typedef struct hostent* (*gethostbyname_fp)(const char*);
typedef int (*gethostbyname_r_fp)(const char*, struct hostent*, char*, size_t, struct hostent**, int*);
typedef struct hostent* (*gethostbyname2_fp)(const char*, int);
typedef int (*gethostbyname2_r_fp)(const char*, int, struct hostent *, char *, size_t, struct hostent**, int*);
typedef struct hostent* (*gethostbyaddr_fp)(const void*, socklen_t, int);
typedef int (*gethostbyaddr_r_fp)(const void*, socklen_t, int, struct hostent*, char*, size_t, struct hostent **, int*);


/* pth */
typedef ssize_t (*pth_read_fp)(int, void*, size_t);
typedef ssize_t (*pth_write_fp)(int, const void*, size_t);
typedef int (*pth_nanosleep_fp)(const struct timespec *, struct timespec *);
typedef int (*pth_usleep_fp)(unsigned int);
typedef long (*pth_ctrl_fp)(unsigned long, ...);
typedef unsigned int  (*pth_sleep_fp)(unsigned int);
typedef void (*pth_init_fp)(void);
typedef pth_t (*pth_spawn_fp)(pth_attr_t attr, void *(*func)(void *), void *arg);
typedef pth_t (*pth_join_fp)(pth_t thread, void **retval);
typedef int (*pth_mutex_init_fp)(pth_mutex_t *);
typedef int (*pth_mutex_acquire_fp)(pth_mutex_t *, int, pth_event_t);
typedef int (*pth_mutex_release_fp)(pth_mutex_t *);
typedef int (*pth_cond_init_fp)(pth_cond_t *);
typedef int (*pth_cond_await_fp)(pth_cond_t *, pth_mutex_t *, pth_event_t);
typedef int (*pth_cond_notify_fp)(pth_cond_t *, int);
typedef int (*pth_key_create_fp)(pth_key_t *, void (*)(void *));
typedef int (*pth_key_delete_fp)(pth_key_t);
typedef int (*pth_key_setdata_fp)(pth_key_t, const void *);
typedef void *(*pth_key_getdata_fp)(pth_key_t);
typedef pth_attr_t (*pth_attr_of_fp)(pth_t t);
typedef int (*pth_attr_destroy_fp)(pth_attr_t a);
typedef int (*pth_attr_set_fp)(pth_attr_t a, int op, ...);
typedef pth_time_t (*pth_time_fp)(long, long);
typedef pth_event_t (*pth_event_fp)(unsigned long, ...);
typedef pth_status_t (*pth_event_status_fp)(pth_event_t);
typedef int (*pth_util_select_fp)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
typedef int (*pth_select_fp)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
typedef ssize_t (*pth_readv_fp)(int, const struct iovec *, int);
typedef ssize_t (*pth_writev_fp)(int, const struct iovec *, int);
typedef ssize_t (*pth_pread_fp)(int, void *, size_t, off_t);
typedef ssize_t (*pth_pwrite_fp)(int, const void *, size_t, off_t);
typedef ssize_t (*pth_recv_fp)(int, void *, size_t, int);
typedef ssize_t (*pth_send_fp)(int, const void *, size_t, int);
typedef ssize_t (*pth_recvfrom_fp)(int, void *, size_t, int, struct sockaddr *, socklen_t *);
typedef ssize_t (*pth_sendto_fp)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
typedef int (*pth_connect_fp)(int, const struct sockaddr *, socklen_t);
typedef int (*pth_accept_fp)(int, struct sockaddr *, socklen_t *);



/* epoll */
struct epoll_event;
typedef int (*epoll_create_fp)(int);
typedef int (*epoll_create1_fp)(int flags);
typedef int (*epoll_ctl_fp)(int epfd, int op, int fd, struct epoll_event *event);
typedef int (*epoll_wait_fp)(int epfd, struct epoll_event *events, int maxevents, int timeout);
typedef int (*epoll_pwait_fp)(int, struct epoll_event*, int, int, const sigset_t*);


/* pthread */

typedef int (*pthread_create_fp)(pthread_t*, const pthread_attr_t*,
        void *(*start_routine) (void *), void *arg);
typedef int (*pthread_detach_fp)(pthread_t);
typedef int (*pthread_join_fp)(pthread_t, void **);
typedef int (*pthread_once_fp)(pthread_once_t*, void (*init_routine)(void));
typedef int (*pthread_key_create_fp)(pthread_key_t*, void (*destructor)(void*));
typedef int (*pthread_setspecific_fp)(pthread_key_t, const void*);
typedef void* (*pthread_getspecific_fp)(pthread_key_t);
typedef int (*pthread_attr_setdetachstate_fp)(pthread_attr_t*, int);
typedef int (*pthread_attr_getdetachstate_fp)(const pthread_attr_t*, int*);
typedef int (*pthread_cond_init_fp)(pthread_cond_t*, const pthread_condattr_t*);
typedef int (*__pthread_cond_init_2_0_fp)(pthread_cond_t*, const pthread_condattr_t*);
typedef int (*pthread_cond_destroy_fp)(pthread_cond_t*);
typedef int (*pthread_cond_signal_fp)(pthread_cond_t*);
typedef int (*pthread_cond_broadcast_fp)(pthread_cond_t*);
typedef int (*pthread_cond_wait_fp)(pthread_cond_t*, pthread_mutex_t*);
typedef int (*pthread_cond_timedwait_fp)(pthread_cond_t*, pthread_mutex_t*, const struct timespec*);
typedef int (*pthread_mutex_init_fp)(pthread_mutex_t*,
              const pthread_mutexattr_t*);
typedef int (*pthread_mutex_destroy_fp)(pthread_mutex_t*);
typedef int (*pthread_mutex_lock_fp)(pthread_mutex_t*);
typedef int (*pthread_mutex_trylock_fp)(pthread_mutex_t*);
typedef int (*pthread_mutex_unlock_fp)(pthread_mutex_t*);

typedef int (*CLogPrintStr_fp)(const char*);
typedef int (*crypto_global_init_fp)(int, const char*, const char*);
typedef int (*crypto_global_cleanup_fp)(void);

/* the key used to store each threads version of their searched function library.
 * the use this key to retrieve this library when intercepting functions from tor.
 */
//static GPrivate bitcoindWorkerKey = G_PRIVATE_INIT((GDestroyNotify)g_free);
static __thread void * pluginWorkerKey = NULL;
#define g_private_get(ptr) (*(ptr))


/* track if we are in a recursive loop to avoid infinite recursion.
 * threads MUST access this via &isRecursive to ensure each has its own copy
 * http://gcc.gnu.org/onlinedocs/gcc-4.3.6/gcc/Thread_002dLocal.html */
static __thread unsigned long isRecursive = 0;

#define __FUNC_TABLE_ENTRY(func) func##_fp func

typedef struct _PthTable PthTable;
struct _PthTable {
	/* pth */
	pth_read_fp pth_read;
	pth_write_fp pth_write;
	pth_usleep_fp pth_usleep;
	pth_nanosleep_fp pth_nanosleep;
	pth_sleep_fp pth_sleep;
	pth_ctrl_fp pth_ctrl;

	pth_init_fp pth_init;
	pth_join_fp pth_join;
	pth_spawn_fp pth_spawn;
	pth_mutex_init_fp pth_mutex_init;
	pth_mutex_acquire_fp pth_mutex_acquire;
	pth_mutex_release_fp pth_mutex_release;
	pth_cond_init_fp pth_cond_init;
	pth_cond_await_fp pth_cond_await;
	pth_cond_notify_fp pth_cond_notify;
	pth_key_create_fp pth_key_create;
	pth_key_delete_fp pth_key_delete;
	pth_key_setdata_fp pth_key_setdata;
	pth_key_getdata_fp pth_key_getdata;

	pth_attr_of_fp pth_attr_of;
	pth_attr_set_fp pth_attr_set;
	pth_attr_destroy_fp pth_attr_destroy;
	pth_time_fp pth_time;
	pth_event_fp pth_event;
	pth_event_status_fp pth_event_status;
	__FUNC_TABLE_ENTRY(pth_util_select);
	__FUNC_TABLE_ENTRY(pth_select);

	__FUNC_TABLE_ENTRY(pth_readv);
	__FUNC_TABLE_ENTRY(pth_writev);
	__FUNC_TABLE_ENTRY(pth_pread);
	__FUNC_TABLE_ENTRY(pth_pwrite);
	__FUNC_TABLE_ENTRY(pth_recv);
	__FUNC_TABLE_ENTRY(pth_send);
	__FUNC_TABLE_ENTRY(pth_recvfrom);
	__FUNC_TABLE_ENTRY(pth_sendto);
	__FUNC_TABLE_ENTRY(pth_connect);
	__FUNC_TABLE_ENTRY(pth_accept);
};

typedef struct _FunctionTable FunctionTable;
struct _FunctionTable {
	close_fp close;
	read_fp read;
	write_fp write;
	usleep_fp usleep;
	nanosleep_fp nanosleep;
	sleep_fp sleep;

	__FUNC_TABLE_ENTRY(CLogPrintStr);

	/* Crypto global */
	__FUNC_TABLE_ENTRY(crypto_global_init);
	__FUNC_TABLE_ENTRY(crypto_global_cleanup);

	/* socket/io family */
	__FUNC_TABLE_ENTRY(socket);
	__FUNC_TABLE_ENTRY(socketpair);
	__FUNC_TABLE_ENTRY(bind);
	__FUNC_TABLE_ENTRY(connect);
	__FUNC_TABLE_ENTRY(getsockname);
	__FUNC_TABLE_ENTRY(getpeername);
	__FUNC_TABLE_ENTRY(send);
	__FUNC_TABLE_ENTRY(sendto);
	__FUNC_TABLE_ENTRY(sendmsg);
	__FUNC_TABLE_ENTRY(recvmsg);
	__FUNC_TABLE_ENTRY(recv);
	__FUNC_TABLE_ENTRY(recvfrom);
	__FUNC_TABLE_ENTRY(getsockopt);
	__FUNC_TABLE_ENTRY(setsockopt);
	__FUNC_TABLE_ENTRY(listen);
	__FUNC_TABLE_ENTRY(accept);
	__FUNC_TABLE_ENTRY(accept4);
	__FUNC_TABLE_ENTRY(shutdown);
	__FUNC_TABLE_ENTRY(fcntl);
	__FUNC_TABLE_ENTRY(ioctl);
	__FUNC_TABLE_ENTRY(pipe);
	__FUNC_TABLE_ENTRY(pipe2);
	__FUNC_TABLE_ENTRY(eventfd);
	__FUNC_TABLE_ENTRY(readv);
	__FUNC_TABLE_ENTRY(writev);
	__FUNC_TABLE_ENTRY(pread);
	__FUNC_TABLE_ENTRY(pwrite);


	/* file specific */
	__FUNC_TABLE_ENTRY(fileno);
	__FUNC_TABLE_ENTRY(open);
	__FUNC_TABLE_ENTRY(open64);
	__FUNC_TABLE_ENTRY(creat);
	__FUNC_TABLE_ENTRY(fopen);
	__FUNC_TABLE_ENTRY(fdopen);
	__FUNC_TABLE_ENTRY(dup);
	__FUNC_TABLE_ENTRY(dup2);
	__FUNC_TABLE_ENTRY(dup3);
	__FUNC_TABLE_ENTRY(fclose);
	__FUNC_TABLE_ENTRY(__fxstat);
	__FUNC_TABLE_ENTRY(fstatfs);
	__FUNC_TABLE_ENTRY(lseek);
	__FUNC_TABLE_ENTRY(flock);
	__FUNC_TABLE_ENTRY(fsync);
	__FUNC_TABLE_ENTRY(ftruncate);
	__FUNC_TABLE_ENTRY(posix_fallocate);

	/* name/address family */
	__FUNC_TABLE_ENTRY(gethostname);
	__FUNC_TABLE_ENTRY(getaddrinfo);
	__FUNC_TABLE_ENTRY(freeaddrinfo);
	__FUNC_TABLE_ENTRY(getnameinfo);
	__FUNC_TABLE_ENTRY(gethostbyname);
	__FUNC_TABLE_ENTRY(gethostbyname_r);
	__FUNC_TABLE_ENTRY(gethostbyname2);
	__FUNC_TABLE_ENTRY(gethostbyname2_r);
	__FUNC_TABLE_ENTRY(gethostbyaddr);
	__FUNC_TABLE_ENTRY(gethostbyaddr_r);
	

	/* memory allocation family */
	__FUNC_TABLE_ENTRY(__cxa_atexit);
	__cxa_atexit_fp bitcoindplugin_cxa_atexit;
	malloc_fp malloc;
	calloc_fp calloc;
	realloc_fp realloc;
	posix_memalign_fp posix_memalign;
	memalign_fp memalign;
	aligned_alloc_fp aligned_alloc;
	valloc_fp valloc;
	pvalloc_fp pvalloc;
	free_fp free;
	mmap_fp mmap;

	/* time family */
	clock_gettime_fp clock_gettime;
	time_fp time;
	gettimeofday_fp gettimeofday;

	/* event */
	epoll_create_fp epoll_create;
	epoll_create1_fp epoll_create1;
	epoll_ctl_fp epoll_ctl;
	epoll_wait_fp epoll_wait;
	epoll_pwait_fp epoll_pwait;

	/* random family */
	rand_fp rand;
	rand_r_fp rand_r;
	srand_fp srand;
	random_fp random;
	random_r_fp random_r;
	srandom_fp srandom;
	srandom_r_fp srandom_r;

	/* crypto family */
	__FUNC_TABLE_ENTRY(CRYPTO_get_locking_callback);
	__FUNC_TABLE_ENTRY(CRYPTO_get_id_callback);
	__FUNC_TABLE_ENTRY(SSL_library_init);
	__FUNC_TABLE_ENTRY(RAND_seed);
	__FUNC_TABLE_ENTRY(RAND_add);
	__FUNC_TABLE_ENTRY(RAND_poll);
	__FUNC_TABLE_ENTRY(RAND_bytes);
	__FUNC_TABLE_ENTRY(RAND_pseudo_bytes);
	__FUNC_TABLE_ENTRY(RAND_cleanup);
	__FUNC_TABLE_ENTRY(RAND_status);
	__FUNC_TABLE_ENTRY(RAND_get_rand_method);
	__FUNC_TABLE_ENTRY(RAND_SSLeay);

	/* pthread */
	pthread_create_fp pthread_create;
	pthread_detach_fp pthread_detach;
	pthread_join_fp pthread_join;
	pthread_once_fp pthread_once;
	pthread_key_create_fp pthread_key_create;
	pthread_setspecific_fp pthread_setspecific;
	pthread_getspecific_fp pthread_getspecific;
	pthread_attr_setdetachstate_fp pthread_attr_setdetachstate;
	pthread_attr_getdetachstate_fp pthread_attr_getdetachstate;
	pthread_cond_init_fp pthread_cond_init;
	pthread_cond_destroy_fp pthread_cond_destroy;
	pthread_cond_signal_fp pthread_cond_signal;
	pthread_cond_broadcast_fp pthread_cond_broadcast;
	pthread_cond_wait_fp pthread_cond_wait;
	pthread_cond_timedwait_fp pthread_cond_timedwait;
	pthread_mutex_init_fp pthread_mutex_init;
	pthread_mutex_destroy_fp pthread_mutex_destroy;
	pthread_mutex_lock_fp pthread_mutex_lock;
	pthread_mutex_trylock_fp pthread_mutex_trylock;
	pthread_mutex_unlock_fp pthread_mutex_unlock;
};

typedef struct _PluginPreloadWorker PluginPreloadWorker;

struct _PluginPreloadWorker {
	GModule* handle;
	ExecutionContext activeContext;
	PluginName activePlugin;
	long isPlugin;
	FunctionTable ftable;
        PthTable plugin_pthtable;
	unsigned long isRecursive;
};

static inline PthTable* _get_active_pthtable(PluginPreloadWorker *worker) {
	assert(worker);
	//assert(worker->activeContext == EXECTX_PLUGIN);
	return &worker->plugin_pthtable;
	assert(0);
	return 0;
}

/* here we search and save pointers to the functions we need to call when
 * we intercept tor's functions. this is initialized for each thread, and each
 * thread has pointers to their own functions (each has its own version of the
 * plug-in state). We dont register these function locations, because they are
 * not *node* dependent, only *thread* dependent.
 */

#define SETSYM_OR_FAIL_(handle, funcptr, funcstr) {	\
	dlerror(); \
	funcptr = dlsym(handle, funcstr); \
	char* errorMessage = dlerror(); \
	if(errorMessage != NULL) { \
		fprintf(stderr, "dlsym(%s): dlerror(): %s\n", funcstr, errorMessage); \
		exit(EXIT_FAILURE); \
	} else if(funcptr == NULL) { \
		fprintf(stderr, "dlsym(%s): returned NULL pointer\n", funcstr); \
		exit(EXIT_FAILURE); \
	} \
}

#define SETSYM_OR_FAIL(funcptr, funcstr) SETSYM_OR_FAIL_(RTLD_NEXT, funcptr, funcstr)
#define SETSYM_OR_FAIL_DEFAULT(funcptr, funcstr) SETSYM_OR_FAIL_(RTLD_DEFAULT, funcptr, funcstr)

#define SETSYM_OR_FAIL_V_(handle,funcptr, funcstr, version) {	\
	dlerror(); \
	funcptr = dlvsym(handle, funcstr, version);	\
	char* errorMessage = dlerror(); \
	if(errorMessage != NULL) { \
		fprintf(stderr, "dlvsym(%s,%s): dlerror(): %s\n", funcstr, version, errorMessage); \
		exit(EXIT_FAILURE); \
	} else if(funcptr == NULL) { \
		fprintf(stderr, "dlvsym(%s,%s): returned NULL pointer\n", funcstr, version); \
		exit(EXIT_FAILURE); \
	} \
}
#define SETSYM_OR_FAIL_V(funcptr, funcstr, version) SETSYM_OR_FAIL_V_(RTLD_NEXT, funcptr, funcstr, version)

#define _FTABLE_GUARD(rctype, func, ...)       \
    static func##_fp real = NULL;	       \
    if (!real) SETSYM_OR_FAIL(real, #func);    \
    assert(real);			       \
    if(__sync_fetch_and_add(&isRecursive, 1)) {\
	    rctype rc = real(__VA_ARGS__);\
	    __sync_fetch_and_sub(&isRecursive, 1);\
            return rc;\
    }\
    PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);\
    if (!worker || (worker->activeContext != EXECTX_PLUGIN && worker->activeContext != EXECTX_PTH)) { \
	    rctype rc = real(__VA_ARGS__);\
            __sync_fetch_and_sub(&isRecursive, 1);\
	    return rc;\
    }\
    if (worker->activeContext != EXECTX_PLUGIN) {\
	    rctype rc = worker->ftable.func(__VA_ARGS__);\
	    __sync_fetch_and_sub(&isRecursive, 1);\
	    return rc;\
    }\
    __sync_fetch_and_sub(&isRecursive, 1);\

#define _FTABLE_GUARD_VOID(func, ...)          \
    static func##_fp real = NULL;	       \
    if (!real) SETSYM_OR_FAIL(real, #func);    \
    assert(real);			       \
    if(__sync_fetch_and_add(&isRecursive, 1)) {\
	    real(__VA_ARGS__);\
	    __sync_fetch_and_sub(&isRecursive, 1);\
            return;\
    }\
    PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);\
    if (!worker) {\
	    real(__VA_ARGS__);\
            __sync_fetch_and_sub(&isRecursive, 1);\
	    return;\
    }\
    if (worker->activeContext != EXECTX_PLUGIN) {\
	    worker->ftable.func(__VA_ARGS__);\
	    __sync_fetch_and_sub(&isRecursive, 1);\
	    return;\
    }\
    __sync_fetch_and_sub(&isRecursive, 1);\

#define _FTABLE_GUARD_V(rctype, func, version, ...)	\
    if(__sync_fetch_and_add(&isRecursive, 1)) {\
	    func##_fp real;\
	    SETSYM_OR_FAIL_V(real, #func, version);	\
	    rctype rc = real(__VA_ARGS__);\
	    __sync_fetch_and_sub(&isRecursive, 1);\
            return rc;\
    }\
    PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);\
    if (!worker || (worker->activeContext != EXECTX_PLUGIN && worker->activeContext != EXECTX_PTH)) { \
	    func##_fp real;\
	    SETSYM_OR_FAIL_V(real, #func, version);	\
	    rctype rc = real(__VA_ARGS__);\
            __sync_fetch_and_sub(&isRecursive, 1);\
	    return rc;\
    }\
    if (worker->activeContext != EXECTX_PLUGIN) {\
	    rctype rc = worker->ftable.func(__VA_ARGS__);\
	    __sync_fetch_and_sub(&isRecursive, 1);\
	    return rc;\
    }\
    __sync_fetch_and_sub(&isRecursive, 1);\

#define _SHADOW_GUARD(rctype, func, ...) {			\
		_FTABLE_GUARD(rctype, func, __VA_ARGS__);	\
		worker->activeContext = EXECTX_PTH;		\
		rctype rc = worker->ftable.func(__VA_ARGS__);	\
		worker->activeContext = EXECTX_PLUGIN;		\
		return rc;					\
	}							\


#define _SHADOW_GUARD_VOID(func, ...) {			\
		_FTABLE_GUARD_VOID(func, __VA_ARGS__);	\
		worker->activeContext = EXECTX_PTH;		\
		worker->ftable.func(__VA_ARGS__);	\
		worker->activeContext = EXECTX_PLUGIN;		\
		return;					\
	}							\

#define _SHD_DL_BODY(rctype, func, ...)					\
  {									\
    PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);	\
    assert(worker);							\
    ExecutionContext e = worker->activeContext;				\
    worker->activeContext = EXECTX_PTH;					\
    rctype rc = worker->ftable.func(__VA_ARGS__);			\
    worker->activeContext = e;						\
    return rc;}

int shd_dl_sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
	return sigprocmask(how, set, oset);
}

int __cxa_atexit(void (*f)(void*), void * arg, void * dso_handle) {
	_FTABLE_GUARD(int, __cxa_atexit, f, arg, dso_handle);
	ExecutionContext e = worker->activeContext;
	worker->activeContext = EXECTX_PTH;
	int rc = worker->ftable.bitcoindplugin_cxa_atexit(f, arg, dso_handle);
	worker->activeContext = e;
	return rc;
}

int on_exit(void (*function)(int , void *), void *arg) { assert(0); }
int atexit(void (*f)(void)) {
	assert(0);
	return 0;
}

ssize_t real_fprintf(FILE *stream, const char *format, ...) {
	char buf[1024];
	va_list ap;
	va_start(ap, format);
	int s = vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (worker) {
		ExecutionContext e = worker->activeContext;
		worker->activeContext = EXECTX_SHADOW;
		int rc = worker->ftable.write(fileno(stream), buf, s);
		worker->activeContext = e;
		return rc;
	} else {
		write_fp _write;
		SETSYM_OR_FAIL(_write, "write");
		return _write(fileno(stream), buf, s);
	}
}

int shd_dl_gettimeofday(struct timeval *tv, struct timezone *tz) {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	assert(worker);
	ExecutionContext e = worker->activeContext;
	worker->activeContext = EXECTX_PTH;
	int rc = gettimeofday(tv, tz);
	worker->activeContext = e;
	tv->tv_sec += SHADOW_TIMER_OFFSET;
	return rc;
}
int shd_dl_accept(int fd, struct sockaddr* addr, socklen_t* addr_len) _SHD_DL_BODY(int, accept, fd, addr, addr_len);
ssize_t shd_dl_read(int fp, void *d, size_t s) _SHD_DL_BODY(ssize_t, read, fp, d, s);
ssize_t shd_dl_write(int fp, const void *d, size_t s) _SHD_DL_BODY(ssize_t, write, fp, d, s);
int shd_dl_connect(int fd, const struct sockaddr* addr, socklen_t len) _SHD_DL_BODY(int, connect, fd, addr, len);
ssize_t shd_dl_recv(int fd, void *buf, size_t n, int flags) _SHD_DL_BODY(ssize_t, recv, fd, buf, n, flags);
ssize_t shd_dl_recvfrom(int fd, void *buf, size_t n, int flags, struct sockaddr* addr, socklen_t *restrict addr_len) _SHD_DL_BODY(ssize_t, recvfrom, fd, buf, n, flags, addr, addr_len);
ssize_t shd_dl_send(int fd, const void *buf, size_t n, int flags) _SHD_DL_BODY(ssize_t, send, fd, buf, n, flags);
ssize_t shd_dl_sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr* addr, socklen_t addr_len) _SHD_DL_BODY(ssize_t, sendto, fd, buf, n, flags, addr, addr_len);
ssize_t shd_dl_readv(int fd, const struct iovec *iov, int iovcnt) { assert(0); }
ssize_t shd_dl_writev(int fd, const struct iovec *iov, int iovcnt) { assert(0); }
int shd_dl_select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout) { assert(0); }
pid_t shd_dl_fork(void) { assert(0); }

/* forward declarations */
static void _shadowtorpreload_cryptoSetup(int);
static void _shadowtorpreload_cryptoTeardown();


#define _WORKER_SET(func) SETSYM_OR_FAIL(worker->ftable.func, #func)
#define _PTH_WORKER_SET(plugin, pthfunc) g_assert(g_module_symbol(handle, #pthfunc, (gpointer*)&worker-> plugin##_pthtable.pthfunc))

#define _PTH_WORKERS(plugin) \
		_PTH_WORKER_SET(plugin, pth_read);\
		_PTH_WORKER_SET(plugin, pth_write);\
		_PTH_WORKER_SET(plugin, pth_spawn);\
		_PTH_WORKER_SET(plugin, pth_usleep);\
		_PTH_WORKER_SET(plugin, pth_ctrl);\
		_PTH_WORKER_SET(plugin, pth_sleep);\
		_PTH_WORKER_SET(plugin, pth_nanosleep);\
		_PTH_WORKER_SET(plugin, pth_join);\
		_PTH_WORKER_SET(plugin, pth_spawn);\
		_PTH_WORKER_SET(plugin, pth_init);\
		_PTH_WORKER_SET(plugin, pth_mutex_init);\
		_PTH_WORKER_SET(plugin, pth_mutex_release);\
		_PTH_WORKER_SET(plugin, pth_mutex_acquire);\
		_PTH_WORKER_SET(plugin, pth_cond_init);\
		_PTH_WORKER_SET(plugin, pth_cond_await);\
		_PTH_WORKER_SET(plugin, pth_cond_notify);\
		_PTH_WORKER_SET(plugin, pth_key_create);\
		_PTH_WORKER_SET(plugin, pth_key_delete);\
		_PTH_WORKER_SET(plugin, pth_key_setdata);\
		_PTH_WORKER_SET(plugin, pth_key_getdata);\
		_PTH_WORKER_SET(plugin, pth_attr_of);\
		_PTH_WORKER_SET(plugin, pth_attr_set);\
		_PTH_WORKER_SET(plugin, pth_attr_destroy);\
		_PTH_WORKER_SET(plugin, pth_time);\
		_PTH_WORKER_SET(plugin, pth_event);\
		_PTH_WORKER_SET(plugin, pth_event_status);\
		_PTH_WORKER_SET(plugin, pth_util_select);\
		_PTH_WORKER_SET(plugin, pth_select);\
		_PTH_WORKER_SET(plugin, pth_readv);\
		_PTH_WORKER_SET(plugin, pth_writev);\
		_PTH_WORKER_SET(plugin, pth_pread);\
		_PTH_WORKER_SET(plugin, pth_pwrite);\
		_PTH_WORKER_SET(plugin, pth_recv);\
		_PTH_WORKER_SET(plugin, pth_send);\
		_PTH_WORKER_SET(plugin, pth_recvfrom);\
		_PTH_WORKER_SET(plugin, pth_sendto);\
		_PTH_WORKER_SET(plugin, pth_connect);\
		_PTH_WORKER_SET(plugin, pth_accept);


void pluginpreload_init(GModule* handle, int nLocks) {	
	PluginPreloadWorker* worker;
	if (!pluginWorkerKey) {
		worker = g_new0(PluginPreloadWorker, 1);
	} else {
		worker = pluginWorkerKey;
	}

	__sync_fetch_and_add(&isRecursive, 1);
	/* memory allocation family */
	g_assert(g_module_symbol(handle, "plugin_cxa_atexit", (gpointer*)&worker->ftable.bitcoindplugin_cxa_atexit));
	_WORKER_SET(__cxa_atexit);
	_WORKER_SET(malloc);
	_WORKER_SET(calloc);
	_WORKER_SET(realloc);
	_WORKER_SET(posix_memalign);
	_WORKER_SET(memalign);
#if 0
	_WORKER_SET(aligned_alloc);
#endif
	_WORKER_SET(valloc);
	_WORKER_SET(pvalloc);
	_WORKER_SET(free);
	_WORKER_SET(mmap);


	const char *module_name = g_module_name(handle);
	/* lookup all our required symbols in this worker's module, asserting success */
	_PTH_WORKERS(plugin);

	/* lookup system and pthread calls that exist outside of the plug-in module.
	 * do the lookup here and save to pointer so we dont have to redo the
	 * lookup on every syscall */
	SETSYM_OR_FAIL(worker->ftable.close, "close");
	SETSYM_OR_FAIL(worker->ftable.read, "read");
	SETSYM_OR_FAIL(worker->ftable.write, "write");
	SETSYM_OR_FAIL(worker->ftable.usleep, "usleep");
	SETSYM_OR_FAIL(worker->ftable.nanosleep, "nanosleep");
	SETSYM_OR_FAIL(worker->ftable.sleep, "sleep");
	SETSYM_OR_FAIL(worker->ftable.write, "write");

	/* Crypto global */
	//_WORKER_SET(crypto_global_init);
	//_WORKER_SET(crypto_global_cleanup);

	/* file specific */
	_WORKER_SET(fileno);
	_WORKER_SET(open);
	_WORKER_SET(open64);
	_WORKER_SET(creat);
	_WORKER_SET(fopen);
	_WORKER_SET(fdopen);
	_WORKER_SET(dup);
	_WORKER_SET(dup2);
	_WORKER_SET(dup3);
	_WORKER_SET(fclose);
	_WORKER_SET(__fxstat);
	_WORKER_SET(fstatfs);
	_WORKER_SET(lseek);
	_WORKER_SET(flock);
	_WORKER_SET(fsync);
	_WORKER_SET(ftruncate);
	_WORKER_SET(posix_fallocate);

	/* socket/io family */
	_WORKER_SET(socket);
	_WORKER_SET(socketpair);
	_WORKER_SET(bind);
	_WORKER_SET(connect);
	_WORKER_SET(getsockname);
	_WORKER_SET(getpeername);
	_WORKER_SET(send);
	_WORKER_SET(sendto);
	_WORKER_SET(sendmsg);
	_WORKER_SET(recvmsg);
	_WORKER_SET(recv);
	_WORKER_SET(recvfrom);
	_WORKER_SET(getsockopt);
	_WORKER_SET(setsockopt);
	_WORKER_SET(listen);
	_WORKER_SET(accept);
	_WORKER_SET(accept4);
	_WORKER_SET(shutdown);
	_WORKER_SET(fcntl);
	_WORKER_SET(ioctl);
	_WORKER_SET(pipe);
	_WORKER_SET(pipe2);
	_WORKER_SET(eventfd);

	/* time family */
	_WORKER_SET(gettimeofday);
	_WORKER_SET(time);
	_WORKER_SET(clock_gettime);

	/* name/address family */
	_WORKER_SET(gethostname);
	_WORKER_SET(getaddrinfo);
	_WORKER_SET(freeaddrinfo);
	_WORKER_SET(getnameinfo);
	_WORKER_SET(gethostbyname);
	_WORKER_SET(gethostbyname_r);
	_WORKER_SET(gethostbyname2);
	_WORKER_SET(gethostbyname2_r);
	_WORKER_SET(gethostbyaddr);
	_WORKER_SET(gethostbyaddr_r);

	/* event */
	_WORKER_SET(epoll_create);
	_WORKER_SET(epoll_create1);
	_WORKER_SET(epoll_ctl);
	_WORKER_SET(epoll_wait);
	_WORKER_SET(epoll_pwait);

	/* random family */
	_WORKER_SET(rand);
	_WORKER_SET(rand_r);
	_WORKER_SET(srand);
	_WORKER_SET(random);
	_WORKER_SET(random_r);
	_WORKER_SET(srandom);
	_WORKER_SET(srandom_r);

	/* crypto family */
	_WORKER_SET(CRYPTO_get_locking_callback);
	_WORKER_SET(CRYPTO_get_id_callback);
	_WORKER_SET(SSL_library_init);
	_WORKER_SET(RAND_seed);
	_WORKER_SET(RAND_add);
	_WORKER_SET(RAND_poll);
	_WORKER_SET(RAND_bytes);
	_WORKER_SET(RAND_pseudo_bytes);
	_WORKER_SET(RAND_cleanup);
	_WORKER_SET(RAND_status);
	_WORKER_SET(RAND_get_rand_method);
	_WORKER_SET(RAND_SSLeay);


	/* pthread */
	_WORKER_SET(pthread_key_create);
	_WORKER_SET(pthread_cond_init);
	SETSYM_OR_FAIL(worker->ftable.pthread_create, "pthread_create");
	SETSYM_OR_FAIL(worker->ftable.pthread_detach, "pthread_detach");
	SETSYM_OR_FAIL(worker->ftable.pthread_join, "pthread_join");
	SETSYM_OR_FAIL(worker->ftable.pthread_once, "pthread_once");
	SETSYM_OR_FAIL(worker->ftable.pthread_setspecific, "pthread_setspecific");
	SETSYM_OR_FAIL(worker->ftable.pthread_getspecific, "pthread_getspecific");
	SETSYM_OR_FAIL(worker->ftable.pthread_attr_setdetachstate, "pthread_attr_setdetachstate");
	SETSYM_OR_FAIL(worker->ftable.pthread_attr_getdetachstate, "pthread_attr_getdetachstate");
	SETSYM_OR_FAIL(worker->ftable.pthread_cond_destroy, "pthread_cond_destroy");
	SETSYM_OR_FAIL(worker->ftable.pthread_cond_signal, "pthread_cond_signal");
	SETSYM_OR_FAIL(worker->ftable.pthread_cond_broadcast, "pthread_cond_broadcast");
	SETSYM_OR_FAIL(worker->ftable.pthread_cond_wait, "pthread_cond_wait");
	SETSYM_OR_FAIL(worker->ftable.pthread_cond_timedwait, "pthread_cond_timedwait");
	SETSYM_OR_FAIL(worker->ftable.pthread_mutex_init, "pthread_mutex_init");
	SETSYM_OR_FAIL(worker->ftable.pthread_mutex_destroy, "pthread_mutex_destroy");
	SETSYM_OR_FAIL(worker->ftable.pthread_mutex_lock, "pthread_mutex_lock");
	SETSYM_OR_FAIL(worker->ftable.pthread_mutex_trylock, "pthread_mutex_trylock");
	SETSYM_OR_FAIL(worker->ftable.pthread_mutex_unlock, "pthread_mutex_unlock");

	//g_private_set(&pluginWorkerKey, worker);
	//assert(g_private_get(&pluginWorkerKey));
	pluginWorkerKey = worker;

	assert(sizeof(pthread_t) >= sizeof(pth_t));
	assert(sizeof(pthread_attr_t) >= sizeof(pth_attr_t));
	assert(sizeof(pthread_mutex_t) >= sizeof(pth_mutex_t));
	assert(sizeof(pthread_cond_t) >= sizeof(pth_cond_t));
        assert(sizeof(pthread_key_t) >= sizeof(pth_key_t));

	__sync_fetch_and_sub(&isRecursive, 1);

	_shadowtorpreload_cryptoSetup(nLocks);
	unsigned char a;
	worker->ftable.RAND_bytes(&a, 1);
}

void pluginpreload_setPluginContext(PluginName plg) {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	worker->activeContext = EXECTX_PLUGIN;
	worker->activePlugin = plg;
}
void pluginpreload_setShadowContext() {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	worker->activeContext = EXECTX_SHADOW;
}
void pluginpreload_setPthContext() {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	worker->activeContext = EXECTX_PTH;
}

int CLogPrintStr(const char *str) {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	assert(worker);
	assert(worker->activeContext == EXECTX_PLUGIN);
	return worker->ftable.CLogPrintStr(str);
}

ssize_t write(int fp, const void *d, size_t s) {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	ssize_t rc = 0;
	//real_fprintf(stderr, "write %d\n", worker->activeContext);

	if(worker && worker->activeContext == EXECTX_PLUGIN) {
		real_fprintf(stderr, "write: going to pth fd:%d\n", fp);
		assert(_get_active_pthtable(worker)->pth_write);
		worker->activeContext = EXECTX_PTH;
		rc = _get_active_pthtable(worker)->pth_write(fp, d, s);
		worker->activeContext = EXECTX_PLUGIN;
	} else if (worker) {
		// dont mess with shadow's calls, and dont change context 
		rc = worker->ftable.write(fp, d, s);
	} else {
		real_fprintf(stderr, "write skipping worker\n");
		write_fp write;
		SETSYM_OR_FAIL(write, "write");
		rc = write(fp, d, s);
	}

	return rc;
}

ssize_t read(int fp, void *d, size_t s) {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	ssize_t rc = 0;
	if(worker && worker->activeContext == EXECTX_PLUGIN) {
		real_fprintf(stderr, "read: going to pth fd:%d\n", fp);
		//assert(fp != -1);
		worker->activeContext = EXECTX_PTH;
		rc = _get_active_pthtable(worker)->pth_read(fp, d, s);
		worker->activeContext = EXECTX_PLUGIN;
	} else if (worker) {
		real_fprintf(stderr, "read skipping worker\n");
		// dont mess with shadow's calls, and dont change context 
		rc = worker->ftable.read(fp, d, s);
	} else {
		read_fp read;
		SETSYM_OR_FAIL(read, "read");
		rc = read(fp, d, s);
	}
	
	return rc;
}

int close(int fd) _SHADOW_GUARD(int, close, fd);

int usleep(unsigned int usec) {
	PluginPreloadWorker* worker_ = g_private_get(&pluginWorkerKey);
	_FTABLE_GUARD(int, usleep, usec);
        worker->activeContext = EXECTX_PTH;
	real_fprintf(stderr, "about to pth_usleep\n");
	int rc = _get_active_pthtable(worker)->pth_usleep(usec);
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
	_FTABLE_GUARD(int, nanosleep, rqtp, rmtp);
        worker->activeContext = EXECTX_PTH;
	real_fprintf(stderr, "about to pth_nanosleep\n");
	int rc = _get_active_pthtable(worker)->pth_nanosleep(rqtp, rmtp);
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

unsigned int sleep(unsigned int sec) {
	_FTABLE_GUARD(unsigned int, sleep, sec);
        worker->activeContext = EXECTX_PTH;
	int rc = _get_active_pthtable(worker)->pth_sleep(sec);
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

/* time family */


int gettimeofday(struct timeval *tv, struct timezone *tz) {
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		gettimeofday_fp real;
		SETSYM_OR_FAIL(real, "gettimeofday");
		int rc = real(tv, tz);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (!worker) {
		gettimeofday_fp real;
		SETSYM_OR_FAIL(real, "gettimeofday");
		int rc = real(tv, tz);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	if (worker->activeContext != EXECTX_PLUGIN) {
		int rc = worker->ftable.gettimeofday(tv, tz);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	__sync_fetch_and_sub(&isRecursive, 1);
	worker->activeContext = EXECTX_PTH;
        int rc = worker->ftable.gettimeofday(tv, tz);
	tv->tv_sec += SHADOW_TIMER_OFFSET; // Offset for a reasonable time!
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}
time_t time(time_t *timer) { 
	_FTABLE_GUARD(time_t, time, timer); 
	return worker->ftable.time(timer) + SHADOW_TIMER_OFFSET;
}

/**
 * epoll
 **/

int epoll_create(int size) _SHADOW_GUARD(int, epoll_create, size);
int epoll_create1(int flags) _SHADOW_GUARD(int, epoll_create1, flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) 
	_SHADOW_GUARD(int, epoll_ctl, epfd, op, fd, event)
int epoll_wait(int epfd, struct epoll_event *events,
	       int maxevents, int timeout) 
	_SHADOW_GUARD(int, epoll_wait, epfd, events, maxevents, timeout);
int epoll_pwait(int epfd, struct epoll_event *events,
		int maxevents, int timeout, const sigset_t *ss) 
	_SHADOW_GUARD(int, epoll_pwait, epfd, events, maxevents, timeout, ss);


/* socket/io family */

#define _PTH_GUARD(rctype, func, ...) {				\
		_FTABLE_GUARD(rctype, func, __VA_ARGS__);	\
		assert(worker->activeContext == EXECTX_PLUGIN);	\
		worker->activeContext = EXECTX_PTH;		\
		rctype rc = _get_active_pthtable(worker)->pth_##func(__VA_ARGS__);	\
		worker->activeContext = EXECTX_PLUGIN;		\
		return rc;					\
	}							\

int socket(int domain, int type, int protocol)
	_SHADOW_GUARD(int, socket, domain, type, protocol); 
int socketpair(int domain, int type, int protocol, int fds[2]) _SHADOW_GUARD(int, socketpair, domain, type, protocol, fds);
int bind(int fd, const struct sockaddr* addr, socklen_t len)  _SHADOW_GUARD(int, bind, fd, addr, len);
int getsockname(int fd, struct sockaddr* addr, socklen_t* len) _SHADOW_GUARD(int, getsockname, fd, addr, len);
int connect(int fd, const struct sockaddr* addr, socklen_t len) _PTH_GUARD(int, connect, fd, addr, len);
int getpeername(int fd, struct sockaddr* addr, socklen_t* len) _SHADOW_GUARD(int, getpeername, fd, addr, len);
ssize_t send(int fd, const void *buf, size_t n, int flags) _PTH_GUARD(ssize_t, send, fd, buf, n, flags);
ssize_t sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr* addr, socklen_t addr_len) _PTH_GUARD(ssize_t, sendto, fd, buf, n, flags, addr, addr_len);
ssize_t sendmsg(int fd, const struct msghdr *message, int flags) { assert(0); }//_PTH_GUARD(ssize_t, sendmsg, fd, message, flags);
ssize_t recv(int fd, void *buf, size_t n, int flags) _PTH_GUARD(ssize_t, recv, fd, buf, n, flags);
ssize_t recvfrom(int fd, void *buf, size_t n, int flags, struct sockaddr* addr, socklen_t *restrict addr_len) _PTH_GUARD(ssize_t, recvfrom, fd, buf, n, flags, addr, addr_len);
ssize_t recvmsg(int fd, struct msghdr *message, int flags) { assert(0); }//_SHADOW_GUARD(ssize_t, recvmsg, fd, message, flags);
int getsockopt(int fd, int level, int optname, void* optval, socklen_t* optlen) _SHADOW_GUARD(int, getsockopt, fd, level, optname, optval, optlen);
int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) _SHADOW_GUARD(int, setsockopt, fd, level, optname, optval, optlen);
int listen(int fd, int n) _SHADOW_GUARD(int, listen, fd, n);
int accept(int fd, struct sockaddr* addr, socklen_t* addr_len) _PTH_GUARD(int, accept, fd, addr, addr_len);
int accept4(int fd, struct sockaddr* addr, socklen_t* addr_len, int flags) {
	assert(0);
	//_SHADOW_GUARD(int, accept4, fd, addr, addr_len, flags);
}
int shutdown(int fd, int how) _SHADOW_GUARD(int, shutdown, fd, how);
//ssize_t read(int fd, void *buff, size_t numbytes) { assert(0); }
//ssize_t write(int fd, const void *buff, size_t n) { assert(0); }
//int close(int fd) { assert(0); }
ssize_t readv(int fd, const struct iovec *iov, int iovcnt) { assert(0); }
ssize_t writev(int fd, const struct iovec *iov, int iovcnt) { assert(0); }
ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset) { assert(0); }
ssize_t pwrite(int fd, const void *buf, size_t nbytes, off_t offset) { assert(0); }

int fcntl(int fd, int cmd, ...) {
	va_list farg;
	va_start(farg, cmd);
	fcntl_fp real = NULL;
	if (!real) SETSYM_OR_FAIL(real, "fcntl");
	assert(real);
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		int rc = real(fd, cmd, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (!worker) {
		int rc = real(fd, cmd, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	if (worker->activeContext != EXECTX_PLUGIN) {
		int rc = worker->ftable.fcntl(fd, cmd, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	__sync_fetch_and_sub(&isRecursive, 1);
	worker->activeContext = EXECTX_PTH;
	int rc = worker->ftable.fcntl(fd, cmd, va_arg(farg, mode_t));
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int ioctl(int fd, unsigned long int request, ...) {
	va_list farg;
	va_start(farg, request);
	static ioctl_fp real = NULL;
	if (!real) SETSYM_OR_FAIL(real, "ioctl");
	assert(real);
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		int rc = real(fd, request, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (!worker) {
		int rc = real(fd, request, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	if (worker->activeContext != EXECTX_PLUGIN) {
		int rc = worker->ftable.ioctl(fd, request, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	__sync_fetch_and_sub(&isRecursive, 1);
	worker->activeContext = EXECTX_PTH;
	int rc = worker->ftable.ioctl(fd, request, va_arg(farg, mode_t));
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pipe2(int pipefds[2], int flags) _SHADOW_GUARD(int, pipe2, pipefds, flags);
int pipe(int pipefds[2]) _SHADOW_GUARD(int, pipe, pipefds);
int eventfd(unsigned int initval, int flags) _SHADOW_GUARD(int, eventfd, initval, flags);
int timerfd_create(int clockid, int flags) { assert(0); }
int timerfd_settime(int fd, int flags, const struct itimerspec *new_value, struct itimerspec *curr_value) { assert(0); }
int timerfd_gettime(int fd, struct itimerspec *curr_value) { assert(0); }
int timer_create(clockid_t clockid, struct sigevent *sevp, timer_t *timerid) { assert(0); }

int select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout) {
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	assert(worker);
	assert(worker->activeContext == EXECTX_PLUGIN);
	worker->activeContext = EXECTX_PTH;
	// TODO: put some select/epoll caching logic here
	int rc = _get_active_pthtable(worker)->pth_select(nfds, rfds, wfds, efds, timeout);
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}


/* file specific */

int fileno(FILE *stream) _SHADOW_GUARD(int, fileno, stream);
int open(const char *pathname, int flags, ...) {
	va_list farg;
	va_start(farg, flags);
	static open_fp real = NULL;
	if (!real) SETSYM_OR_FAIL(real, "open");
	assert(real);
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		int rc = real(pathname, flags, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (!worker) {
		int rc = real(pathname, flags, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	if (worker->activeContext != EXECTX_PLUGIN) {
		int rc = worker->ftable.open(pathname, flags, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	__sync_fetch_and_sub(&isRecursive, 1);
	worker->activeContext = EXECTX_PTH;
	int rc = worker->ftable.open(pathname, flags, va_arg(farg, mode_t));
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int open64(const char *pathname, int flags, ...) {
	va_list farg;
	va_start(farg, flags);
	static open64_fp real = NULL;
	if (!real) SETSYM_OR_FAIL(real, "open64");
	assert(real);
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		int rc = real(pathname, flags, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (!worker) {
		int rc = real(pathname, flags, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	if (worker->activeContext != EXECTX_PLUGIN) {
		int rc = worker->ftable.open(pathname, flags, va_arg(farg, mode_t));
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	__sync_fetch_and_sub(&isRecursive, 1);
	worker->activeContext = EXECTX_PTH;
	int rc = worker->ftable.open64(pathname, flags, va_arg(farg, mode_t));
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}
int creat(const char *pathname, mode_t mode) { assert(0); }
FILE *fopen(const char *path, const char *mode) _SHADOW_GUARD(FILE*, fopen, path, mode);
FILE *fdopen(int fd, const char *mode) _SHADOW_GUARD(FILE*, fdopen, fd, mode);
int dup(int oldfd) { assert(0); }
int dup2(int oldfd, int newfd) { assert(0); }
int dup3(int oldfd, int newfd, int flags) { assert(0); }
int fclose(FILE *fp) _SHADOW_GUARD(int, fclose, fp);
int __fxstat (int ver, int fd, struct stat *buf) _SHADOW_GUARD(int, __fxstat, ver, fd, buf);
int fstatfs (int fd, struct statfs *buf) _SHADOW_GUARD(int, fstatfs, fd, buf);
off_t lseek(int fd, off_t offset, int whence) { assert(0); }
int flock(int fd, int operation) { assert(0); }
int posix_fallocate(int fd, off_t offset, off_t len) _SHADOW_GUARD(int, posix_fallocate, fd, offset, len);
int ftruncate(int fd, off_t length) _SHADOW_GUARD(int, ftruncate, fd, length);

//TODO
//int fstatvfs(int fd, struct statvfs *buf);
//
int fsync(int fd) _SHADOW_GUARD(int, fsync, fd);
int fdatasync(int fd) { return fsync(fd); }
int syncfs(int fd) { assert(0); }
//int fallocate(int fd, int mode, off_t offset, off_t len) { assert(0); }
int fexecve(int fd, char *const argv[], char *const envp[]) { assert(0); }
long fpathconf(int fd, int name) { assert(0); }
int fchdir(int fd) { assert(0); }
int fchown(int fd, uid_t owner, gid_t group) { assert(0); }
int fchmod(int fd, mode_t mode) { assert(0); }
int posix_fadvise(int fd, off_t offset, off_t len, int advice) { assert(0); }
int lockf(int fd, int cmd, off_t len) { assert(0); }
int openat(int dirfd, const char *pathname, int flags, mode_t mode) { assert(0); }
int faccessat(int dirfd, const char *pathname, int mode, int flags) { assert(0); }
int unlinkat(int dirfd, const char *pathname, int flags) { assert(0); }
int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) { assert(0); }
int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) { assert(0); }

/* name/address family */



int gethostname(char* name, size_t len) _SHADOW_GUARD(int, gethostname, name, len);
int getaddrinfo(const char *name, const char *service,
		const struct addrinfo *hints, struct addrinfo **res)
	_SHADOW_GUARD(int, getaddrinfo, name, service, hints, res);
int getaddrinfo_a(int mode, struct gaicb *list[], int nitems, struct sigevent *sevp) { return 0; } 
int gai_suspend(struct gaicb *list[], int nitems,
                struct timespec *timeout) { return 0; }
int gai_error(struct gaicb *req) { return 0; }
int gai_cancel(struct gaicb *req) { return 0; }
void freeaddrinfo(struct addrinfo *res) _SHADOW_GUARD_VOID(freeaddrinfo, res);
int getnameinfo(const struct sockaddr* sa, socklen_t salen,
		char * host, socklen_t hostlen, char *serv, socklen_t servlen,
#if (__GLIBC__ > 2 || (__GLIBC__ == 2 && (__GLIBC_MINOR__ < 2 || __GLIBC_MINOR__ > 13)))
		int flags)
#else
	unsigned int flags)
#endif
	_SHADOW_GUARD(int, getnameinfo, sa, salen, host, hostlen, serv, servlen, flags);

struct hostent* gethostbyname(const gchar* name) { assert(0); }

int gethostbyname_r(const gchar *name, struct hostent *ret, gchar *buf,
		    gsize buflen, struct hostent **result, gint *h_errnop) {
	assert(0); }

struct hostent* gethostbyname2(const gchar* name, gint af) {
	assert(0);}
int gethostbyname2_r(const gchar *name, gint af, struct hostent *ret,
		     gchar *buf, gsize buflen, struct hostent **result, gint *h_errnop) {
	assert(0);}
struct hostent* gethostbyaddr(const void* addr, socklen_t len, gint type) { assert(0); }
int gethostbyaddr_r(const void *addr, socklen_t len, gint type,
		    struct hostent *ret, char *buf, gsize buflen, struct hostent **result,
		    gint *h_errnop) { assert(0); }


/* random family */

int rand() _SHADOW_GUARD(int, rand);
int rand_r(unsigned int *seedp) _SHADOW_GUARD(int, rand_r, seedp);
void srand(unsigned int seed) _SHADOW_GUARD_VOID(srand, seed);

long int random() { assert(0); }
int random_r(struct random_data *buf, int32_t *result) { assert(0); }
void srandom(unsigned int seed) _SHADOW_GUARD_VOID(srandom, seed);
int srandom_r(unsigned int seed, struct random_data *buf) { assert(0); }



/* memory allocation family */

void* malloc(size_t size) _SHADOW_GUARD(void*, malloc, size);
//void* calloc(size_t nmemb, size_t size) _SHADOW_GUARD(void*, calloc, nmemb, size);
void* realloc(void *ptr, size_t size) _SHADOW_GUARD(void*, realloc, ptr, size);
void free(void *ptr) _SHADOW_GUARD_VOID(free, ptr);

int posix_memalign(void** memptr, size_t alignment, size_t size) _SHADOW_GUARD(int, posix_memalign, memptr, alignment, size);
void* memalign(size_t blocksize, size_t bytes) _SHADOW_GUARD(void*, memalign, blocksize, bytes);
#if 0
void* aligned_alloc(size_t alignment, size_t size) { assert(0); }
#endif
void* valloc(size_t size) _SHADOW_GUARD(void*, valloc, size);
void* pvalloc(size_t size) _SHADOW_GUARD(void*, pvalloc, size);
void* mmap(void *addr, size_t length, int prot, int flags,
		   int fd, off_t offset) _SHADOW_GUARD(void*, mmap, addr, length, prot, flags, fd, offset);


/* pth context keeping track of */
int pth_shadow_enter() {
	PluginPreloadWorker* worker = pluginWorkerKey;
	assert(worker);
	//real_fprintf(stderr, "pth_shadow_enter:%d\n", worker->activeContext);
	worker->activeContext = EXECTX_PLUGIN;
	return worker->activeContext;
	
}
void pth_shadow_leave(int ctx) {
	PluginPreloadWorker* worker = pluginWorkerKey;
	assert(worker);
	//real_fprintf(stderr, "pth_shadow_leave:%d\n", worker->activeContext);
	worker->activeContext = ctx;
}


/**
 * pthread
 */

/* general success return value */
#ifdef OK
#undef OK
#endif
#define OK 0

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start_routine) (void *), void *arg) {
        _FTABLE_GUARD(int, pthread_create, thread, attr, start_routine, arg);
        worker->activeContext = EXECTX_PTH;
	real_fprintf(stderr, "passing to pth_pthread_create\n");
	pth_attr_t na;
	int rc = 0;
	
	if (thread == NULL || start_routine == NULL) {
		errno = EINVAL;
		rc = EINVAL;
	} else {
		na = (attr != NULL) ? *((pth_attr_t*)attr) : PTH_ATTR_DEFAULT;
		
		*thread = (pthread_t)_get_active_pthtable(worker)->pth_spawn(na, start_routine, arg);
		if (thread == NULL) {
			errno = EAGAIN;
			rc = EAGAIN;
		}
	}
	real_fprintf(stderr, "returning from pth_pthread_create\n");
	worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_join(pthread_t thread, void **retval) {
        _FTABLE_GUARD(int, pthread_join, thread, retval);
        worker->activeContext = EXECTX_PTH;
	real_fprintf(stderr, "pthread_join to pth\n");
	int rc = 0;
        if (!_get_active_pthtable(worker)->pth_join((pth_t)thread, retval)) {
		rc = errno;
        } else if(retval != NULL && *retval == PTH_CANCELED) {
		*retval = PTHREAD_CANCELED;
        }	
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_detach(pthread_t thread) {
        _FTABLE_GUARD(int, pthread_detach, thread);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
        pth_attr_t na;

        if (thread == 0) {
            errno = EINVAL;
            rc = EINVAL;
        } else if ((na = _get_active_pthtable(worker)->pth_attr_of((pth_t)thread)) == NULL ||
                !_get_active_pthtable(worker)->pth_attr_set(na, PTH_ATTR_JOINABLE, FALSE)) {
            rc = errno;
        } else {
            _get_active_pthtable(worker)->pth_attr_destroy(na);
        }

        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
	_FTABLE_GUARD(int, pthread_once, once_control, init_routine);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
        if (once_control == NULL || init_routine == NULL) {
		errno = EINVAL;
		rc = EINVAL;
        } else {
		if (*once_control != 1) {
			worker->activeContext = EXECTX_PLUGIN;
			init_routine();
			worker->activeContext = EXECTX_PTH;
		}
		*once_control = 1;
        }
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*)) {
	_FTABLE_GUARD(int, pthread_key_create, key, destructor);
        worker->activeContext = EXECTX_PTH;
	real_fprintf(stderr, "pthread_key_create:%p\n", key);
	_get_active_pthtable(worker)->pth_init();
	int rc = 0;
        if (!_get_active_pthtable(worker)->pth_key_create((pth_key_t *)key, destructor)) {
		rc = errno;
        }
	real_fprintf(stderr, "pthread_create:%d\n", *((pth_key_t*)key));
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}


int pthread_setspecific(pthread_key_t key, const void *value)  {
	_FTABLE_GUARD(int, pthread_setspecific, key, value);
        worker->activeContext = EXECTX_PTH;
        //real_fprintf(stderr, "pthread_setspecific:%d %p\n", key, value);
	int rc = 0;
        if (!_get_active_pthtable(worker)->pth_key_setdata((pth_key_t)key, value)) {
		rc = errno;
        }
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

void *pthread_getspecific(pthread_key_t key) {
	static pthread_getspecific_fp real_pthread_getspecific = NULL;
	if (!real_pthread_getspecific) {
		real_pthread_getspecific = dlsym(RTLD_NEXT, "pthread_getspecific");
	}
	assert(real_pthread_getspecific);
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		pthread_getspecific_fp real;
		real = real_pthread_getspecific;
		void * rc = real(key);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = g_private_get(&pluginWorkerKey);
	if (!worker) {
		pthread_getspecific_fp real;
		real = real_pthread_getspecific;
		void * rc = real(key);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	if (worker->activeContext != EXECTX_PLUGIN) {
		void * rc = worker->ftable.pthread_getspecific(key);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	__sync_fetch_and_sub(&isRecursive, 1);
        worker->activeContext = EXECTX_PTH;
        //real_fprintf(stderr, "pthread_getspecific:%dn", key);
	void* pointer = NULL;
        pointer = _get_active_pthtable(worker)->pth_key_getdata((pth_key_t)key);
        worker->activeContext = EXECTX_PLUGIN;
	return pointer;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
	_FTABLE_GUARD(int, pthread_attr_setdetachstate, attr, detachstate);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	assert(0);
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
	_FTABLE_GUARD(int, pthread_attr_getdetachstate, attr, detachstate);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	assert(0);
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int PTHREAD_COND_IS_INITIALIZED(pthread_cond_t cond) {
        return !(((pth_cond_t*)&cond)->cn_state & PTH_COND_INITIALIZED);
	//pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
	//return !strncmp((const char *)&cond, (const char *)&empty, sizeof(pthread_cond_t));
}

int PTHREAD_MUTEX_IS_INITIALIZED(pthread_mutex_t mutex) {
        return !(((pth_mutex_t*)&mutex)->mx_state & PTH_MUTEX_INITIALIZED);
	//pthread_mutex_t empty = PTHREAD_MUTEX_INITIALIZER;
	//return !strncmp((const char *)&mutex, (const char *)&empty, sizeof(pthread_mutex_t));
}


int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) 
{
	_FTABLE_GUARD_V(int, pthread_cond_init, "GLIBC_2.3.2", cond, attr);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	_get_active_pthtable(worker)->pth_init();
	if (cond == NULL)
		rc = EINVAL;
	else if (!_get_active_pthtable(worker)->pth_cond_init((pth_cond_t *)cond))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	_FTABLE_GUARD(int, pthread_cond_destroy, cond);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	if (cond == NULL)
		rc = EINVAL;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	_FTABLE_GUARD(int, pthread_cond_signal, cond);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	if (cond == NULL)
		rc = EINVAL;
	else if (PTHREAD_COND_IS_INITIALIZED(*cond)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_cond_init(cond, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && !_get_active_pthtable(worker)->pth_cond_notify((pth_cond_t *)cond, FALSE))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
	_FTABLE_GUARD(int, pthread_cond_broadcast, cond);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	if (cond == NULL)
		rc = EINVAL;
	else if (PTHREAD_COND_IS_INITIALIZED(*cond)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_cond_init(cond, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && !_get_active_pthtable(worker)->pth_cond_notify((pth_cond_t *)cond, TRUE))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	_FTABLE_GUARD(int, pthread_cond_wait, cond, mutex);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	if (cond == NULL || mutex == NULL)
		rc = EINVAL;
	else if (PTHREAD_COND_IS_INITIALIZED(*cond)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_cond_init(cond, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && PTHREAD_MUTEX_IS_INITIALIZED(*mutex)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_mutex_init(mutex, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && !_get_active_pthtable(worker)->pth_cond_await((pth_cond_t *)cond, (pth_mutex_t *)mutex, NULL))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
              const struct timespec *abstime) {
	_FTABLE_GUARD(int, pthread_cond_wait, cond, mutex);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;

	pth_event_t ev;
	pth_key_t ev_key = PTH_KEY_INIT;

	if (cond == NULL || mutex == NULL || abstime == NULL)
		rc = EINVAL;
	if (!rc && (abstime->tv_sec < 0 || abstime->tv_nsec < 0 || abstime->tv_nsec >= 1000000000))
		rc = EINVAL;
	if (!rc && PTHREAD_COND_IS_INITIALIZED(*cond)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_cond_init(cond, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && PTHREAD_MUTEX_IS_INITIALIZED(*mutex)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_mutex_init(mutex, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc)
		ev = _get_active_pthtable(worker)->pth_event(PTH_EVENT_TIME|PTH_MODE_STATIC, &ev_key,
			       _get_active_pthtable(worker)->pth_time(abstime->tv_sec, (abstime->tv_nsec)/1000)
			       );
	if (!rc && !_get_active_pthtable(worker)->pth_cond_await((pth_cond_t *)cond, (pth_mutex_t *)mutex, ev))
		rc = errno;
	if (!rc && _get_active_pthtable(worker)->pth_event_status(ev) == PTH_STATUS_OCCURRED)
		rc = ETIMEDOUT;

        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
	_FTABLE_GUARD(int, pthread_mutex_init, mutex, attr);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	_get_active_pthtable(worker)->pth_init();
	//real_fprintf(stderr, "pthread_mutex_init: %p\n", mutex);
	if (mutex == NULL) {
		rc = EINVAL;
	} else if (!_get_active_pthtable(worker)->pth_mutex_init((pth_mutex_t *)mutex))
		rc = errno;
	if (!rc) assert(!PTHREAD_MUTEX_IS_INITIALIZED(*mutex));
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
	_FTABLE_GUARD(int, pthread_mutex_destroy, mutex);
        worker->activeContext = EXECTX_PTH;
	int rc = 0;
	if (mutex == NULL)
		rc = EINVAL;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
	_FTABLE_GUARD(int, pthread_mutex_lock, mutex);
        worker->activeContext = EXECTX_PTH;
	//real_fprintf(stderr, "pthread_mutex_lock:%p\n", mutex);
	int rc = 0;
	if (mutex == NULL) {
		rc = EINVAL;
	} else if (PTHREAD_MUTEX_IS_INITIALIZED(*mutex)) {
		real_fprintf(stderr, "pthread_mutex_lock: initializing\n");
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_mutex_init(mutex, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && !_get_active_pthtable(worker)->pth_mutex_acquire((pth_mutex_t *)mutex, FALSE, NULL))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
	_FTABLE_GUARD(int, pthread_mutex_trylock, mutex);
        worker->activeContext = EXECTX_PTH;
	//real_fprintf(stderr, "pthread_mutex_trylock:%p\n", mutex);
	int rc = 0;
	if (mutex == NULL) {
		rc = EINVAL;
	} else if (PTHREAD_MUTEX_IS_INITIALIZED(*mutex)) {
		worker->activeContext = EXECTX_PLUGIN;
		if (pthread_mutex_init(mutex, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && !_get_active_pthtable(worker)->pth_mutex_acquire((pth_mutex_t *)mutex, TRUE, NULL))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
	_FTABLE_GUARD(int, pthread_mutex_unlock, mutex);
        worker->activeContext = EXECTX_PTH;
	//real_fprintf(stderr, "pthread_mutex_unlock:%p\n", mutex);
	int rc = 0;
	if (mutex == NULL) {
		rc = EINVAL;
	} else if (PTHREAD_MUTEX_IS_INITIALIZED(*mutex)) {
		worker->activeContext = EXECTX_PLUGIN;
		assert(0);
		real_fprintf(stderr, "pthread_mutex_unlock: initializing\n");
		if (pthread_mutex_init(mutex, NULL) != OK)
			rc = errno;
		worker->activeContext = EXECTX_PTH;
	}
	if (!rc && !_get_active_pthtable(worker)->pth_mutex_release((pth_mutex_t *)mutex))
		rc = errno;
        worker->activeContext = EXECTX_PLUGIN;
	return rc;
}


/********************************************************************************
 * the code below provides support for multi-threaded openssl.
 * we need global state here to manage locking for all threads, so we use the
 * preload library because the symbols here do not get hoisted and copied
 * for each instance of the plug-in.
 *
 * @see '$man CRYPTO_lock'
 ********************************************************************************/

/*
 * Holds global state for the Tor preload library. This state will not be
 * copied for every instance of Tor, or for every instance of the plugin.
 * Therefore, we must ensure that modifications to it are properly locked
 * for thread safety, it is not initialized multiple times, etc.
 */
typedef struct _PreloadGlobal PreloadGlobal;
struct _PreloadGlobal {
	gboolean initialized;
	gint nTorCryptoNodes;
	gint nThreads;
	gint numCryptoThreadLocks;
	GRWLock* cryptoThreadLocks;
};

G_LOCK_DEFINE_STATIC(shadowtorpreloadGlobalLock);
PreloadGlobal shadowtorpreloadGlobalState = {FALSE, 0, 0, 0, NULL};

/**
 * these init and cleanup Tor functions are called to handle openssl.
 * they must be globally locked and only called once globally to avoid openssl errors.
 */

PluginPreloadWorker *_preload_getworker() {
	return g_private_get(&pluginWorkerKey);
}


int crypto_global_init(int useAccel, const char *accelName, const char *accelDir) {
	G_LOCK(shadowtorpreloadGlobalLock);

	gint result = 0;
	if(++shadowtorpreloadGlobalState.nTorCryptoNodes == 1) {
		result = _preload_getworker()->ftable.crypto_global_init(useAccel, accelName, accelDir);
	}

	G_UNLOCK(shadowtorpreloadGlobalLock);
	return result;
}
int crypto_global_cleanup(void) {
	G_LOCK(shadowtorpreloadGlobalLock);

	gint result = 0;
	if(--shadowtorpreloadGlobalState.nTorCryptoNodes == 0) {
		result = _preload_getworker()->ftable.crypto_global_cleanup();
	}

	G_UNLOCK(shadowtorpreloadGlobalLock);
	return result;
}

static unsigned long _shadowtorpreload_getIDFunc() {
	/* return an ID that is unique for each thread */
	return (unsigned long)(g_private_get(&pluginWorkerKey));
}

unsigned long (*CRYPTO_get_id_callback(void))(void) {
	return _shadowtorpreload_getIDFunc;
}

static void _shadowtorpreload_cryptoLockingFunc(int mode, int n, const char *file, int line) {
	assert(shadowtorpreloadGlobalState.initialized);

	GRWLock* lock = &(shadowtorpreloadGlobalState.cryptoThreadLocks[n]);
	assert(lock);

	if(mode & CRYPTO_LOCK) {
		if(mode & CRYPTO_READ) {
			g_rw_lock_reader_lock(lock);
		} else if(mode & CRYPTO_WRITE) {
			g_rw_lock_writer_lock(lock);
		}
	} else if(mode & CRYPTO_UNLOCK) {
		if(mode & CRYPTO_READ) {
			g_rw_lock_reader_unlock(lock);
		} else if(mode & CRYPTO_WRITE) {
			g_rw_lock_writer_unlock(lock);
		}
	}
}

void (*CRYPTO_get_locking_callback(void))(int mode,int type,const char *file,
					  int line) {
	return _shadowtorpreload_cryptoLockingFunc;
}

static void _shadowtorpreload_cryptoSetup(int numLocks) {
	G_LOCK(shadowtorpreloadGlobalLock);

	if(!shadowtorpreloadGlobalState.initialized) {
		shadowtorpreloadGlobalState.numCryptoThreadLocks = numLocks;

		shadowtorpreloadGlobalState.cryptoThreadLocks = g_new0(GRWLock, numLocks);
		for(gint i = 0; i < numLocks; i++) {
			g_rw_lock_init(&(shadowtorpreloadGlobalState.cryptoThreadLocks[i]));
		}

		shadowtorpreloadGlobalState.initialized = TRUE;
	}

	shadowtorpreloadGlobalState.nThreads++;

	G_UNLOCK(shadowtorpreloadGlobalLock);
}

static void _shadowtorpreload_cryptoTeardown() {
	G_LOCK(shadowtorpreloadGlobalLock);

	if(shadowtorpreloadGlobalState.initialized &&
            shadowtorpreloadGlobalState.cryptoThreadLocks &&
	   --shadowtorpreloadGlobalState.nThreads == 0) {

		for(int i = 0; i < shadowtorpreloadGlobalState.numCryptoThreadLocks; i++) {
			g_rw_lock_clear(&(shadowtorpreloadGlobalState.cryptoThreadLocks[i]));
		}

		g_free(shadowtorpreloadGlobalState.cryptoThreadLocks);
		shadowtorpreloadGlobalState.cryptoThreadLocks = NULL;
		shadowtorpreloadGlobalState.initialized = 0;
	}

	G_UNLOCK(shadowtorpreloadGlobalLock);
}

/********************************************************************************
 * end code that supports multi-threaded openssl.
 ********************************************************************************/

/* SSL */
int SSL_library_init() {
	//_SHADOW_GUARD(int, SSL_library_init);
        return 0;
}
void SSL_load_error_strings() {
	// FIXME
}
void __do_global_dtors_aux() {
}
void RAND_seed(const void *buf, int num) { assert(0); }
void RAND_add(const void *buf, int num, double entropy) _SHADOW_GUARD_VOID(RAND_add, buf, num, entropy);
int RAND_poll() { assert(0); }
int RAND_bytes(unsigned char *buf, int num) _SHADOW_GUARD(int, RAND_bytes, buf, num);
int RAND_pseudo_bytes(unsigned char *buf, int num) _SHADOW_GUARD(int, RAND_pseudo_bytes, buf, num);
void RAND_cleanup() { assert(0); }
int RAND_status() { assert(0); }
const void *RAND_get_rand_method() _SHADOW_GUARD(const void*, RAND_get_rand_method);

/* Inception! g_private_get is special because it's called by shadow before deciding
   whether to look up symbols again from the beginning. */
#ifdef g_private_get
#undef g_private_get
#endif

typedef gpointer (*g_private_get_fp)(GPrivate *key);

gpointer g_private_get(GPrivate *key) {
	static g_private_get_fp real = NULL;
	if (!real) SETSYM_OR_FAIL(real, "g_private_get");
	assert(real);
	if(__sync_fetch_and_add(&isRecursive, 1)) {
		gpointer rc = real(key);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	PluginPreloadWorker* worker = pluginWorkerKey;
	if (!worker) {
		gpointer rc = real(key);
		__sync_fetch_and_sub(&isRecursive, 1);
		return rc;
	}
	ExecutionContext e = worker->activeContext;
	worker->activeContext = EXECTX_SHADOW;
	gpointer rc = real(key);
	worker->activeContext = e;
	__sync_fetch_and_sub(&isRecursive, 1);
	return rc;
}
