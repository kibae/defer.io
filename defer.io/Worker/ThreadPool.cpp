//
//  ThreadPool.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#include "ThreadPool.h"

bool ThreadPool::terminate = false;
Thread **ThreadPool::pool = NULL;
long ThreadPool::poolSize = 0;
std::queue<Job *> ThreadPool::jobs;

pthread_cond_t ThreadPool::_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t ThreadPool::_lock = PTHREAD_MUTEX_INITIALIZER;

int ThreadPool::make( long poolSize )
{
	pool = (Thread **) malloc( sizeof(Thread *) * poolSize );
	ThreadPool::poolSize = poolSize;
	for ( int i=0; i < poolSize; i++ )
	{
		pool[i] = new Thread( i );
		usleep( 1000 );
	}

	return 0;
}

void ThreadPool::push( Job *job )
{
	lock();

	jobs.push( job );
	pthread_cond_signal( &_cond );

	unlock();
}

Job *ThreadPool::shift()
{
	Job		*job = NULL;

	lock();
	if ( jobs.size() > 0 )
	{
		job = jobs.front();
		jobs.pop();
	}
	else
	{
		pthread_cond_wait( &_cond, &_lock );
		if ( jobs.size() > 0 )
		{
			job = jobs.front();
			jobs.pop();
		}
	}
	unlock();

	return job;
}

void ThreadPool::finish()
{
	lock();
	poolSize--;
	unlock();
}
