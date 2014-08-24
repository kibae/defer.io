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

Thread::Thread( const int _seq ): status(0), seq(_seq), thread(&Thread::proc, this)
{
}

void Thread::proc() const
{
	std::cout << seq << " thread start " << this << "\n";

	Job *job = NULL;
	while ( !ThreadPool::terminate )
	{
		job = ThreadPool::shift();
		if ( job != NULL )
		{
			job->execute();
			job->finish();
			job = NULL;
		}
	}

	std::cout << seq << " thread end\n";
	ThreadPool::finish();
}
