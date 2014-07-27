//
//  Config.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#ifndef __defer_io__Config__
#define __defer_io__Config__

#include "include.h"
#include <unordered_map>

#include <ev++.h>

class Config
{
private:
	const static uint32_t	LIMIT_NOFILE = (1024*4);


	static Config			*lastInstance;
	
	ev::sig					sio_int;
	ev::sig					sio_usr1;
	ev::loop_ref			loop;
	
	static int				argc;
	static const char		**argv;
public:
	//const
	const static long		DEF_CACHE_COUNT_LIMIT = (1024);


	Config( ev::loop_ref loop, int argc, const char *argv[] );
	~Config();

	std::unordered_map<std::string, std::string> values;
	typedef std::pair<std::string, std::string> valueRow;

	void signal_usr1( ev::sig &signal, int revents );
	void signal_int( ev::sig &signal, int revents );

	long numVal( const std::string &nm );
	long numVal( const std::string &nm, long defVal );
	long numVal( const char *nm );
	long numVal( const char *nm, long defVal );

	static void reload();
};

#endif /* defined(__defer_io__Config__) */
