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

Job::Job( const char *buf ): header(), key(), path(), data(), client(NULL), resHeader(), result()
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

Job::Job( uint8_t cmd, std::string _key, std::string _data ): header(), key(), path(), data(_data), client(NULL), resHeader(), result()
{
	header.cmd = cmd;
	header.keyLen = (uint16_t) _key.length();
	header.dataLen = (uint32_t) _data.length();

	if ( header.keyLen > 0 )
		initKey( _key );
}

Job::Job( uint8_t cmd, std::string _key ): header(), key(), path(), data(), client(NULL), result()
{
	header.cmd = cmd;
	header.keyLen = (uint16_t) _key.length();

	if ( header.keyLen > 0 )
		initKey( _key );
}

void Job::initKey( std::string &k )
{
	size_t off = k.find_first_of( ".[" );
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
		std::string p( &(k.c_str()[off]), k.length()-off );
		path.parse( p );
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
			Json error( "Document not found." );
			buf.append( error.stringify() );

			resHeader.status = 500;
			resHeader.dataLen = (uint32_t) buf.length();

			result.append( (char *) &resHeader, sizeof(ResponseHeader) );
			result.append( buf );
			return;
		}

		try
		{
			Json arg;
			arg.parse( data );
			resHeader.status = 200;
			Json res = doc->execute( path, cmd, arg );
			buf.append( res.stringify() );
		}
		catch ( const Json::Error& le )
		{
			//DEBUGS(le.status())
			DEBUGS(le.what())
			resHeader.status = le.status();
			Json error( le.what() );
			buf.append( error.stringify() );
		}
		doc->unlock();
	}
	else if ( cmd > Document::CMD::SYSTEM_CMD )
	{
		//system command
		try
		{
			resHeader.status = 200;
			Json res = System::execute( this );
			buf.append( res.stringify() );
		}
		catch ( const Json::Error& le )
		{
			//DEBUGS(le.status())
			DEBUGS(le.what())
			resHeader.status = le.status();
			std::string tmp(le.what());
			Json error( tmp.c_str() );
			buf.append( error.stringify() );
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
	std::cout << "Path: " << path.str() << "\n";
	std::cout << "Cmd: " << header.cmd << "\n";
	std::cout << "Data: " << data << "\n";
	std::cout << "Result: " << result << "\n";
}
