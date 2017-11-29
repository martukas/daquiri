#pragma once

//#define USE_BOOST_MUTEX 1

#ifdef USE_BOOST_MUTEX

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#define MUTEX_LOCK_TIMOUT boost::chrono::microseconds(100)
#define WAIT_MUTEX_LOCK boost::this_thread::sleep_for(MUTEX_LOCK_TIMOUT)
#define DEFER boost::defer_lock

using mutex = boost::mutex;
using mutex_st = boost::shared_mutex;
using shared_lock_st = boost::shared_lock<mutex_st>;
using unique_lock_st = boost::unique_lock<mutex_st>;
using unique_lock = boost::unique_lock<mutex>;
using condition_variable = boost::condition_variable;

#else

#include <shared_mutex>
#include <thread>

#define MUTEX_LOCK_TIMOUT std::chrono::microseconds(100)
#define WAIT_MUTEX_LOCK std::this_thread::sleep_for(MUTEX_LOCK_TIMOUT)
#define DEFER std::defer_lock

using mutex = std::mutex;
using mutex_st = std::shared_timed_mutex;
using shared_lock_st = std::shared_lock<mutex_st>;
using unique_lock_st = std::unique_lock<mutex_st>;
using unique_lock = std::unique_lock<mutex>;
using condition_variable = std::condition_variable;

#endif

#define UNIQUE_LOCK unique_lock ulock(mutex_, DEFER);
#define UNIQUE_LOCK_ST unique_lock_st ulock(mutex_, DEFER);
#define SHARED_LOCK_ST shared_lock_st lock(mutex_);
#define EVENTUALLY_LOCK while (!ulock.try_lock()) WAIT_MUTEX_LOCK;
#define UNIQUE_LOCK_EVENTUALLY UNIQUE_LOCK EVENTUALLY_LOCK
#define UNIQUE_LOCK_EVENTUALLY_ST UNIQUE_LOCK_ST EVENTUALLY_LOCK