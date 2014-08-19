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
	void initKey( std::string& );
public:
	Job( const char *buf );

	Job( uint8_t cmd, std::string key, std::string data );
	Job( uint8_t cmd, std::string key );

	typedef enum REQ_OPTION {
		OPT_NO_RESULTSET = 'R',		//response only status/error
	} REQ_OPTION;

	struct Header
	{
		uint8_t			cmd;		//Document::CMD
		uint8_t			opt;		//REQ_OPTION
		uint32_t		apikey;
		uint16_t		keyLen;		//max: 16KB
		uint32_t		dataLen;	//max: 16MB

		Header(): cmd(0), opt(0), apikey(0), keyLen(0), dataLen(0) {}
	} __attribute__((packed));

	struct ResponseHeader
	{
		uint16_t		status;
		uint32_t		apikey;
		uint32_t		dataLen;	//max: 16MB

		ResponseHeader(): status(0), apikey(0), dataLen(0) {}
	} __attribute__((packed));

	Header				header;
	Key					key;
	std::string			path;
	std::string			data;

	Client				*client;

	ResponseHeader		resHeader;
	std::string			result;

	void execute();

	static Job *parse( const std::string&, size_t* );
	static void finish( Job* );

	void dump();
};

#endif /* defined(__defer_io__Job__) */
