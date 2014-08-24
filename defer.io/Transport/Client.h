//
//  Client.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#ifndef __defer_io__Client__
#define __defer_io__Client__

#include "../include.h"
#include "Server.h"
#include <ev++.h>
#include <sys/socket.h>

class Client : public RWLock
{
	friend class Server;
private:
	int					sock;
	bool				_connected;

	bool				authorized;

	uint32_t			requested;
	uint32_t			working;

	ev::io				rio;
	std::string			rbuf;
	size_t				rbuf_off;

	ev::io				wio;
	std::string			wbuf;
	size_t				wbuf_off;

	void error();
	void disconnect();
	void finalize();

	void read_cb( ev::io &watcher, int revents );
	void write_cb( ev::io &watcher, int revents );
	void writeFinish();

	void wioStart();
public:
	Client( int sock, ev::loop_ref loop );

	void jobFinish();
	bool response( std::string& );
};

#endif /* defined(__defer_io__Client__) */
