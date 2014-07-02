//
//  JSON.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#ifndef __defer_io__JSON__
#define __defer_io__JSON__

#include "../include.h"

#include <json/json.h>

class JSON : public Json::Value
{
private:
	Json::Reader		reader;
	Json::FastWriter	writer;
public:
	JSON();
	JSON( const std::string &buf );

	void parse( const std::string &buf );	//throw

	static JSON *fromString( const std::string &buf );
};



#endif /* defined(__defer_io__JSON__) */
