
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
#include "DB.h"
#include "Cache.h"

Config		*config;
int main( int argc, const char * argv[] )
{
	ev::default_loop	loop;

	try
	{
		config = new Config( loop, argc, argv );
	}
	catch ( const std::logic_error &le )
	{
		DEBUGS( le.what() )
		return -1;
	}

	//Config setting
	DB::init( config->values["datadir"], config->numVal( "sync_request_interval", Config::DEF_SYNC_REQ_INTERVAL ) );

	Cache::init( config->numVal( "cache_memory_limit", Config::DEF_CACHE_MEMORY_LIMIT ), config->numVal( "cache_count_limit", Config::DEF_CACHE_COUNT_LIMIT ) );

	short port = config->numVal( "port", 7654 );
	long numCores = config->numVal( "worker_count", sysconf( _SC_NPROCESSORS_ONLN )*1.5 );


	ThreadPool::make( numCores );

	Server	server( port, loop );
	server.start();

	loop.run();

	server.end();
}

