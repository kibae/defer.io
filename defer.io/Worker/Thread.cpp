//
//  Thread.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#include "Thread.h"
#include "Job.h"
#include "Server.h"

Thread::Thread( const int _seq ):thread(), status(0), seq(_seq)
{
	if ( pthread_create( &thread, NULL, virtual_proc, this ) != 0 )
	{
		throw std::runtime_error( strerror(errno) );
		return;
	}

	pthread_detach( thread );
}

void *Thread::virtual_proc( void *arg )
{
	Thread *thread = (Thread *) arg;
	thread->proc();

	delete thread;

	return NULL;
}

void *Thread::proc()
{
	std::cout << seq << " thread start " << thread << " " << this << "\n";

	Job *job = NULL;
	while ( !ThreadPool::terminate )
	{
		job = ThreadPool::shift();
		if ( job != NULL )
		{
			//ThreadPool::lock(); LLOG job->dump(); ThreadPool::unlock();

			//TODO:process

			//job->result.append(job->data);

			Server::jobFinish( job );
			job = NULL;
		}
	}

	std::cout << seq << " thread end\n";
	ThreadPool::finish();

	return NULL;
}
