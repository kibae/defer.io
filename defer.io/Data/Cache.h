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

#include <unordered_map>
#include "Document.h"
#include "Key.h"

class Cache: public RWLock
{
private:
	//LRU cache
	struct LRU {
		struct Entry {
			Entry		*prev;	//LRU double linked list
			Entry		*next;	//LRU double linked list

			Document	*val;

			Entry( Document *v ): prev(NULL), next(NULL), val(v) {}
		};

		static void insertAfter( Entry *node, Entry *newNode );
		static void insertBefore( Entry *node, Entry *newNode );
		static void insertBeginning( Entry *newNode );
		static void insertEnd( Entry *newNode );
		static void remove( Entry *node );

		static Entry		*head;
		static Entry		*tail;
	};

	static std::unordered_map<std::string, LRU::Entry*> data;
	typedef std::pair<std::string, LRU::Entry*> dataRow;
	
	static pthread_rwlock_t		_lock;
	static unsigned long		countLimit;
public:
	static uint64_t seq;
	static inline void rlock() {
		pthread_rwlock_rdlock( &_lock );
	}

	static inline void wlock() {
		pthread_rwlock_wrlock( &_lock );
	}

	static inline void unlock() {
		pthread_rwlock_unlock( &_lock );
	}

	static void setCountLimit( long cnt );

	static bool exists( const Key &key );
	static void push( const Key &key, Document* datum, bool ignoreExists=true );
	static void remove( const Key &key, bool noLock=false );
	static Document *get( const Key &key );
};

#endif /* defined(__defer_io__Cache__) */
