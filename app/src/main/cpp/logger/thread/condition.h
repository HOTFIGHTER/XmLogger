#ifndef CONDITION_H_
#define CONDITION_H_

#include <errno.h>
#include <pthread.h>
#include <climits>
#include <sys/time.h>

#include <logger/thread/atomic_oper.h>
#include <logger/thread/lock.h>
#include <logger/thread/mutex.h>

class Condition {
 public:
  Condition()
      : condition_(), mutex_(), anyway_notify_(0) {
    int ret = pthread_cond_init(&condition_, 0);

    if (EAGAIN == ret) assert(0 == EAGAIN);
    else if (ENOMEM == ret) assert(0 == ENOMEM);
    else if (EBUSY == ret) assert(0 == EBUSY);
    else if (EINVAL == ret) assert(0 == EINVAL);
    else if (0 != ret) {
      //ASSERT2(0 == ret, "%d", ret);
    }
  }

  ~Condition() {
    int ret = pthread_cond_destroy(&condition_);

    if (EBUSY == ret) assert(0 == EBUSY);
    else if (EINVAL == ret) assert(0 == EINVAL);
    else if (0 != ret) {
      // ASSERT2(0 == ret, "%d", ret);
    }
  }

  void wait(ScopedLock &lock) {
    assert(lock.internal().islocked());

    int ret = 0;

    if (!atomic_cas32(&anyway_notify_, 0, 1)) {
      ret = pthread_cond_wait(&condition_, &(lock.internal().internal()));
    }

    anyway_notify_ = 0;

    if (EPERM == ret) assert(0 == EPERM);
    else if (EINVAL == ret) assert(0 == EINVAL);
    else if (0 != ret) {
      // ASSERT2(0 == ret, "%d", ret);
    }
  }

  int wait(ScopedLock &lock, long millisecond) {
    assert(lock.internal().islocked());
    struct timespec ts;
    makeTimeout(&ts, millisecond);

    int ret = 0;

    if (!atomic_cas32(&anyway_notify_, 0, 1)) {
      ret = pthread_cond_timedwait(&condition_, &(lock.internal().internal()), &ts);
    }

    anyway_notify_ = 0;

    if (ETIMEDOUT == ret || 0 == ret) return ret;

    if (EPERM == ret) assert(0 == EPERM);
    else if (EINVAL == ret) assert(0 == EINVAL);
    else if (0 != ret) {
      //ASSERT2(0 == ret, "%d", ret);
    }

    return ret;
  }

  void wait() {
    ScopedLock scopedLock(mutex_);
    wait(scopedLock);
  }

  int wait(long millisecond) {
    ScopedLock scopedLock(mutex_);
    return wait(scopedLock, millisecond);
  }

  void notifyOne() {
    int ret = pthread_cond_signal(&condition_);

    if (EINVAL == ret) assert(0 == EINVAL);
    else if (0 != ret) {
      //ASSERT2(0 == ret, "%d", ret);
    }
  }

  void notifyOne(ScopedLock &lock) {
    assert(lock.internal().islocked());
    notifyOne();
  }

  void notifyAll(bool anywaynotify = false) {
    if (anywaynotify) anyway_notify_ = 1;

    int ret = pthread_cond_broadcast(&condition_);

    if (EINVAL == ret) assert(0 == EINVAL);
    else if (0 != ret) {
      // ASSERT2(0 == ret, "%d", ret);
    }
  }

  void notifyAll(ScopedLock &lock, bool anywaynotify = false) {
    assert(lock.internal().islocked());
    notifyAll(anywaynotify);
  }

  void cancelAnyWayNotify() { anyway_notify_ = 0; }

 private:
  static void makeTimeout(struct timespec *pts, long millisecond) {
    struct timeval tv;
    gettimeofday(&tv, 0);
    pts->tv_sec = millisecond / 1000 + tv.tv_sec;
    pts->tv_nsec = (millisecond % 1000) * 1000 * 1000 + tv.tv_usec * 1000;

    pts->tv_sec += pts->tv_nsec / (1000 * 1000 * 1000);
    pts->tv_nsec = pts->tv_nsec % (1000 * 1000 * 1000);
  }

 private:
  Condition(const Condition &);
  Condition &operator=(const Condition &);

 private:
  pthread_cond_t condition_;
  Mutex mutex_;
  volatile unsigned int anyway_notify_;
};


#endif
