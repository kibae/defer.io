//
//  Datum.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#ifndef __defer_io__Datum__
#define __defer_io__Datum__

#include "../include.h"

#include "JSON.h"

class Datum
{
private:
	std::string				key;		//immutable

	uint32_t				lastSyncVersion;
	pthread_mutex_t			_readLock;
	pthread_mutex_t			_writeLock;
public:
	Datum( const std::string &key );
	Datum( const char *key, size_t len );

	struct Header
	{
		uint16_t		vBucketID;	//immutable
		uint16_t		keyLen:14;	//max: 16KB, immutable
		uint16_t		compressed:1;
		uint16_t		deleted:1;
		uint32_t		version;
		uint32_t		timeLastChange;
		uint32_t		dataLen;	//max: 16MB
	} __attribute__((packed));

	Header					header;
	JSON					data;

	bool loadData( bool cache=true );
	bool setData( const std::string &data );

	inline bool isModified() {
		return lastSyncVersion >= header.version;
	}
};

#endif /* defined(__defer_io__Datum__) */
