/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef SHARE_GC_Z_ZLOCK_INLINE_HPP
#define SHARE_GC_Z_ZLOCK_INLINE_HPP

#include "gc/z/zLock.hpp"
#include "runtime/atomic.hpp"
#include "runtime/thread.hpp"
#include "utilities/debug.hpp"
#include "runtime/os.inline.hpp"
#include <time.h>

inline ZLock::ZLock() {
  pthread_mutex_init(&_lock, NULL);
}

inline ZLock::~ZLock() {
  pthread_mutex_destroy(&_lock);
}

inline void ZLock::lock() {
  pthread_mutex_lock(&_lock);
}

inline bool ZLock::try_lock() {
  return pthread_mutex_trylock(&_lock) == 0;
}

inline void ZLock::unlock() {
  pthread_mutex_unlock(&_lock);
}

inline ZReentrantLock::ZReentrantLock() :
    _lock(),
    _owner(NULL),
    _count(0) {}

inline void ZReentrantLock::lock() {
  Thread* const thread = Thread::current();
  Thread* const owner = Atomic::load(&_owner);

  if (owner != thread) {
    _lock.lock();
    Atomic::store(thread, &_owner);
  }

  _count++;
}

inline void ZReentrantLock::unlock() {
  assert(is_owned(), "Invalid owner");
  assert(_count > 0, "Invalid count");

  _count--;

  if (_count == 0) {
    Atomic::store((Thread*)NULL, &_owner);
    _lock.unlock();
  }
}

inline bool ZReentrantLock::is_owned() const {
  Thread* const thread = Thread::current();
  Thread* const owner = Atomic::load(&_owner);
  return owner == thread;
}

inline ZConditionLock::ZConditionLock() {
  pthread_cond_init(&_cond, NULL);
  pthread_mutex_init(&_mutex, NULL);
}

inline ZConditionLock::~ZConditionLock() {
  pthread_cond_destroy(&_cond);
  pthread_mutex_destroy(&_mutex);
}

inline void ZConditionLock::lock() {
  pthread_mutex_lock(&_mutex);
}

inline bool ZConditionLock::try_lock() {
  return pthread_mutex_trylock(&_mutex) == 0;
}

inline void ZConditionLock::unlock() {
  pthread_mutex_unlock(&_mutex);
}

inline bool ZConditionLock::wait(uint64_t millis) {
  if (millis > 0) {
    jlong timeout = millis * (NANOUNITS / MILLIUNITS);

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    // Calculate a new absolute time that is "timeout" nanoseconds from "now".
    struct timespec abstime;
    jlong seconds = timeout / NANOUNITS;
    abstime.tv_sec = now.tv_sec + seconds;

    timeout %= NANOUNITS; // remaining nanos
    long nanos = now.tv_nsec + timeout;
    if (nanos >= NANOUNITS) { // overflow
      abstime.tv_sec += 1;
      nanos -= NANOUNITS;
    }
    abstime.tv_nsec = nanos;

    int status = pthread_cond_timedwait(&_cond, &_mutex, &abstime);
    return status == 0;
  } else {
    pthread_cond_wait(&_cond, &_mutex);
    return true;
  }
}

inline void ZConditionLock::notify() {
  pthread_cond_signal(&_cond);
}

inline void ZConditionLock::notify_all() {
  pthread_cond_broadcast(&_cond);
}

template <typename T>
inline ZLocker<T>::ZLocker(T* lock) :
    _lock(lock) {
  _lock->lock();
}

template <typename T>
inline ZLocker<T>::~ZLocker() {
  _lock->unlock();
}

#endif // SHARE_GC_Z_ZLOCK_INLINE_HPP
