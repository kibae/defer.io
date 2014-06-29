//
//  Server.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#ifndef __defer_io__Server__
#define __defer_io__Server__

#include "../include.h"
#include <queue>

#include <ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Job.h"

class Server
{
private:
	ev::io					io;
	ev::sig					sio;

	int						port;
	int						sock;
	static std::queue<Job *>	finishedJobs;
public:
	static ev::default_loop	loop;
	static pthread_mutex_t	_lock;
	static ev::async		aio;

	Server( short port );

	void accept_cb( ev::io &watcher, int revents );
	int start();
	void end();

	static inline void lock() {
		pthread_mutex_lock( &_lock );
	}

	static inline void unlock() {
		pthread_mutex_unlock( &_lock );
	}

	static void signal_cb( ev::sig &signal, int revents );

	static void jobFinish( Job *job );
	static void aio_cb( ev::async &watcher, int revents );
};

#endif /* defined(__defer_io__Server__) */
