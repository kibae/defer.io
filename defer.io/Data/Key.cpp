//
//  Key.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "Key.h"
#include "xxhash.h"
#include "DB.h"

Key::Key(): hashCalc(false), _hash(0) {}
Key::Key( std::string &k ): std::string(k), hashCalc(false), _hash(0)
{
	validation();
}

Key::Key( const std::string &k ): std::string(k), hashCalc(false), _hash(0)
{
	validation();
}

void Key::set( const char *k, const size_t len )
{
	this->append( k, len );
	validation();
}

void Key::set( const std::string &k )
{
	this->append( k );
	validation();
}

void Key::validation()
{
	if ( find_first_of(".[]") != std::string::npos )
		throw std::length_error( "Key cannot contains these characters: \".[]\"" );
}

std::string Key::str() const
{
	return *this;
}

void Key::calcHash() const
{
	hashCalc = true;
	_hash = XXH32( c_str(), length(), 0 );
	_bucketID = _hash%DB::vBucketSize;
	_lockKey = _hash%lockSize;
}

uint32_t Key::hash() const
{
	if ( !hashCalc )
		calcHash();
	return _hash;
}

uint8_t Key::lockKey() const
{
	if ( !hashCalc )
		calcHash();
	return _lockKey;
}

uint16_t Key::bucketID() const
{
	if ( !hashCalc )
		calcHash();
	return _bucketID;
}

void Key::rlock() const
{
	DB::VBucket *bucket = DB::getBucket(*this);
	if ( bucket != NULL )
		pthread_rwlock_rdlock( &(bucket->keyLocks[lockKey()]) );
}

void Key::wlock() const
{
	DB::VBucket *bucket = DB::getBucket(*this);
	if ( bucket != NULL )
		pthread_rwlock_wrlock( &(bucket->keyLocks[lockKey()]) );
}

void Key::unlock() const
{
	DB::VBucket *bucket = DB::getBucket(*this);
	if ( bucket != NULL )
		pthread_rwlock_unlock( &(bucket->keyLocks[lockKey()]) );
}
