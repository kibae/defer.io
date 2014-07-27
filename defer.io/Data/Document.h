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

#include "JSON.h"
#include "DB.h"
#include "Key.h"

#include <vector>

class Document: public RWLock
{
	Key				key;
	JSON			content;

	DB::VBucket		*bucket;

	bool			_created;
	bool			_destroyed;
	bool			_changed;
	uint64_t		_timeChanged;

	std::string		_out;
	uint64_t		_timeOutGenerated;

	static std::vector<Document *> destroyPool;

	JSON get( const Json::Value& );
	JSON set( Json::Value&, const JSON& );
	JSON getSet( Json::Value&, const JSON& );

	JSON arrayPush( Json::Value&, const JSON& );
	JSON arrayPop( Json::Value&, const JSON& );
	JSON arrayUnshift( Json::Value&, const JSON& );
	JSON arrayShift( Json::Value&, const JSON& );
	JSON arraySplice( Json::Value&, const JSON& );
	JSON arrayCut( Json::Value&, const JSON& );
	JSON arraySlice( Json::Value&, const JSON& );
	JSON arrayCount( Json::Value&, const JSON& );

	JSON listPush( Json::Value&, const JSON&, bool checkUniq=false );
	JSON listUnshift( Json::Value&, const JSON&, bool checkUniq=false );
	JSON listUniqArg( Json::Value&, const JSON& );

	JSON objectSet( Json::Value&, const JSON& );
	JSON objectDel( Json::Value&, const JSON& );
	JSON objectKeys( Json::Value&, const JSON& );

	JSON numberIncr( Json::Value&, const JSON& );
	JSON numberDecr( Json::Value&, const JSON& );

	JSON stringAppend( Json::Value&, const JSON& );
	JSON stringPrepend( Json::Value&, const JSON& );
	JSON stringLength( Json::Value&, const JSON& );

	JSON boolToggle( Json::Value&, const JSON& );
public:
	Document( const Key & );
	Document( const Key &, const std::string & );
	Document( DB::VBucket *b, const Key & );
	Document( DB::VBucket *b, const Key &, const std::string & );

	void setBucket( DB::VBucket *b );
	void setData( const std::string &v );

	Key &getKey();

	bool save();
	void destroy();

	bool changed();
	void touch();
	void created();

	std::string out();

	enum CMD {				//arg
		Exists,

		Set,				//[new value]
		Get,				//getter
		GetSet,				//[new value]

		ArrayPush,			//[value[, value, value, ...]]
		ArrayPop,			//
		ArrayUnshift,		//[value[, value, value, ...]]
		ArrayShift,			//
		ArraySplice,		//[off, del count[, value, value, ...]]
		ArrayCut,			//[size, off=0]
		ArraySlice,			//[size, off=0], getter
		ArrayCount,			//getter
		//random get? random pop?

		ListPush,			//[list size, value[, value, ..]], cut left
		ListUnshift,		//[list size, value[, value, ..]], cut right
		ListPushUniq,		//[list size, value[, value, ..]] only string/number, cut left
		ListUnshiftUniq,	//[list size, value[, value, ..]] only string/number, cut right

		ObjectSet,			//[key, value[, key, value, ..]]
		ObjectDel,			//[key[, key, ...]]
		ObjectKeys,			//getter

		NumberIncr,			//[incr_val]
		NumberDecr,			//[decr_val]

		StringAppend,		//[right_str]
		StringPrepend,		//[left_str]
		StringLength,		//getter
		//search? replace? substr?

		BoolToggle,			//
	};

	JSON execute( const std::string&, CMD, const std::string );
	JSON execute( const std::string&, CMD, const JSON& );
	JSON manipulate( Json::Value&, CMD, const JSON& );

	static Document *get( const Key& );
	static Document *getOrCreate( const Key& );
	static Document *set( const Key&, const std::string& );
};

#endif /* defined(__defer_io__Document__) */
