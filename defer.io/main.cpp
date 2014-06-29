//
//  main.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#include <iostream>
#include <ev++.h>
#include <unistd.h>

#include "ThreadPool.h"
#include "Server.h"

int main( int argc, const char * argv[] )
{
	short	port = 7654;
	long	numCores = sysconf( _SC_NPROCESSORS_ONLN );

	ThreadPool::make( numCores );

	Server	server( port );
	//main loop
	server.start();

	server.end();
}

