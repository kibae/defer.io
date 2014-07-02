//
//  Job.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#include "Job.h"

Job::Job( const char *buf ): header({0,}), key(), data(), client(NULL), result()
{
	memcpy( &header, buf, sizeof(Header) );
	if ( header.keyLen > 0 )
		key.append( &(buf[sizeof(header)]), header.keyLen );
	if ( header.dataLen > 0 )
		data.append( &(buf[sizeof(header)+header.keyLen]), header.dataLen );
}

#define KEY_MAX_LEN	(1024*16)	//16KB
#define DATA_MAX_LEN	(1024*1024*16)	//16MB
Job *Job::parse( const std::string &buf, ssize_t *buf_off )
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

void Job::dump()
{
	std::cout << "key: " << key << "\n";
	std::cout << "data: " << data << "\n";
}
