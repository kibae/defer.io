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
#include "Client.h"
#include <queue>

#include <ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>

class Client;
class Server
{
private:
	ev::io						io;

	int							port;
	int							sock;
	static std::queue<Client *>	sendClients;
public:
	ev::loop_ref				loop;
	static pthread_mutex_t		_lock;
	static ev::async			aio;

	Server( short port, ev::loop_ref loop );

	void accept_cb( ev::io &watcher, int revents );
	int start();
	void end();

	static inline void lock() {
		pthread_mutex_lock( &_lock );
	}

	static inline void unlock() {
		pthread_mutex_unlock( &_lock );
	}

	static void sendClient( Client *client );
	static void aio_cb( ev::async &watcher, int revents );
};

#endif /* defined(__defer_io__Server__) */
