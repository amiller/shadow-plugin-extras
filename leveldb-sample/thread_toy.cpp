// A producer consumer queue
#include <boost/thread/thread.hpp>
#include <iostream>

boost::condition_variable cond1, cond2;
boost::mutex lock1, lock2;
const int iterations = 5;

void producer(void) 
{
  using namespace std;
  for (int i = 0; i != iterations; ++i) {
    cout << "Producer " << i << endl;
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
    cond1.notify_one();
    {
      boost::unique_lock<boost::mutex> lock(lock2);
      cond2.wait(lock);
    }
  }
}

void consumer(void)
{
  using namespace std;
  for (int i = 0; i < iterations; i++) {
    {
      boost::unique_lock<boost::mutex> lock(lock1);
      cond1.wait(lock);
    }
    cout << "Consumer " << i << endl;
    cond2.notify_one();
  }
}

void doWork()
{
  using namespace std;
  boost::thread cons(&consumer);
  boost::thread prod(&producer);
  prod.join();
  cons.join();
}
