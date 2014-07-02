//
//  Datum.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#include "Datum.h"
#include "VBucket.h"

Datum::Datum( const std::string &_key ): key(_key), lastSyncVersion(0), _readLock(PTHREAD_MUTEX_INITIALIZER), _writeLock(PTHREAD_MUTEX_INITIALIZER), header({0,}), data()
{
	if ( key.length() > 0 )
		this->header.vBucketID = VBucket::hash(this->key);
}

Datum::Datum( const char *_key, size_t len ): key(_key, len), lastSyncVersion(0), _readLock(PTHREAD_MUTEX_INITIALIZER), _writeLock(PTHREAD_MUTEX_INITIALIZER), header({0,}), data()
{
	if ( len > 0 )
		this->header.vBucketID = VBucket::hash(this->key);
}

bool Datum::loadData( bool cache )
{
	VBucket *bucket = VBucket::getOnlineBucket( header.vBucketID );

	if ( bucket )
	{
	}

	return false;//setData( data );
}

bool Datum::setData( const std::string &_data )
{
	try
	{
		data.parse( _data );
	}
	catch ( const std::logic_error& le )
	{
		DEBUGS( le.what() )
	}
	return false;
}
