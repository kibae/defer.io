//
//  Cache.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "Cache.h"

std::vector<Cache*>		Cache::pool;
unsigned long			Cache::memoryLimit = Config::DEF_CACHE_MEMORY_LIMIT;
unsigned long			Cache::countLimit = Config::DEF_CACHE_COUNT_LIMIT;
unsigned long			Cache::perCountLimit = ceil( ((double) Config::DEF_CACHE_COUNT_LIMIT)/((double) Key::lockSize) );

void Cache::init( long mem, long cnt )
{
	setMemoryLimit( mem );
	setCountLimit( cnt );
	Json::init();

	for ( uint8_t i=0; i < Key::lockSize; i++ )
	{
		pool.push_back( new Cache() );
	}
}

void Cache::setMemoryLimit( unsigned long mem )
{
	Cache::memoryLimit = mem;
}

void Cache::setCountLimit( unsigned long cnt )
{
	Cache::countLimit = cnt;
	Cache::perCountLimit = ceil( ((double) cnt)/((double) Key::lockSize) );
}

Cache::Cache(): _changed(0), _lastChanged(0), seq(0)
{
}

bool Cache::exists( const Key &key )
{
	rlock();
	std::unordered_map<std::string, Document*>::const_iterator got = data.find( key.str() );
	bool exists = got != data.end();
	unlock();

	return exists;
}

void Cache::push( const Key &key, Document *datum, bool ignoreExists )
{
	Status::cacheInRetain();

	wlock();
	Document *check = data[key.str()];
	if ( check != NULL )
	{
		if ( ignoreExists )
		{
			LLOG
			delete check;
			Status::cacheOutRetain();
			//check->destroy();
		}
		else
			throw std::runtime_error( "Data is current exists." );
	}
	data[key.str()] = datum;
	seq++;

	list.push_front( datum );
	if ( data.size() > Cache::perCountLimit )
	{
		Document *oldest = list.back();
		if ( oldest != NULL )
		{
			list.pop_back();

			oldest->save();
			data.erase( oldest->getKey().str() );
			delete oldest;

			Status::cacheOutRetain();
		}
	}
	unlock();
}

Document *Cache::getCache( const Key &key )
{
	Cache *pool = getCachePool( key.cacheKey() );
	if ( pool == NULL )
		return NULL;
	return pool->get( key );
}

Document *Cache::get( const Key &key )
{
	Status::cacheReqRetain();

	wlock();
	Document *doc = data[key.str()];
	if ( doc && list.front() != doc )
	{
		std::list<Document*>::iterator it = std::find( list.begin(), list.end(), doc );
		list.erase( it );
		list.push_front( doc );
	}
	unlock();

	if ( doc )
		Status::cacheHitRetain();

	return doc;
}

void Cache::sync()
{
	//TODO: sync
	return;
	/*
	if ( data.size() <= 0 ) return;

	//bg save
	int i = (int) ((float) Cache::countLimit / 10.0f);

	rlock();
	std::unordered_map<std::string, LRU::Entry*>::iterator it = data.begin();
	if ( data.size() > i )
	{
		//std::advance( it, (abs((int) random())%data.size()) - i );
	}

	for ( int z = 0; it != data.end(); ++it, z++ )
	{
		//DEBUGS(z)
		if ( it->second != NULL || it->second->val == NULL || !it->second->val->changed() )
			continue;
		i--;
		DEBUGS(it->first)
		it->second->val->save();
	}
	unlock();
	 */
}

void Cache::flushLast()
{
	wlock();
	Document *doc = list.back();
	if ( doc != NULL )
	{
		list.pop_back();
		data.erase( doc->getKey().str() );

		doc->wlock();
		if ( doc->changed() )
			doc->save();
		doc->unlock();
		delete doc;
		Status::cacheOutRetain();

		seq++;
	}
	unlock();
}

void Cache::flush()
{
	wlock();
	while ( list.size() > 0 )
	{
		Document *doc = list.back();
		list.pop_back();

		doc->wlock();
		if ( doc->changed() )
			doc->save();
		doc->unlock();
		delete doc;
		Status::cacheOutRetain();
	}
	list.clear();
	data.clear();
	seq++;
	unlock();
}

void Cache::changed()
{
	_changed++;
}


void Cache::syncAll()
{
	for ( uint8_t i=0; i < Key::lockSize; i++ )
	{
		//std::cout << (int)i << "	" << pool[i]->data.size() << "\n";
		pool[i]->sync();
	}

	long i = Cache::perCountLimit/3;
	while ( i-- && Status::memoryUsed() > Cache::memoryLimit )
	{
		for ( uint8_t i=0; i < Key::lockSize; i++ )
		{
			pool[i]->flushLast();
		}
	}
	//DEBUGS(Json::Memory::size())
}

void Cache::flushAll()
{
	for ( uint8_t i=0; i < Key::lockSize; i++ )
	{
		pool[i]->flush();
	}
}

Cache *Cache::getCachePool( const uint8_t idx )
{
	return pool[idx];
}

Cache *Cache::getCachePool( const Key &key )
{
	return pool[key.cacheKey()];
}

void Cache::clearByBucket( DB::VBucket *bucket )
{
	for ( uint8_t i=0; i < Key::lockSize; i++ )
	{
		Cache *cache = pool[i];

		cache->wlock();
		std::list<Document*>::iterator it = cache->list.begin();
		while ( it != cache->list.end() )
		{
			Document *doc = *it;
			if ( doc->bucket == bucket )
			{
				cache->data.erase( doc->getKey().str() );
				cache->list.erase( it++ );

				doc->wlock();
				if ( doc->changed() )
					doc->save();
				doc->unlock();
				delete doc;

				Status::cacheOutRetain();
				continue;
			}
			++it;
		}
		cache->unlock();
	}
}

