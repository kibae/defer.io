//
//  DB.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "DB.h"

std::string DB::datadir;
std::unordered_map<uint16_t, DB::VBucket*> DB::vBuckets;
uint16_t DB::vBucketSize = 100;

void DB::init( const std::string &_datadir )
{
	datadir = _datadir;
	mkdir( _datadir.c_str(), S_IRWXU );

	for ( uint16_t i=0; i < vBucketSize; i++ )
	{
		new VBucket( i );
	}
}

DB::VBucket::VBucket( uint16_t id ): _id(id), db(NULL), readonly(false)
{
	leveldb::Options	options;
	std::stringstream	path;

	options.create_if_missing = true;

	path << datadir << "/" << id << ".db";
	//DEBUGS( path.str() )
	leveldb::Status s = leveldb::DB::Open( options, path.str(), &db );
	if ( !s.ok() )
		throw std::length_error( s.ToString() );

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
	leveldb::Status s = db->Get( leveldb::ReadOptions(), k.str(), v );
	return s.ok();
}

bool DB::VBucket::exists( const Key &k )
{
	leveldb::Status s = db->Get( leveldb::ReadOptions(), k.str(), NULL );
	return !s.IsNotFound();
}

bool DB::VBucket::put( const Key &k, const std::string &v )
{
	leveldb::Status s = db->Put( leveldb::WriteOptions(), k.str(), v );
	return s.ok();
}

bool DB::VBucket::set( const Key &k, const std::string &v )
{
	//TODO: check delete, put
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
