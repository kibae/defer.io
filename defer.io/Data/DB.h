//
//  DB.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#ifndef __defer_io__DB__
#define __defer_io__DB__

#include "../include.h"

#include "Key.h"

#include <unordered_map>
#include <leveldb/db.h>

class DB
{
public:
	static std::string		datadir;
	static uint16_t			vBucketSize;

	static void init( const std::string &datadir );

	class VBucket: public RWLock {
	private:
		uint16_t			_id;
		leveldb::DB			*db;
	public:
		pthread_rwlock_t	keyLocks[Key::lockSize] = {PTHREAD_RWLOCK_INITIALIZER,};
		bool				readonly;

		uint16_t id();
		VBucket( uint16_t id );
		~VBucket();

		bool get( const Key&, std::string * );
		bool exists( const Key& );
		bool put( const Key &, const std::string & );
		bool set( const Key &, const std::string & );
	};

	static std::unordered_map<uint16_t, DB::VBucket*> vBuckets;
	typedef std::pair<uint16_t, DB::VBucket*> vBucketRow;

	static VBucket *getBucket( const Key &k );
};

#endif /* defined(__defer_io__DB__) */
