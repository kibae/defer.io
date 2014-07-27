
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


void test()
{
	//return;

	Document doc( Key("test"), "[{\"a\":[1,2,3]}]" );
	DEBUGS( doc.out() );
	DEBUGS( doc.execute( ".", Document::Set, std::string("[{\"a\":[1,2,3,4,5,6]}]") ).toString() );
	DEBUGS( doc.out() );
	doc.execute( ".", Document::ObjectSet, std::string("[\"b\", [4,5,6], \"d\", 10, null, 20, 20.0, 40.0, \"100\", 20.0, \"\", 100]") );
	DEBUGS( doc.out() );
	doc.execute( ".", Document::ObjectDel, std::string("[\"b\"]") );
	DEBUGS( doc.out() );
	doc.execute( ".a", Document::ArrayPush, std::string("[4,5,6,7,8,9]") );
	DEBUGS( doc.out() );
	doc.execute( ".c", Document::ArrayPush, std::string("[14,5,6,7,8,9]") );
	DEBUGS( doc.out() );
	doc.execute( ".a", Document::ArrayCut, std::string("[13]") );
	DEBUGS( doc.out() );
	doc.execute( ".a", Document::ArrayCut, std::string("[10]") );
	DEBUGS( doc.out() );
	doc.execute( ".a", Document::ArrayCut, std::string("[100, -5]") );
	DEBUGS( doc.out() );
	doc.execute( ".b", Document::NumberIncr, std::string("") );
	DEBUGS( doc.out() );
	doc.execute( ".c[16]", Document::NumberIncr, std::string("[100]") );
	DEBUGS( doc.out() );
	doc.execute( ".c[14]", Document::NumberIncr, std::string("") );
	DEBUGS( doc.out() );
	doc.execute( ".c[13]", Document::Set, std::string("[\"aaa\"]") );
	DEBUGS( doc.out() );

	DEBUGS( doc.execute( ".", Document::ObjectKeys, std::string("[]") ).toString() );
	DEBUGS( doc.execute( ".c", Document::ArrayCount, std::string("[]") ).toString() );

	DEBUGS( doc.execute( ".c", Document::GetSet, std::string("[\"ccccc\"]") ).toString() );
	DEBUGS( doc.out() );

	doc.execute( ".a", Document::ArraySplice, std::string("[2, 2, 1,2,3]") ).toString();
	DEBUGS( doc.out() );

	exit(0);
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
	DB::init( config->values["datadir"] );
	Cache::setCountLimit( config->numVal( "cache_size", Config::DEF_CACHE_COUNT_LIMIT ) );

	test();

	short port = config->numVal( "port", 7654 );
	long numCores = config->numVal( "worker_count", sysconf( _SC_NPROCESSORS_ONLN )*1.5 );



	ThreadPool::make( numCores );

	Server	server( port, loop );
	server.start();

	loop.run();

	server.end();
}

