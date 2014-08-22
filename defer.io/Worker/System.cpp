//
//  System.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 18..
//
//

#include "System.h"
#include "Document.h"
#include "Cache.h"
#include "Status.h"

DB::VBucket *getBucket( Json &arg, int idx )
{
	int64_t id = arg.size() > idx ? arg[idx].asNumber() : -1;
	if ( id < 0 || id >= DB::vBucketSize )
		throw Json::Error( Json::LogicError, "Bucket id range error" );
	return DB::getBucket( (uint16_t) id );
}

Json System::execute( Job *job, Json &arg )
{
	Document::CMD cmd = (Document::CMD) job->header.cmd;
	switch ( cmd )
	{
		case Document::CMD::FlushCache:
			Cache::flushAll();
			break;
		case Document::CMD::Status:
			return *Status::dump();
		case Document::CMD::ReplStream:
		{
			DB::VBucket *bucket = getBucket( arg, 0 );
			if ( bucket == NULL || bucket->outOfService() )
				return *Json::False;
			bucket->hookTouch( job );
			break;
		}
		case Document::CMD::ReplStreamLazy:
		{
			DB::VBucket *bucket = getBucket( arg, 0 );
			if ( bucket == NULL || bucket->outOfService() )
				return *Json::False;
			bucket->hookSave( job );
			break;
		}
		case Document::CMD::ReplDump:
		{
			DB::VBucket *bucket = getBucket( arg, 0 );
			if ( bucket == NULL || bucket->outOfService() )
				return *Json::False;

			int64_t time = arg.size() > 1 ? arg[1].asNumber() : -1;
			bucket->dump( job, time );
			break;
		}
		case Document::CMD::ShardSetSource:
		{
			DB::VBucket *bucket = getBucket( arg, 0 );
			if ( bucket == NULL || bucket->outOfService() )
				return *Json::False;
			bool res = bucket->setShardSource();
			return res ? *Json::True : *Json::False;
		}
		case Document::CMD::ShardGetForce:
		{
			std::string buf;
			int res = DB::get( job->key, &buf, true );
			if ( !res )
				return *Json::Null;
			return *Json( buf );
		}
		default:
			return *Json::False;
			break;
	}

	return *Json::True;
}
