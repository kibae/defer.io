//
//  Thread.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#ifndef __defer_io__Thread__
#define __defer_io__Thread__

#include "../include.h"

class Thread;

#include "ThreadPool.h"

class Thread
{
private:
	static bool				_destroy;

	pthread_t				thread;
	int						status;
	int						seq;
public:
	static void *virtual_proc( void *arg );

	Thread( int _seq );

	void *proc();
};

#endif /* defined(__defer_io__Thread__) */
