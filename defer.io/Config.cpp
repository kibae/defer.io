//
//  Config.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#include "Config.h"

Config *Config::lastInstance = NULL;
int Config::argc = 0;
const char **Config::argv = NULL;

Config::Config( ev::loop_ref _loop, int argc, const char *argv[] ): sio_int(), sio_usr1(), loop(_loop)
{
	Config::argc = argc;
	Config::argv = argv;
	
	sio_int.set<Config, &Config::signal_int>(this);
	sio_int.set( loop );
	sio_int.start( SIGINT );

	sio_usr1.set<Config, &Config::signal_usr1>(this);
	sio_usr1.set( loop );
	sio_usr1.start( SIGUSR1 );

	//TODO: parse
	values.insert( valueRow("port", "7654") );
	values.insert( valueRow("datadir", "/tmp/deferio") );

	struct rlimit	limit;
	if ( getrlimit( RLIMIT_NOFILE, &limit ) != 0 )
	{
		std::stringstream e;
		e << "getrlimit() failed with errno(" << errno << ") " << strerror(errno) << "\n";
		throw std::logic_error( e.str() );
	}

	limit.rlim_cur = 1024;
	if ( setrlimit( RLIMIT_NOFILE, &limit ) != 0 )
	{
		std::stringstream e;
		e << "setrlimit() failed with errno(" << errno << ") " << strerror(errno) << "\n";
		throw std::logic_error( e.str() );
	}

	Config::lastInstance = this;
}

Config::~Config()
{
	sio_int.stop();
	sio_usr1.stop();
}

long Config::numVal( const std::string &nm )
{
	return numVal( nm, 0 );
}

long Config::numVal( const std::string &nm, long defVal )
{
	std::string		val = values[nm];

	if ( val != "" )
	{
		return ::atol( val.c_str() );
	}

	return defVal;
}

long Config::numVal( const char *nm )
{
	return numVal( nm, 0 );
}

long Config::numVal( const char *nm, long defVal )
{
	std::string _nm( nm );
	return numVal( _nm, defVal );
}


void Config::signal_int( ev::sig &signal, int revents )
{
	LLOG
	loop.break_loop();
}

void Config::signal_usr1( ev::sig &signal, int revents )
{
	LLOG
	
	//reload config
	reload();
}

void Config::reload()
{
	if ( lastInstance != NULL )
	{
		Config *prev = lastInstance;
		Config *config = NULL;
		try
		{
			config = new Config( lastInstance->loop, Config::argc, Config::argv );
		}
		catch( const std::length_error& le )
		{
			DEBUGS( le.what() )
		}
	}
}

