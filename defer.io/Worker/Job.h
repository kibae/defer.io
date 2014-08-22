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
#include "JSON.h"

class Job
{
	void initKey( std::string& );
public:
	std::atomic<uint32_t> refCount;

	Job( const char *buf );

	Job( uint8_t cmd, std::string key, std::string data );
	Job( uint8_t cmd, std::string key );
	~Job();

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

	struct ReplEntryHeader
	{
		ResponseHeader	res;
		uint16_t		kLen;
		uint32_t		vLen;

		ReplEntryHeader( const std::string &k, const std::string &v ): res(), kLen((uint16_t) k.length()), vLen((uint32_t) v.length()) {
			res.status = Json::ReplEntry;
			res.dataLen = (uint32_t) (sizeof(uint16_t)+sizeof(uint32_t)+kLen+vLen);
		}
	} __attribute__((packed));

	Header				header;
	Key					key;
	Json::Path			path;
	std::string			data;

	Client				*client;

	std::string			result;

	void execute();

	static Job *parse( const std::string&, size_t* );
	static void finish( Job* );

	void dump();
};

#endif /* defined(__defer_io__Job__) */
