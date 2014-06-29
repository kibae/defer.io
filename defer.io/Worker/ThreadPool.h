//
//  ThreadPool.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#ifndef __defer_io__ThreadPool__
#define __defer_io__ThreadPool__

#include "../include.h"
#include <queue>

#include "Thread.h"
#include "Job.h"

class ThreadPool
{
private:
	static Thread				**pool;
	static long					poolSize;

	static pthread_cond_t		_cond;
	static pthread_mutex_t		_lock;

	static std::queue<Job *>	jobs;
public:
	static bool terminate;
	static int make( long poolSize );

	static inline void lock() {
		pthread_mutex_lock( &_lock );
	}

	static inline void unlock() {
		pthread_mutex_unlock( &_lock );
	}

	static void push( Job *job );
	static Job *shift();

	static void finish();
};

#endif /* defined(__defer_io__ThreadPool__) */
