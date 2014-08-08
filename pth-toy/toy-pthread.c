#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

const int iterations = 5;

static void producer(void) {
  int i;
  for (i = 0; i != iterations; ++i) {
    printf("Producer %d\n", i);
    // sleep 200 ms
    usleep(200*1000);
    pthread_mutex_lock(&lock1);
    pthread_cond_signal(&cond1);
    pthread_mutex_unlock(&lock1);
    {
      pthread_mutex_lock(&lock2);
      pthread_cond_wait(&cond2, &lock2);
      pthread_mutex_unlock(&lock2);
    }
  }
}

static void consumer(void)
{
  int i;
  for (i = 0; i < iterations; i++) {
    {
      pthread_mutex_lock(&lock1);
      pthread_cond_wait(&cond1, &lock1);
      pthread_mutex_unlock(&lock1);
    }
    printf("Consumer %d\n", i);
    pthread_mutex_lock(&lock2);
    pthread_cond_signal(&cond2);
    pthread_mutex_unlock(&lock2);
  }
}

void toy_pthread_doWork()
{
  pthread_t prod, cons;
  pthread_create(&cons, NULL, (void * (*) (void *)) consumer, NULL);
  pthread_create(&prod, NULL, (void * (*) (void *)) producer, NULL);  
  pthread_join(cons, NULL);
  pthread_join(prod, NULL);
}
