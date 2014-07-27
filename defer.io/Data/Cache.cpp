//
//  Cache.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "Cache.h"

uint64_t Cache::seq = 0;
std::unordered_map<std::string, Cache::LRU::Entry*> Cache::data;

pthread_rwlock_t	Cache::_lock = PTHREAD_RWLOCK_INITIALIZER;
unsigned long		Cache::countLimit = Config::DEF_CACHE_COUNT_LIMIT;

Cache::LRU::Entry		*Cache::LRU::head = NULL;
Cache::LRU::Entry		*Cache::LRU::tail = NULL;


void Cache::setCountLimit( long cnt )
{
	Cache::countLimit = cnt;
}

bool Cache::exists( const Key &key )
{
	rlock();
	std::unordered_map<std::string, LRU::Entry*>::const_iterator got = data.find( key.str() );
	bool exists = got != data.end();
	unlock();

	return exists;
}

void Cache::push( const Key &key, Document *datum, bool ignoreExists )
{
	LRU::Entry	*entry = NULL;

	wlock();
	entry = data[key.str()];
	if ( entry )
	{
		if ( ignoreExists )
		{
			if ( entry->val )
				entry->val->destroy();
			entry->val = datum;
		}
		else
			throw std::runtime_error( "Data is current exists." );
	}
	else
	{
		entry = data[key.str()] = new LRU::Entry( datum );
	}
	seq++;

	LRU::insertBeginning( entry );
	if ( data.size() > Cache::countLimit )
	{
		LRU::Entry *oldest = LRU::tail;
		if ( oldest )
		{
			LRU::remove( oldest );
			if ( oldest->val )
			{
				oldest->val->save();
				remove( oldest->val->getKey(), true );
			}
		}
	}
	unlock();
}

void Cache::remove( const Key &key, bool noLock )
{
	if ( !noLock ) wlock();

	LRU::Entry	*entry = data[key.str()];
	if ( !entry )
	{
		if ( !noLock ) unlock();
		return;
	}

	if ( entry->val )
		entry->val->destroy();
	data.erase( key.str() );

	seq++;
	if ( !noLock ) unlock();

	delete entry;
}

Document *Cache::get( const Key &key )
{
	rlock();
	LRU::Entry *entry = data[key.str()];
	Document *doc = entry && entry->val ? entry->val : NULL;
	unlock();

	return doc;
}

void Cache::LRU::insertAfter( Cache::LRU::Entry *node, Cache::LRU::Entry *newNode )
{
	newNode->prev = node;
	newNode->next = node->next;
	if ( node->next == NULL )
		tail = newNode;
	else
	{
		node->next->prev = newNode;
		node->next = newNode;
	}
}

void Cache::LRU::insertBefore( Cache::LRU::Entry *node, Cache::LRU::Entry *newNode )
{
	newNode->prev = node->prev;
	newNode->next = node;
	if ( node->prev == NULL )
		head = newNode;
	else
	{
		node->prev->next = newNode;
		node->prev = newNode;
	}

}

void Cache::LRU::insertBeginning( Cache::LRU::Entry *newNode )
{
	if ( head == NULL )
	{
		head = tail = newNode;
		newNode->prev = newNode->next = NULL;
	}
	else
		insertBefore( head, newNode );

}

void Cache::LRU::insertEnd( Cache::LRU::Entry *newNode )
{
	if ( tail == NULL )
		insertBeginning( newNode );
	else
		insertAfter( tail, newNode );

}

void Cache::LRU::remove( Cache::LRU::Entry *node )
{
	if ( node->prev == NULL )
		head = node->next;
	else
		node->prev->next = node->next;

	if ( node->next == NULL )
		tail = node->prev;
	else
		node->next->prev = node->prev;
}
