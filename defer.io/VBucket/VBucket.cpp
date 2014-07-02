//
//  VBucket.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#include "VBucket.h"
#include "xxhash.h"

std::unordered_map<uint16_t, VBucket*> VBucket::onlineBuckets;

VBucket::VBucket( uint16_t _id ): _lock(PTHREAD_MUTEX_INITIALIZER), id(_id), assigned(false), readOnly(true)
{
	onlineBuckets.insert( bucketRow(id, this) );
}

VBucket *VBucket::getBucket( const uint16_t keyHash )
{
	return NULL;
}

VBucket *VBucket::getOnlineBucket( const uint16_t keyHash )
{
	return onlineBuckets[keyHash];
}

std::string VBucket::getData( const uint16_t keyHash, const std::string &key )
{
	return NULL;
}

uint16_t VBucket::hash( const std::string &key )
{
	return (uint16_t) (XXH32( key.c_str(), (int) key.length(), 0 ) % (4096));
}
