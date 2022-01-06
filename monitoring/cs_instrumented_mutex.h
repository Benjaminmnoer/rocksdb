//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include "monitoring/statistics.h"
#include "port/port.h"
#include "rocksdb/statistics.h"
#include "rocksdb/system_clock.h"
#include "rocksdb/thread_status.h"
#include "util/stop_watch.h"

namespace ROCKSDB_NAMESPACE {
class CSInstrumentedCondVar;

// A wrapper class for port::Mutex that provides additional layer
// for collecting stats and instrumentation.
class CSInstrumentedMutex {
 public:
  explicit CSInstrumentedMutex(bool adaptive = false)
      : mutex_(adaptive), stats_(nullptr), clock_(nullptr), stats_code_(0) {}

  explicit CSInstrumentedMutex(SystemClock* clock, bool adaptive = false)
      : mutex_(adaptive), stats_(nullptr), clock_(clock), stats_code_(0) {}

  CSInstrumentedMutex(Statistics* stats, SystemClock* clock, int stats_code,
                    bool adaptive = false)
      : mutex_(adaptive),
        stats_(stats),
        clock_(clock),
        stats_code_(stats_code) {}

  void Lock();

  void Unlock() {
    mutex_.Unlock();
  }

  void AssertHeld() {
    mutex_.AssertHeld();
  }

 private:
  void LockInternal();
  friend class CSInstrumentedCondVar;
  port::Mutex mutex_;
  Statistics* stats_;
  SystemClock* clock_;
  int stats_code_;
};

// RAII wrapper for CSInstrumentedMutex
class CSInstrumentedMutexLock {
 public:
  explicit CSInstrumentedMutexLock(CSInstrumentedMutex* mutex) : mutex_(mutex) {
    mutex_->Lock();
  }

  ~CSInstrumentedMutexLock() {
    mutex_->Unlock();
  }

 private:
  CSInstrumentedMutex* const mutex_;
  CSInstrumentedMutexLock(const CSInstrumentedMutexLock&) = delete;
  void operator=(const CSInstrumentedMutexLock&) = delete;
};

// RAII wrapper for temporary releasing CSInstrumentedMutex inside
// CSInstrumentedMutexLock
class CSInstrumentedMutexUnlock {
 public:
  explicit CSInstrumentedMutexUnlock(CSInstrumentedMutex* mutex) : mutex_(mutex) {
    mutex_->Unlock();
  }

  ~CSInstrumentedMutexUnlock() { mutex_->Lock(); }

 private:
  CSInstrumentedMutex* const mutex_;
  CSInstrumentedMutexUnlock(const CSInstrumentedMutexUnlock&) = delete;
  void operator=(const CSInstrumentedMutexUnlock&) = delete;
};

class CSInstrumentedCondVar {
 public:
  explicit CSInstrumentedCondVar(CSInstrumentedMutex* instrumented_mutex)
      : cond_(&(instrumented_mutex->mutex_)),
        stats_(instrumented_mutex->stats_),
        clock_(instrumented_mutex->clock_),
        stats_code_(instrumented_mutex->stats_code_) {}

  void Wait();

  bool TimedWait(uint64_t abs_time_us);

  void Signal() {
    cond_.Signal();
  }

  void SignalAll() {
    cond_.SignalAll();
  }

 private:
  void WaitInternal();
  bool TimedWaitInternal(uint64_t abs_time_us);
  port::CondVar cond_;
  Statistics* stats_;
  SystemClock* clock_;
  int stats_code_;
};

}  // namespace ROCKSDB_NAMESPACE
