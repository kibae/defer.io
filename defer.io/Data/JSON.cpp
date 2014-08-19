//
//  JSON.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#include "JSON.h"

ssize_t JSONMem::used = 0;
pthread_mutex_t	JSONMem::_lock = PTHREAD_MUTEX_INITIALIZER;
const JSONVal JSON::Null;

void JSONMem::retain( ssize_t sz )
{
	lock();
	used += sz;
	unlock();
}

void JSONMem::release( ssize_t sz )
{
	lock();
	used -= sz;
	unlock();
}

ssize_t JSONMem::size()
{
	return used;
}

JSONDoc::JSONDoc(): allocator(2*1024), rapidjson::Document( &allocator )
{
}

JSONDoc::~JSONDoc()
{
	JSONMem::release( allocator.Capacity() );
	allocator.Clear();
}

bool JSON::parse( const std::string &buf, JSONDoc &doc )
{
	return doc.Parse( buf.c_str() ).HasParseError();
}

std::string JSON::stringify( JSONVal &v )
{
	rapidjson::StringBuffer buf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
	v.Accept( writer );
	return buf.GetString();
}
