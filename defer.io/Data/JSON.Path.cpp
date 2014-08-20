//
//  JSON.Path.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 20..
//
//

#include "JSON.h"

Json::Path::Path(): arguments(), _isset(false), parent(), element()
{
}

Json::Path::Path( std::string &path ): Path()
{
	parse( path );
}

void Json::Path::parse( std::string &path )
{
	_isset = true;

	std::string str( path );
	size_t off = str.find_first_of( ".[" );
	while ( off != std::string::npos && str.length() > off+1 )
	{
		if ( str[off] == '.' )
		{
			//obj
			size_t off2 = str.find_first_of( ".[", off+1 );
			if ( off2 == std::string::npos )
				arguments.push_back( Argument(str.substr(off+1)) );
			else if ( off2 <= off+1 )
				break;
			else
				arguments.push_back( Argument(str.substr(off+1, off2-off-1)) );
			off = off2;
		}
		else
		{
			//array
			int32_t idx = atoi( &(str[off+1]) );
			arguments.push_back( Argument(idx) );
			off = str.find_first_of( "]", off );
			if ( off != std::string::npos )
				off++;
		}
	}
	DEBUGS( this->str() );
}

bool Json::Path::find( Json &root )
{
	if ( arguments.size() <= 0 )
		return false;

	Json node = root;
	unsigned int i = 0;
	for ( ; i < arguments.size(); i++ )
	{
		if ( i == arguments.size()-1 )
			parent = node;

		if ( arguments[i].kind == Object )
		{
			//DEBUGS(arguments[i].name)
			if ( node.type() == JSON_OBJECT && node.hasMember(arguments[i].name) )
				node = node[arguments[i].name];
			else
				break;
		}
		else
		{
			//DEBUGS(arguments[i].idx)
			if ( node.type() == JSON_ARRAY && node.size() > arguments[i].idx )
				node = node[arguments[i].idx];
			else
				break;
		}
	}

	if ( i == arguments.size() && !!parent )
	{
		element = node;
		return true;
	}
	else if ( i == arguments.size()-1 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string Json::Path::str()
{
	std::string buf;

	for ( unsigned int i=0; i < arguments.size(); i++ )
		buf.append( arguments[i].toString() );

	return buf;
}

bool Json::Path::isset()
{
	return _isset;
}

Json::Path::Argument &Json::Path::getLastArgument()
{
	return arguments.back();
}

Json::Path::Argument::Argument( uint32_t _idx )
{
	kind = Array;
	idx = _idx;
}

Json::Path::Argument::Argument( std::string _name )
{
	kind = Object;
	name.assign( _name );
}

std::string Json::Path::Argument::toString()
{
	std::stringstream buf;
	if ( kind == Object )
	{
		buf << "." << name;
	}
	else
	{
		buf << "[" << idx << "]";
	}
	return buf.str();
}

