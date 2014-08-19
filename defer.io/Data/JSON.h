//
//  JSON.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#ifndef __defer_io__JSON__
#define __defer_io__JSON__

#include "../include.h"

class JSONMem
{
	static ssize_t			used;
	static pthread_mutex_t	_lock;
public:
	static void retain( ssize_t sz );
	static void release( ssize_t sz );
	static ssize_t size();

	static inline void lock() {
		pthread_mutex_lock( &_lock );
	}

	static inline void unlock() {
		pthread_mutex_unlock( &_lock );
	}
};

#include "../external/rapidjson/rapidjson.h"
#include "../external/rapidjson/allocators.h"
#include "../external/rapidjson/document.h"
#include "../external/rapidjson/writer.h"

typedef rapidjson::Value JSONVal;

class JSONDoc : public rapidjson::Document
{
public:
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> allocator;

	JSONDoc();
	~JSONDoc();
};

class JSON
{
public:
	const static JSONVal Null;

	static bool parse( const std::string&, JSONDoc& );
	static std::string stringify( JSONVal& );

	rapidjson::Value &test() {
		rapidjson::Value doc(1);
		return doc.Move();
	}
};



#endif /* defined(__defer_io__JSON__) */
