//
//  Cache.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#ifndef __defer_io__Cache__
#define __defer_io__Cache__

#include "../include.h"

class Cache;

#include <unordered_map>
#include <list>
#include <vector>
#include "Document.h"
#include "Key.h"
#include "Status.h"

class Cache: public RWLock
{
private:
	std::unordered_map<std::string, Document*> data;
	typedef std::pair<std::string, Document*> dataRow;

	std::list<Document*> list;
	uint64_t _changed;
	uint64_t _lastChanged;

	static std::vector<Cache*>	pool;
	static unsigned long		memoryLimit;
	static unsigned long		countLimit;
	static unsigned long		perCountLimit;
public:
	uint64_t seq;

	static void init( long mem, long cnt );

	Cache();

	bool exists( const Key &key );
	void push( const Key &key, Document* datum, bool ignoreExists=true );
	Document *get( const Key &key );
	void sync();
	void flushLast();
	void flush();
	void changed();

	static Document *getCache( const Key& );
	static void syncAll();
	static void flushAll();
	static Cache *getCachePool( const uint8_t );
	static Cache *getCachePool( const Key& );

	static void setMemoryLimit( unsigned long mem );
	static void setCountLimit( unsigned long cnt );
};

#endif /* defined(__defer_io__Cache__) */
