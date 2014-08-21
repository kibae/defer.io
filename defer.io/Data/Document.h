//
//  Document.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#ifndef __defer_io__Document__
#define __defer_io__Document__

#include "../include.h"

class Document;

#include "JSON.h"
#include "DB.h"
#include "Key.h"
#include "Cache.h"

#include <vector>

class Document: public RWLock
{
private:
	Key				key;
	Json			content;

	DB::VBucket		*bucket;

	bool			_created;
	bool			_changed;
	uint64_t		_timeChanged;

	std::string		_out;
	uint64_t		_timeOutGenerated;

	uint64_t		_version;
public:
	Document( const Key &, uint64_t version=0 );
	Document( const Key &, const std::string &, uint64_t version=0 );
	Document( DB::VBucket *b, const Key &, uint64_t version=0 );
	Document( DB::VBucket *b, const Key &, const std::string &, uint64_t version=0 );
	~Document();

	void setBucket( DB::VBucket *b );
	void setData( const std::string &v );

	Key &getKey();

	bool save();

	uint64_t version();
	void setVersion( uint64_t v );
	uint64_t versionIncr();

	bool changed();
	void touch();
	void created();

	std::string out();

	enum CMD : uint8_t {	//arg
		AUTH = 10,
		SYSTEM_CMD = 70,
		FlushCache,

		OBJECT_CMD = 90,	//char(d)
		Sync = 91,

		Exists = 100,		//getter

		Get,				//getter
		GetOrCreate,		//[default value]
		Set,				//[new value]
		GetSet,				//[new value]
		SetIfNotExists,		//[new value]

		ArrayPush = 120,	//[value[, value, value, ...]]
		ArrayPop,			//
		ArrayUnshift,		//[value[, value, value, ...]]
		ArrayShift,			//
		ArraySplice,		//[off, del count[, value, value, ...]]
		ArrayCut,			//[size, off=0]
		ArraySlice,			//[size, off=0], getter
		ArrayCount,			//getter
		ArrayRandGet,		//getter
		ArrayRandPop,		//
		//sort? rsort?

		ListPush = 140,		//[list size, value[, value, ..]], cut left
		ListUnshift,		//[list size, value[, value, ..]], cut right
		ListPushUniq,		//[list size, value[, value, ..]] only string/number, cut left
		ListUnshiftUniq,	//[list size, value[, value, ..]] only string/number, cut right

		ObjectSet = 160,	//[key, value[, key, value, ..]]
		ObjectSetIfNotExists,	//[key, value[, key, value, ..]]
		ObjectDel,			//[key[, key, ...]]
		ObjectKeys,			//[search], getter

		NumberIncr = 180,	//[incr_val]
		NumberDecr,			//[decr_val]

		StringAppend = 200,	//[right_str]
		StringPrepend,		//[left_str]
		StringLength,		//getter
		StringSub,			//[offset, len], getter
		//search/match? replace?

		BoolToggle = 220,	//
	};

	Json execute( Json::Path &path, CMD cmd, Json &arg );
	Json manipulate( Json &sub, CMD cmd, Json &arg );

	static Document *getOrCreate( const Key& );
};

#endif /* defined(__defer_io__Document__) */
