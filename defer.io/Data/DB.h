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
#include "Client.h"

#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <leveldb/db.h>

class Document;
class DB
{
	static std::thread				syncThread;
	static std::mutex				syncLock;
	static std::condition_variable	syncCond;
	static uint64_t					syncReqCount;
	static long						syncReqInterval;
public:
	static std::string		datadir;
	static uint16_t			vBucketSize;

	static void init( const std::string&, const long );

	class VBucket: public RWLock {
	private:
		uint16_t				_id;
		leveldb::DB				*db;
		bool					_outOfService;

		std::vector<Job*>		hooksTouch;
		std::vector<Job*>		hooksSave;
	public:
		pthread_rwlock_t	keyLocks[Key::lockSize];

		uint16_t id();
		VBucket( uint16_t id );
		~VBucket();

		bool outOfService();

		/// \brief Get datum from data bucket.
		bool get( const Key&, std::string *, bool force=false );

		bool exists( const Key& );
		bool set( const Key &, const std::string & );

		bool setShardSource();

		bool hookTouch( Job *job );
		bool hasTookTouch();
		void dispatchTouch( Document *doc );
		bool hookSave( Job *job );
		bool hasTookSave();
		bool dump( Job *job, uint64_t time=-1 );

		void replBroadcast( std::vector<Job*> &hooks, const std::string &k, const std::string &v );
	};

	static std::unordered_map<uint16_t, DB::VBucket*> vBuckets;
	typedef std::pair<uint16_t, DB::VBucket*> vBucketRow;

	static VBucket *getBucket( const Key &k );
	static VBucket *getBucket( const uint16_t id );
	static bool get( const Key&, std::string *, bool force=false );
	static void sync();
	static void changed();
	static void touched( Document *doc );
};

#endif /* defined(__defer_io__DB__) */
