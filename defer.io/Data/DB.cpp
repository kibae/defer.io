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

DB::VBucket::VBucket( uint16_t id ): _id(id), db(NULL), _outOfService(false), hooksTouch(), hooksSave()
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

	for ( int i=0; i < hooksTouch.size(); i++ )
	{
		Job *job = hooksTouch[i];
		job->refCount--;
		job->client->jobFinish( job );
	}
	hooksTouch.clear();

	for ( int i=0; i < hooksSave.size(); i++ )
	{
		Job *job = hooksSave[i];
		job->refCount--;
		job->client->jobFinish( job );
	}
	hooksSave.clear();
}

bool DB::VBucket::get( const Key &k, std::string *v, bool force )
{
	if ( _outOfService && !force )
		throw std::logic_error( "This virtual bucket is out of service" );
	leveldb::Status s = db->Get( leveldb::ReadOptions(), k.str(), v );
	return s.ok();
}

bool DB::VBucket::exists( const Key &k )
{
	if ( _outOfService )
		throw std::logic_error( "This virtual bucket is out of service" );
	leveldb::Status s = db->Get( leveldb::ReadOptions(), k.str(), NULL );
	return !s.IsNotFound();
}

bool DB::VBucket::set( const Key &k, const std::string &v )
{
	if ( _outOfService )
		throw std::logic_error( "This virtual bucket is out of service" );
	leveldb::Status s = db->Put( leveldb::WriteOptions(), k.str(), v );

	if ( s.ok() && hooksSave.size() > 0 )
		replBroadcast( hooksSave, k.str(), v );

	return s.ok();
}

void DB::VBucket::replBroadcast( std::vector<Job*> &hooks, const std::string &k, const std::string &v )
{
	Job::ReplEntryHeader header( k, v );
	std::string buf( (char *) &header, sizeof(Job::ReplEntryHeader) );
	buf.append( k );
	buf.append( v );

	for ( long i=hooks.size(); i--; )
	{
		Job *job = hooks[i];
		if ( job->client->connected() )
			job->client->jobResponse( buf );
		else
		{
			hooks.erase( hooksSave.begin()+i );
			job->refCount--;
			job->client->jobFinish( job );
		}
	}
}

bool DB::VBucket::outOfService()
{
	return _outOfService;
}

uint16_t DB::VBucket::id()
{
	return _id;
}

bool DB::VBucket::setShardSource()
{
	if ( _outOfService )
		return false;
	_outOfService = true;

	//TODO: collapse cache

	return true;
}

bool DB::VBucket::hookTouch( Job *job )
{
	//TODO: implement
	if ( _outOfService )
		throw std::logic_error( "This virtual bucket is out of service" );
	job->refCount++;
	hooksTouch.push_back( job );
	return true;
}

bool DB::VBucket::hookSave( Job *job )
{
	if ( _outOfService )
		throw std::logic_error( "This virtual bucket is out of service" );
	job->refCount++;
	hooksSave.push_back( job );
	return true;
}

bool DB::VBucket::dump( Job *job, uint64_t time )
{
	//TODO: implement
	job->refCount++;
	return true;
}

DB::VBucket *DB::getBucket( const Key &k )
{
	return vBuckets[k.bucketID()];
}

DB::VBucket *DB::getBucket( const uint16_t id )
{
	return vBuckets[id];
}

bool DB::get( const Key &k, std::string *buf, bool force )
{
	k.wlock();
	VBucket *bucket = getBucket(k);
	if ( bucket == NULL )
	{
		k.unlock();
		return false;
	}

	bool res = false;
	try
	{
		res = bucket->get( k, buf, force );
	}
	catch ( const std::logic_error &le )
	{
		//TODO: log
		k.unlock();
		return false;
	}
	k.unlock();
	return res;
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
