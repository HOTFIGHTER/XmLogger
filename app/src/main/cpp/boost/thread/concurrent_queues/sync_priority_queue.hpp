// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_SYNC_PRIORITY_QUEUE
#define BOOST_THREAD_SYNC_PRIORITY_QUEUE

#include <boost/thread/detail/config.hpp>

#include <boost/thread/concurrent_queues/detail/sync_queue_base.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/csbl/vector.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/atomic.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>

#include <exception>
#include <queue>
#include <utility>

#include <boost/config/abi_prefix.hpp>

namespace mars_boost {}
namespace boost = mars_boost;
namespace mars_boost {
namespace detail {

template<
    class Type,
    class Container = csbl::vector <Type>,
    class Compare = std::less <Type>
>
class priority_queue {
 private:
  Container _elements;
  Compare _compare;
 public:
  typedef Type value_type;
  typedef typename Container::size_type size_type;

  explicit priority_queue(const Compare &compare = Compare())
      : _elements(), _compare(compare) {}

  size_type size() const {
    return _elements.size();
  }

  bool empty() const {
    return _elements.empty();
  }

  void push(Type const &element) {
    _elements.push_back(element);
    std::push_heap(_elements.begin(), _elements.end(), _compare);
  }
  void push(BOOST_RV_REF(Type) element) {
    _elements.push_back(mars_boost::move(element));
    std::push_heap(_elements.begin(), _elements.end(), _compare);
  }

  void pop() {
    std::pop_heap(_elements.begin(), _elements.end(), _compare);
    _elements.pop_back();
  }
  Type pull() {
    Type result = mars_boost::move(_elements.front());
    pop();
    return mars_boost::move(result);
  }

