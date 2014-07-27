//
//  Key.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#ifndef __defer_io__Key__
#define __defer_io__Key__

#include "../include.h"

class Key:private std::string
{
	mutable bool		hashCalc;
	mutable uint32_t	_hash;
	mutable uint16_t	_bucketID;
	mutable uint8_t		_lockKey;
public:
	const static uint8_t lockSize = 113;	//prime

	Key();
	Key( std::string& );
	Key( const std::string& );

	void set( const char*, const size_t );
	void set( const std::string& );

	void validation();

	std::string str() const;

	void calcHash() const;
	uint32_t hash() const;
	uint8_t lockKey() const;
	uint16_t bucketID() const;
	void rlock() const;
	void wlock() const;
	void unlock() const;
};

#endif /* defined(__defer_io__Key__) */
