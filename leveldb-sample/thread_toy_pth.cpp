// A producer consumer queue
#include <pth.h>
#include <iostream>

pth_cond_t cond1 = PTH_COND_INIT, cond2 = PTH_COND_INIT;
pth_mutex_t lock1 = PTH_MUTEX_INIT, lock2 = PTH_MUTEX_INIT;
const int iterations = 5;

void producer(void) {
  using namespace std;
  for (int i = 0; i != iterations; ++i) {
    cout << "Producer " << i << endl;
    // sleep 200 ms
    pth_nap(pth_time(0,50000));
    pth_mutex_acquire(&lock1, FALSE, NULL);
    pth_cond_notify(&cond1, FALSE);
    pth_mutex_release(&lock1);
    {
      pth_mutex_acquire(&lock2, FALSE, NULL);
      pth_cond_await(&cond2, &lock2, NULL);
      pth_mutex_release(&lock2);
    }
  }
}

void consumer(void)
{
  using namespace std;
  for (int i = 0; i < iterations; i++) {
    {
      pth_mutex_acquire(&lock1, FALSE, NULL);
      pth_cond_await(&cond1, &lock1, NULL);
      pth_mutex_release(&lock1);
    }
    cout << "Consumer " << i << endl;
    pth_mutex_acquire(&lock2, FALSE, NULL);
    pth_cond_notify(&cond2, FALSE);
    pth_mutex_release(&lock2);
  }
}

void doWork()
{
  pth_init();

  using namespace std;
  pth_attr_t attr = pth_attr_new();
  pth_attr_init(attr);
  pth_t cons = pth_spawn(attr, (void * (*) (void *)) consumer, NULL);
  pth_t prod = pth_spawn(attr, (void * (*) (void *)) producer, NULL);
  int prio;
  cout << "Std:" << PTH_PRIO_STD << " Min: " << PTH_PRIO_MIN << " Max: " << PTH_PRIO_MAX << endl;
  while (pth_ctrl(PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_NEW | PTH_CTRL_GETTHREADS_WAITING)) {
    while (pth_ctrl(PTH_CTRL_GETTHREADS_READY | PTH_CTRL_GETTHREADS_NEW)) {
      cout << " READY: " << pth_ctrl(PTH_CTRL_GETTHREADS_READY|PTH_CTRL_GETTHREADS_NEW) << endl;
      cout << "Master activate" << endl;
      //pth_attr_get(pth_attr_of(prod), PTH_ATTR_PRIO, &prio);
      //cout << "[Prod]:" << prio << " " << pth_ctrl(PTH_CTRL_GETPRIO, prod) << endl;
      
      //pth_attr_get(pth_attr_of(cons), PTH_ATTR_PRIO, &prio);
      //cout << "[Cons]:" << prio << " " << pth_ctrl(PTH_CTRL_GETPRIO, cons) << endl;
      
      //pth_attr_get(pth_attr_of(pth_self()), PTH_ATTR_PRIO, &prio);
      //cout << "[Master]:" << prio << " " << pth_ctrl(PTH_CTRL_GETPRIO, pth_self()) << endl;

      pth_attr_set(pth_attr_of(pth_self()), PTH_ATTR_PRIO, PTH_PRIO_MIN);
      pth_yield(NULL);
    }
    cout << "napping" << endl;
    pth_nap(pth_time(0,50000));
  }
  //pth_join(prod, NULL);
  //pth_join(cons, NULL);
}