  Type const &top() {
    return _elements.front();
  }
};
}

namespace concurrent {
template<class ValueType,
    class Container = csbl::vector <ValueType>,
    class Compare = std::less<typename Container::value_type> >
class sync_priority_queue
    : public detail::sync_queue_base<ValueType,
                                     mars_boost::detail::priority_queue<ValueType,
                                                                        Container,
                                                                        Compare> > {
  typedef detail::sync_queue_base<ValueType,
                                  mars_boost::detail::priority_queue<ValueType,
                                                                     Container,
                                                                     Compare> > super;

 public:
  typedef ValueType value_type;
  //typedef typename super::value_type value_type; // fixme
  typedef typename super::underlying_queue_type underlying_queue_type;
  typedef typename super::size_type size_type;
  typedef typename super::op_status op_status;

  typedef chrono::steady_clock clock;
 protected:

 public:
  sync_priority_queue() {}

  ~sync_priority_queue() {
    if (!super::closed()) {
      super::close();
    }
  }

  void push(const ValueType &elem);
  void push(BOOST_THREAD_RV_REF(ValueType) elem);

  queue_op_status try_push(const ValueType &elem);
  queue_op_status try_push(BOOST_THREAD_RV_REF(ValueType) elem);

  ValueType pull();

  void pull(ValueType &);

  queue_op_status pull_until(const clock::time_point &, ValueType &);
  queue_op_status pull_for(const clock::duration &, ValueType &);

  queue_op_status try_pull(ValueType &elem);
  queue_op_status wait_pull(ValueType &elem);
  queue_op_status nonblocking_pull(ValueType &);

 private:
  void push(unique_lock <mutex> &, const ValueType &elem);
  void push(lock_guard <mutex> &, const ValueType &elem);
  void push(unique_lock <mutex> &, BOOST_THREAD_RV_REF(ValueType) elem);
  void push(lock_guard <mutex> &, BOOST_THREAD_RV_REF(ValueType) elem);

  queue_op_status try_push(unique_lock <mutex> &, const ValueType &elem);
  queue_op_status try_push(unique_lock <mutex> &, BOOST_THREAD_RV_REF(ValueType) elem);

  ValueType pull(unique_lock <mutex> &);
  ValueType pull(lock_guard <mutex> &);

  void pull(unique_lock <mutex> &, ValueType &);
  void pull(lock_guard <mutex> &, ValueType &);

  queue_op_status try_pull(lock_guard <mutex> &lk, ValueType &elem);
  queue_op_status try_pull(unique_lock <mutex> &lk, ValueType &elem);

  queue_op_status wait_pull(unique_lock <mutex> &lk, ValueType &elem);

  queue_op_status nonblocking_pull(unique_lock <mutex> &lk, ValueType &);

  sync_priority_queue(const sync_priority_queue &);
  sync_priority_queue &operator=(const sync_priority_queue &);
  sync_priority_queue (BOOST_THREAD_RV_REF(sync_priority_queue));
  sync_priority_queue &operator=(BOOST_THREAD_RV_REF(sync_priority_queue));
}; //end class


//////////////////////
template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::push(unique_lock <mutex> &lk, const T &elem) {
  super::throw_if_closed(lk);
  super::data_.push(elem);
  super::notify_not_empty_if_needed(lk);
}
template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::push(lock_guard <mutex> &lk, const T &elem) {
  super::throw_if_closed(lk);
  super::data_.push(elem);
  super::notify_not_empty_if_needed(lk);
}
template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::push(const T &elem) {
  lock_guard <mutex> lk(super::mtx_);
  push(lk, elem);
}

//////////////////////
template<class T, class Container, class Cmp>
void
sync_priority_queue<T, Container, Cmp>::push(unique_lock <mutex> &lk, BOOST_THREAD_RV_REF(T) elem) {
  super::throw_if_closed(lk);
  super::data_.push(mars_boost::move(elem));
  super::notify_not_empty_if_needed(lk);
}
template<class T, class Container, class Cmp>
void
sync_priority_queue<T, Container, Cmp>::push(lock_guard <mutex> &lk, BOOST_THREAD_RV_REF(T) elem) {
  super::throw_if_closed(lk);
  super::data_.push(mars_boost::move(elem));
  super::notify_not_empty_if_needed(lk);
}
template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::push(BOOST_THREAD_RV_REF(T) elem) {
  lock_guard <mutex> lk(super::mtx_);
  push(lk, mars_boost::move(elem));
}

//////////////////////
template<class T, class Container, class Cmp>
queue_op_status sync_priority_queue<T, Container, Cmp>::try_push(const T &elem) {
  lock_guard <mutex> lk(super::mtx_);
  if (super::closed(lk)) return queue_op_status::closed;
  push(lk, elem);
  return queue_op_status::success;
}

//////////////////////
template<class T, class Container, class Cmp>
queue_op_status sync_priority_queue<T, Container, Cmp>::try_push(BOOST_THREAD_RV_REF(T) elem) {
  lock_guard <mutex> lk(super::mtx_);
  if (super::closed(lk)) return queue_op_status::closed;
  push(lk, mars_boost::move(elem));

  return queue_op_status::success;
}

//////////////////////
template<class T, class Container, class Cmp>
T sync_priority_queue<T, Container, Cmp>::pull(unique_lock <mutex> &) {
  return super::data_.pull();
}
template<class T, class Container, class Cmp>
T sync_priority_queue<T, Container, Cmp>::pull(lock_guard <mutex> &) {
  return super::data_.pull();
}

template<class T, class Container, class Cmp>
T sync_priority_queue<T, Container, Cmp>::pull() {
  unique_lock <mutex> lk(super::mtx_);
  super::wait_until_not_empty(lk);
  return pull(lk);
}

//////////////////////
template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::pull(unique_lock <mutex> &, T &elem) {
  elem = super::data_.pull();
}
template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::pull(lock_guard <mutex> &, T &elem) {
  elem = super::data_.pull();
}

template<class T, class Container, class Cmp>
void sync_priority_queue<T, Container, Cmp>::pull(T &elem) {
  unique_lock <mutex> lk(super::mtx_);
  super::wait_until_not_empty(lk);
  pull(lk, elem);
}

//////////////////////
template<class T, class Cont, class Cmp>
queue_op_status
sync_priority_queue<T, Cont, Cmp>::pull_until(const clock::time_point &tp, T &elem) {
  unique_lock <mutex> lk(super::mtx_);
  if (queue_op_status::timeout == super::wait_until_not_empty_until(lk, tp))
    return queue_op_status::timeout;
  pull(lk, elem);
  return queue_op_status::success;
}

//////////////////////
template<class T, class Cont, class Cmp>
queue_op_status
sync_priority_queue<T, Cont, Cmp>::pull_for(const clock::duration &dura, T &elem) {
  return pull_until(clock::now() + dura, elem);
}

//////////////////////
template<class T, class Container, class Cmp>
queue_op_status
sync_priority_queue<T, Container, Cmp>::try_pull(unique_lock <mutex> &lk, T &elem) {
  if (super::empty(lk)) {
    if (super::closed(lk)) return queue_op_status::closed;
    return queue_op_status::empty;
  }
  pull(lk, elem);
  return queue_op_status::success;
}

template<class T, class Container, class Cmp>
queue_op_status
sync_priority_queue<T, Container, Cmp>::try_pull(lock_guard <mutex> &lk, T &elem) {
  if (super::empty(lk)) {
    if (super::closed(lk)) return queue_op_status::closed;
    return queue_op_status::empty;
  }
  pull(lk, elem);
  return queue_op_status::success;
}

template<class T, class Container, class Cmp>
queue_op_status
sync_priority_queue<T, Container, Cmp>::try_pull(T &elem) {
  lock_guard <mutex> lk(super::mtx_);
  return try_pull(lk, elem);
}

//////////////////////
template<class T, class Container, class Cmp>
queue_op_status
sync_priority_queue<T, Container, Cmp>::wait_pull(unique_lock <mutex> &lk, T &elem) {
  if (super::empty(lk)) {
    if (super::closed(lk)) return queue_op_status::closed;
  }
  bool has_been_closed = super::wait_until_not_empty_or_closed(lk);
  if (has_been_closed) return queue_op_status::closed;
  pull(lk, elem);
  return queue_op_status::success;
}

template<class T, class Container, class Cmp>
queue_op_status sync_priority_queue<T, Container, Cmp>::wait_pull(T &elem) {
  unique_lock <mutex> lk(super::mtx_);
  return wait_pull(lk, elem);
}

//////////////////////

template<class T, class Container, class Cmp>
queue_op_status sync_priority_queue<T, Container, Cmp>::nonblocking_pull(T &elem) {
  unique_lock <mutex> lk(super::mtx_, try_to_lock);
  if (!lk.owns_lock()) return queue_op_status::busy;
  return try_pull(lk, elem);
}

} //end concurrent namespace

using concurrent::sync_priority_queue;

} //end boost namespace
#include <boost/config/abi_suffix.hpp>

#endif
