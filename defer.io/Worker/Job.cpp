//
//  Job.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#include "Job.h"
#include "Server.h"
#include "Document.h"
#include "System.h"

Job::Job( const char *buf ): header(), key(), path("."), data(), client(NULL), resHeader(), result()
{
	memcpy( &header, buf, sizeof(Header) );
	if ( header.cmd > Document::CMD::OBJECT_CMD )
	{
		if ( header.keyLen > 0 )
		{
			std::string k( &(buf[sizeof(header)]), header.keyLen );
			initKey( k );
		}

		if ( header.dataLen > 0 )
			data.append( &(buf[sizeof(header)+header.keyLen]), header.dataLen );
	}
	else
	{
		//connect or operation commands
	}
}

Job::Job( uint8_t cmd, std::string _key, std::string _data ): header(), key(), path("."), data(_data), client(NULL), resHeader(), result()
{
	header.cmd = cmd;
	header.keyLen = (uint16_t) _key.length();
	header.dataLen = (uint32_t) _data.length();

	if ( header.keyLen > 0 )
		initKey( _key );
}

Job::Job( uint8_t cmd, std::string _key ): header(), key(), path("."), data(), client(NULL), result()
{
	header.cmd = cmd;
	header.keyLen = (uint16_t) _key.length();

	if ( header.keyLen > 0 )
		initKey( _key );
}

void Job::initKey( std::string &k )
{
	ssize_t off = k.find_first_of( ".[" );
	if ( off == std::string::npos )
		key.set( k );
	else
	{
		if ( off == 0 )
		{
			//invalid key
			throw std::length_error( "Invalid key." );
		}

		key.set( k.c_str(), off );
		if ( k[off] == '.' )
			path.append( &(k.c_str()[off+1]), k.length()-off-1 );	//object
		else
			path.append( &(k.c_str()[off]), k.length()-off );		//array
	}
}

#define KEY_MAX_LEN		(1024*16)	//16KB
#define DATA_MAX_LEN	(1024*1024*16)	//16MB
Job *Job::parse( const std::string &buf, size_t *buf_off )
{
	const char	*data = &((buf)[*buf_off]);
	Header		*header = (Header *) data;

	if ( buf.length() <= *buf_off )
		return NULL;

	if ( header->keyLen > KEY_MAX_LEN )
		throw std::length_error( "Length of key is too long." );
	else if ( header->dataLen > DATA_MAX_LEN )
		throw std::length_error( "Length of data is too long." );

	size_t target_size = sizeof(Header) + header->keyLen + header->dataLen;
	if ( buf.length()-(*buf_off) < target_size )
		return NULL;

	*buf_off += target_size;

	return new Job( data );
}

void Job::execute()
{
	std::string buf;
	resHeader.apikey = header.apikey;
	resHeader.status = 404;

	Document::CMD cmd = (Document::CMD) header.cmd;
	if ( cmd > Document::CMD::OBJECT_CMD )
	{
		Document *doc = key.str().length() > 0 ? Document::getOrCreate( key ) : NULL;
		if ( doc == NULL )
		{
			//document error
			JSONVal error( "Document not found." );
			buf.append( JSON::stringify(error) );

			resHeader.status = 500;
			resHeader.dataLen = (uint32_t) buf.length();

			result.append( (char *) &resHeader, sizeof(ResponseHeader) );
			result.append( buf );
			return;
		}

		try
		{
			JSONDoc res, arg;
			JSON::parse( data, arg );
			resHeader.status = doc->execute( path, cmd, arg, res );
			buf.append( JSON::stringify( res ) );
		}
		catch ( const Document::Error& le )
		{
			//DEBUGS(le.status())
			DEBUGS(le.what())
			resHeader.status = le.status();
			JSONVal error( le.what(), doc->allocator() );
			buf.append( JSON::stringify(error) );
		}
		doc->unlock();
	}
	else if ( cmd > Document::CMD::SYSTEM_CMD )
	{
		//system command
		try
		{
			JSONDoc res;
			resHeader.status = System::execute( this, res );
			buf.append( JSON::stringify( res ) );
		}
		catch ( const Document::Error& le )
		{
			//DEBUGS(le.status())
			DEBUGS(le.what())
			resHeader.status = le.status();
			std::string tmp(le.what());
			JSONVal error( tmp.c_str(), tmp.length() );
			buf.append( JSON::stringify(error) );
		}
	}

	resHeader.dataLen = (uint32_t) buf.length();

	result.append( (char *) &resHeader, sizeof(ResponseHeader) );
	result.append( buf );
}

void Job::finish( Job *job )
{
	if ( job->client != NULL )
	{
		Server::jobFinish( job );
	}
	else
	{
		job->dump();
	}
}

void Job::dump()
{
	std::cout << "Key: " << key.str() << "\n";
	std::cout << "Path: " << path << "\n";
	std::cout << "Cmd: " << header.cmd << "\n";
	std::cout << "Data: " << data << "\n";
	std::cout << "Result: " << result << "\n";
}
