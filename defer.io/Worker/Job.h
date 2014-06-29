//
//  Job.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#ifndef __defer_io__Job__
#define __defer_io__Job__

#include "../include.h"

class Job;
#include "Client.h"

class Job
{
public:
	Job( char *buf );

	typedef enum OPER_TYPE {
		OPER_GET = 'G',
		OPER_SET = 'S',
		OPER_REPLACE = 'R',
		OPER_DELETE = 'D',
	} OPER_TYPE;

	struct Header
	{
		char			oper;
		uint32_t		apikey;
		uint8_t			keyLen;
		uint32_t		dataLen;
	} __attribute__((packed));

	Header				header;
	std::string			key;
	std::string			data;

	Client				*client;

	std::string			result;

	static Job *parse( std::string *buf, ssize_t *buf_off );

	void dump();
};

#endif /* defined(__defer_io__Job__) */
