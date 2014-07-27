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
#include "Key.h"

class Job
{
public:
	Job( const char *buf );

	typedef enum OPER_TYPE {
		OPER_EXECUTE = 'E',		//response object
		OPER_THROW = 'P',		//response only status/error
		OPER_BATCH = 'B',		//response only status/error
	} OPER_TYPE;

	struct Header
	{
		char			oper;
		uint32_t		apikey;
		uint16_t		keyLen;		//max: 16KB
		uint32_t		dataLen;	//max: 16MB
	} __attribute__((packed));

	Header				header;
	Key					key;
	std::string			data;

	Client				*client;

	std::string			result;

	static Job *parse( const std::string &buf, size_t *buf_off );

	void dump();
};

#endif /* defined(__defer_io__Job__) */
