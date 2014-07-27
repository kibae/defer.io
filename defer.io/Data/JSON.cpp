//
//  JSON.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#include "JSON.h"

const JSON	JSON::Null;
const JSON	JSON::True(true);
const JSON	JSON::False(false);


JSON::JSON(): reader(), writer()
{
}

JSON::JSON( Json::ValueType t ): Json::Value(t), reader(), writer()
{
}

JSON::JSON( const Json::Value& o ): Json::Value(o), reader(), writer()
{
}

JSON::JSON( Int v ): Json::Value(v), reader(), writer()
{
}

JSON::JSON( UInt v ): Json::Value(v), reader(), writer()
{
}

JSON::JSON( Int64 v ): Json::Value(v), reader(), writer()
{
}

JSON::JSON( UInt64 v ): Json::Value(v), reader(), writer()
{
}

JSON::JSON( double v ): Json::Value(v), reader(), writer()
{
}

JSON::JSON( bool v ): Json::Value(v), reader(), writer()
{
}

JSON::JSON( const std::string &buf ): reader(), writer()
{
	parse( buf );
}


void JSON::parse( const std::string &buf )
{
	if ( buf.length() > 0 && !reader.parse( buf, *this, false ) )
	{
		throw new std::logic_error( reader.getFormattedErrorMessages() );
		return;
	}
}

std::string JSON::toString() const
{
	return writer.write( *this );
}

std::string JSON::toString()
{
	return writer.write( *this );
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
