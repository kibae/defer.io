//
//  Status.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 22..
//
//

#include "Status.h"
#include "ThreadPool.h"

time_t Status::_now = 0;
std::thread Status::timeThread( Status::timeProc );
Status::Entries Status::total;
Status::Entries Status::now;

//memory usage
void Status::memoryRetain( ssize_t sz )
{
	total.memoryUsed += sz;
	now.memoryUsed += sz;
}

void Status::memoryRelease( ssize_t sz )
{
	total.memoryUsed -= sz;
	now.memoryUsed -= sz;
}

ssize_t Status::memoryUsed()
{
	return total.memoryUsed;
}

//client
void Status::clientRetain()
{
	total.client++;
	now.client++;

	total.connect++;
	now.connect++;
}

void Status::clientRelease()
{
	total.client--;
	now.client--;
}

ssize_t Status::client()
{
	return total.client;
}

ssize_t Status::connect()
{
	return total.connect;
}

//job
void Status::jobRetain()
{
	total.job++;
	now.job++;

	total.jobReq++;
	now.jobReq++;
}

void Status::jobRelease()
{
	total.job--;
	now.job--;
}

ssize_t Status::job()
{
	return total.job;
}

ssize_t Status::jobReq()
{
	return total.jobReq;
}


//cache in
void Status::cacheInRetain()
{
	total.cacheIn++;
	now.cacheIn++;
}

ssize_t Status::cacheIn()
{
	return total.cacheIn;
}

//cache out
void Status::cacheOutRetain()
{
	total.cacheOut++;
	now.cacheOut++;
}

ssize_t Status::cacheOut()
{
	return total.cacheOut;
}

//cache req
void Status::cacheReqRetain()
{
	total.cacheReq++;
	now.cacheReq++;
}

ssize_t Status::cacheReq()
{
	return total.cacheReq;
}

//cache hit
void Status::cacheHitRetain()
{
	total.cacheHit++;
	now.cacheHit++;
}

ssize_t Status::cacheHit()
{
	return total.cacheHit;
}



//time thread
void Status::timeProc()
{
	while ( !ThreadPool::terminate )
	{
		_now = std::time( NULL );
		now.reset();
		sleep(1);
	}
}

void Status::_dump( Json &dst, Entries &entry, bool isNow )
{
	std::string k;

	dst.setObjMember( "memory" , (int64_t) entry.memoryUsed );
	if ( !isNow )
		dst.setObjMember( "client" , (int64_t) entry.client );
	dst.setObjMember( "connect" , (int64_t) entry.connect );
	dst.setObjMember( "job" , (int64_t) entry.job );
	dst.setObjMember( "jobReq" , (int64_t) entry.jobReq );
	dst.setObjMember( "cacheIn" , (int64_t) entry.cacheIn );
	dst.setObjMember( "cacheOut" , (int64_t) entry.cacheOut );
	dst.setObjMember( "cacheReq" , (int64_t) entry.cacheReq );
	dst.setObjMember( "cacheHit" , (int64_t) entry.cacheHit );
}

Json Status::dump()
{
	Json doc( JSON_OBJECT );

	Json oTotal( JSON_OBJECT );
	_dump( oTotal, total );
	Json oNow( JSON_OBJECT );
	_dump( oNow, now, true );

	doc.setObjMember( "total", oTotal );
	doc.setObjMember( "now", oNow );

	return *doc;
}

