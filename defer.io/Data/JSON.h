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
	Json::Reader				reader;
	mutable Json::FastWriter	writer;
public:
	const static JSON	Null;
	const static JSON	True;
	const static JSON	False;

	JSON();
	JSON( Json::ValueType );
	JSON( const Json::Value& );

	JSON( Int );
	JSON( UInt );
	JSON( Int64 );
	JSON( UInt64 );
	JSON( double );
	JSON( bool );
	JSON( const std::string &buf );

	void parse( const std::string &buf );	//throw
	std::string toString() const;
	std::string toString();

	static JSON *fromString( const std::string &buf );
};



#endif /* defined(__defer_io__JSON__) */
