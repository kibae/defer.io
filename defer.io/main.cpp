
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

#include "Key.h"
#include "Document.h"
#include "Job.h"

Json test1()
{
	return Json(2);
}

Json test2()
{
	return Json("world");
}

Json test3()
{
	Json arr( JSON_ARRAY );
	return arr;
}

void test()
{
	Json a( 1 );
	Json b = test1();

	a = b;

	Json str1( "hello" );
	Json str2 = test2();
	Json arr = test3();
	arr.setArrayMember( 0, str1 );
	arr.setArrayMember( 1, str2 );

	Json s1 = arr[0];
	Json s2 = arr[1];

	return;

	DEBUGS( Document::CMD::AUTH )
	DEBUGS( Document::CMD::OBJECT_CMD )
	DEBUGS( Document::CMD::Exists )


	Job *job = new Job( Document::CMD::Set, "test", "[[1,2,3]]" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ArrayCount, "test" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ArrayUnshift, "test", "[4,5,6]" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ArrayCount, "test" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ListPush, "test", "[8, 11,12,13]" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ArrayCount, "test" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ListPush, "test", "[7, 21,22,23,24,25,26,27,28]" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ListPush, "test", "[8, 21,22,23,24,25,26,27,28]" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::ListPush, "test", "[9, 21,22,23,24,25,26,27,28]" );
	job->execute();
	DEBUGS( job->result )

	job = new Job( Document::CMD::NumberIncr, "test[0]" );
	job->execute();
	DEBUGS( job->result )
}

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
	Cache::init( config->numVal( "cache_size", Config::DEF_CACHE_COUNT_LIMIT ) );

	//test();

	short port = config->numVal( "port", 7654 );
	long numCores = config->numVal( "worker_count", sysconf( _SC_NPROCESSORS_ONLN )*1.5 );



	ThreadPool::make( numCores );

	Server	server( port, loop );
	server.start();

	loop.run();

	server.end();
}

