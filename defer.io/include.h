//
//  include.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 30..
//
//

#ifndef defer_io_include_h
#define defer_io_include_h

#include <iostream>
#include <sstream>
#include <algorithm>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef MAX
#define MAX(a,b)		((a) > (b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)		((a) > (b)?(b):(a))
#endif
#define AVG(a,b)		(((a)+(b))/2)

#define LLOG			{std::cout << (uint64_t) pthread_self() << "\t" << basename((char *)__FILE__) << "\t" << __func__ << "(line: " << (int) __LINE__ << ")\n";fflush(stdout);}
#define DEBUGS(x)		std::cout << (uint64_t) pthread_self() << "\t" << basename((char *)__FILE__) << "\t" << __func__ << "(line: " << (int) __LINE__ << ")	" << (x) << "\n";


class Util {
public:
	static off_t fsize( int fd ) {
		struct stat sbuf;
		if ( fstat(fd, &sbuf) != 0 )
			return -1;
		return sbuf.st_size;
	}

	static uint64_t microtime() {
		struct timeval	now;
		gettimeofday( &now, NULL );

		return (((uint64_t)now.tv_sec) * (uint64_t)1000000) + ((uint64_t)now.tv_usec);
	}
};

class RWLock
{
protected:
	pthread_rwlock_t	_lock;
public:
	inline void rlock() {
		pthread_rwlock_rdlock( &_lock );
	}

	inline void wlock() {
		pthread_rwlock_wrlock( &_lock );
	}

	inline void unlock() {
		pthread_rwlock_unlock( &_lock );
	}

	RWLock():_lock(PTHREAD_RWLOCK_INITIALIZER) {}
};

#include "Config.h"

#endif
