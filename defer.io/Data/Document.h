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
public:
	enum Status : uint16_t {
		OK = 200,
		NotFound = 404,
		LogicError = 500,
	};
private:
	Key				key;
	JSONDoc			content;

	DB::VBucket		*bucket;

	bool			_created;
	bool			_changed;
	uint64_t		_timeChanged;

	std::string		_out;
	uint64_t		_timeOutGenerated;

	Status get( JSONDoc&, JSONDoc& );
	Status getOrCreate( JSONDoc&, JSONDoc&, JSONDoc& );
	Status set( JSONDoc&, JSONDoc&, JSONDoc& );
	Status getSet( JSONDoc&, JSONDoc&, JSONDoc& );
	Status setIfNotExists( JSONDoc&, JSONDoc&, JSONDoc& );

	Status arrayPush( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayPop( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayUnshift( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayShift( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arraySplice( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayCut( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arraySlice( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayCount( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayRandGet( JSONDoc&, JSONDoc&, JSONDoc& );
	Status arrayRandPop( JSONDoc&, JSONDoc&, JSONDoc& );

	void listUniqArg( JSONDoc&, JSONDoc&, JSONVal& );
	Status listPush( JSONDoc&, JSONDoc&, JSONDoc&, bool checkUniq=false );
	Status listUnshift( JSONDoc&, JSONDoc&, JSONDoc&, bool checkUniq=false );

	Status objectSet( JSONDoc&, JSONDoc&, JSONDoc& );
	Status objectSetIfNotExists( JSONDoc&, JSONDoc&, JSONDoc& );
	Status objectDel( JSONDoc&, JSONDoc&, JSONDoc& );
	Status objectKeys( JSONDoc&, JSONDoc&, JSONDoc& );

	Status numberIncr( JSONDoc&, JSONDoc&, JSONDoc& );
	Status numberDecr( JSONDoc&, JSONDoc&, JSONDoc& );

	Status stringAppend( JSONDoc&, JSONDoc&, JSONDoc& );
	Status stringPrepend( JSONDoc&, JSONDoc&, JSONDoc& );
	Status stringLength( JSONDoc&, JSONDoc&, JSONDoc& );
	Status stringSub( JSONDoc&, JSONDoc&, JSONDoc& );

	Status boolToggle( JSONDoc&, JSONDoc&, JSONDoc& );
public:
	Document( const Key & );
	Document( const Key &, const std::string & );
	Document( DB::VBucket *b, const Key & );
	Document( DB::VBucket *b, const Key &, const std::string & );
	~Document();

	void setBucket( DB::VBucket *b );
	void setData( const std::string &v );

	Key &getKey();

	bool save();

	bool changed();
	void touch();
	void created();

	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &allocator();

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

	Status execute( const std::string&, CMD, JSONDoc&, JSONDoc& );
	Status manipulate( JSONDoc&, CMD, JSONDoc&, JSONDoc& );

	static Document *getOrCreate( const Key& );

	class Error : public std::runtime_error {
		Status		_status;
	public:
		Error( const Status, const std::string& );
		Error( const Status, const char* );
		Status status() const;
	};
};

#endif /* defined(__defer_io__Document__) */
