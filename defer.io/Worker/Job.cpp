//
//  Job.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#include "Job.h"

Job::Job( char *buf ): client(NULL)
{
	memcpy( &header, buf, sizeof(Header) );
	key.append( &(buf[sizeof(header)]), header.keyLen );
	data.append( &(buf[sizeof(header)+header.keyLen]), header.dataLen );
}

#define DATA_MAX_LEN	(1024*1024*4)	//4MB
Job *Job::parse( std::string *buf, ssize_t *buf_off )
{
	char		*data = &((*buf)[*buf_off]);
	Header		*header = (Header *) data;

	if ( buf->length() <= *buf_off )
		return NULL;

	if ( header->dataLen > DATA_MAX_LEN )
		throw std::length_error( "Length of data is too long." );

	size_t target_size = sizeof(Header) + header->keyLen + header->dataLen;
	if ( buf->length()-(*buf_off) < target_size )
		return NULL;

	*buf_off += target_size;

	return new Job( data );
}

void Job::dump()
{
	std::cout << "key: " << key << "\n";
	std::cout << "data: " << data << "\n";
}
