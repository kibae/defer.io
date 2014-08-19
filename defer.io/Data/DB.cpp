//
//  DB.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "DB.h"
#include "ThreadPool.h"
#include "Document.h"
#include "Cache.h"

std::thread DB::syncThread( DB::sync );
std::mutex DB::syncLock;
std::condition_variable DB::syncCond;
uint64_t DB::syncReqCount = 0;
long DB::syncReqInterval = Config::DEF_SYNC_REQ_INTERVAL;

std::string DB::datadir;
std::unordered_map<uint16_t, DB::VBucket*> DB::vBuckets;
uint16_t DB::vBucketSize = 100;

void DB::init( const std::string &_datadir, const long _syncReqInterval )
{
	datadir = _datadir;
	syncReqInterval = _syncReqInterval;
	mkdir( _datadir.c_str(), S_IRWXU );

	for ( uint16_t i=0; i < vBucketSize; i++ )
	{
		new VBucket( i );
	}
}

DB::VBucket::VBucket( uint16_t id ): _id(id), db(NULL), exporting(false), readonly(false)
{
	leveldb::Options	options;
	std::stringstream	path;

	options.create_if_missing = true;
	options.compression = leveldb::kSnappyCompression;

	path << datadir << "/" << id << ".db";
	//DEBUGS( path.str() )
	leveldb::Status s = leveldb::DB::Open( options, path.str(), &db );
	if ( !s.ok() )
		throw std::length_error( s.ToString() );

	for ( int i=0; i < Key::lockSize; i++ )
		keyLocks[i] = PTHREAD_RWLOCK_INITIALIZER;

	vBuckets.insert( vBucketRow(_id, this) );
}

DB::VBucket::~VBucket()
{
	if ( db != NULL )
	{
		delete db;
		db = NULL;
	}
}

bool DB::VBucket::get( const Key &k, std::string *v )
{
	if ( exporting )
		throw std::logic_error( "Exporting" );
	leveldb::Status s = db->Get( leveldb::ReadOptions(), k.str(), v );
	return s.ok();
}

bool DB::VBucket::exists( const Key &k )
{
	if ( exporting )
		throw std::logic_error( "Exporting" );
	leveldb::Status s = db->Get( leveldb::ReadOptions(), k.str(), NULL );
	return !s.IsNotFound();
}

bool DB::VBucket::set( const Key &k, const std::string &v )
{
	if ( exporting )
		throw std::logic_error( "Exporting" );
	leveldb::Status s = db->Put( leveldb::WriteOptions(), k.str(), v );
	return s.ok();
}

uint16_t DB::VBucket::id()
{
	return _id;
}

DB::VBucket *DB::getBucket( const Key &k )
{
	return vBuckets[k.bucketID()];
}

void DB::sync()
{
	while ( !ThreadPool::terminate )
	{
		std::unique_lock<std::mutex> lk( syncLock );
		syncCond.wait_for( lk, std::chrono::seconds(5) );

		Cache::syncAll();
	}
}

void DB::changed()
{
	syncReqCount++;
	if ( syncReqCount%syncReqInterval == 0 )
	{
		syncCond.notify_one();
	}
}
