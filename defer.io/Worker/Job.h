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

#include "Client.h"
#include "Key.h"
#include "JSON.h"

class Client;

class Job: public RWLock
{
	void initKey( std::string& );

	Client				*client;

	Job( const char *buf );

	Job( uint8_t cmd, std::string key, std::string data );
	Job( uint8_t cmd, std::string key );
public:
	std::atomic<uint32_t> refCount;

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
		ReplEntryHeader(): res(), kLen(0), vLen(0) {
			res.status = Json::ReplEntry;
		}

		void set( const std::string &k, const std::string &v ) {
			kLen = (uint16_t) k.length();
			vLen = (uint32_t) v.length();
			res.dataLen = (uint32_t) (sizeof(uint16_t)+sizeof(uint32_t)+kLen+vLen);
		}
		void reset() {
			kLen = vLen = 0;
			res.dataLen = (uint32_t) (sizeof(uint16_t)+sizeof(uint32_t));
		}
	} __attribute__((packed));

	Header				header;
	Key					key;
	Json::Path			path;
	std::string			data;

	std::string			result;

	void execute();
	void finish();

	static Job *parse( const std::string&, size_t*, Client* );
	bool response( std::string &buf );
	void dump();
};

#endif /* defined(__defer_io__Job__) */
