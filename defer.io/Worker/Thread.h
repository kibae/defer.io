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
#include <thread>

class Thread;

#include "ThreadPool.h"

class Thread
{
private:
	static bool				_destroy;

	int						status;
	int						seq;
	std::thread				thread;
public:
	Thread( const int seq );

	void proc() const;
};

#endif /* defined(__defer_io__Thread__) */
