//
//  Status.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 22..
//
//

#ifndef __defer_io__Status__
#define __defer_io__Status__

#include "../include.h"
#include "Json.h"

#include <time.h>
#include <thread>

class Status
{
	static std::thread				timeThread;
	static time_t					_now;

	struct Entries {
		std::atomic<ssize_t>		memoryUsed;

		std::atomic<ssize_t>		connect;
		std::atomic<ssize_t>		client;

		std::atomic<ssize_t>		job;
		std::atomic<ssize_t>		jobReq;

		std::atomic<ssize_t>		cacheIn;
		std::atomic<ssize_t>		cacheOut;
		std::atomic<ssize_t>		cacheHit;
		std::atomic<ssize_t>		cacheReq;

		Entries() : memoryUsed(0), connect(0), client(0), job(0), jobReq(0), cacheIn(0), cacheOut(0), cacheHit(0), cacheReq(0) {}
		void reset() {
			memoryUsed = 0;
			connect = 0;
			client = 0;
			job = 0;
			jobReq = 0;
			cacheIn = 0;
			cacheOut = 0;
			cacheHit = 0;
			cacheReq = 0;
		}
	};

	static Entries total;
	static Entries now;
public:
	static void timeProc();

	static void memoryRetain( ssize_t sz );
	static void memoryRelease( ssize_t sz );
	static ssize_t memoryUsed();

	static void clientRetain();
	static void clientRelease();
	static ssize_t client();
	static ssize_t connect();

	static void jobRetain();
	static void jobRelease();
	static ssize_t job();
	static ssize_t jobReq();

	static void cacheInRetain();
	static ssize_t cacheIn();

	static void cacheOutRetain();
	static ssize_t cacheOut();

	static void cacheReqRetain();
	static ssize_t cacheReq();

	static void cacheHitRetain();
	static ssize_t cacheHit();

	static void _dump( Json &dst, Entries &entry, bool isNow=false );
	static Json dump();
};



#endif /* defined(__defer_io__Status__) */
