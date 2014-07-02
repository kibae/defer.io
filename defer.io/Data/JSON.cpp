//
//  JSON.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#include "JSON.h"

JSON::JSON(): reader(), writer()
{
}

JSON::JSON( const std::string &buf ): reader(), writer()
{
	parse( buf );
}

void JSON::parse( const std::string &buf )
{
	if ( !reader.parse( buf, *this, false ) )
	{
		throw new std::logic_error( reader.getFormattedErrorMessages() );
		return;
	}
}

JSON *JSON::fromString( const std::string &buf )
{
	JSON *obj = NULL;
	try
	{
		obj = new JSON( buf );
	}
	catch ( const std::logic_error& e )
	{
		DEBUGS(e.what())
	}

	return obj;
}
