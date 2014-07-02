//
//  VBucket.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#ifndef __defer_io__VBucket__
#define __defer_io__VBucket__

#include "../include.h"

#include <unordered_map>

class VBucket
{
private:
	pthread_mutex_t		_lock;
public:
	uint16_t			id;
	bool				assigned;
	bool				readOnly;

	VBucket( uint16_t id );

	inline void lock() {
		pthread_mutex_lock( &_lock );
	}

	inline void unlock() {
		pthread_mutex_unlock( &_lock );
	}

	static uint16_t hash( const std::string &key );

	static std::unordered_map<uint16_t, VBucket*> onlineBuckets;
	typedef std::pair<uint16_t, VBucket*> bucketRow;

	static VBucket *getBucket( const uint16_t keyHash );
	static VBucket *getOnlineBucket( const uint16_t keyHash );
	static std::string getData( const uint16_t keyHash, const std::string &key );
};

#endif /* defined(__defer_io__VBucket__) */
