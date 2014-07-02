//
//  main.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#include "include.h"

#include "Config.h"
#include "ThreadPool.h"
#include "Server.h"

#include "JSON.h"

Config		*config;
int main( int argc, const char * argv[] )
{
	ev::default_loop	loop;

	config = new Config( loop, argc, argv );


	short				port = config->numVal( "port", 7654 );
	long				numCores = config->numVal( "worker_count", sysconf( _SC_NPROCESSORS_ONLN ) );

	ThreadPool::make( numCores );

	Server	server( port, loop );
	server.start();

	loop.run();

	server.end();
}

