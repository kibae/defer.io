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
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>

#ifndef MAX
#define MAX(a,b)		((a) > (b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)		((a) > (b)?(b):(a))
#endif
#define AVG(a,b)		(((a)+(b))/2)

#define LLOG			{std::cout << (uint64_t) pthread_self() << "\t" << basename((char *)__FILE__) << "\t" << __func__ << "(line: " << (int) __LINE__ << ")\n";fflush(stdout);}
#define DEBUGS(x)		std::cout << (uint64_t) pthread_self() << "\t" << basename((char *)__FILE__) << "\t" << __func__ << "(line: " << (int) __LINE__ << ")	" << (x) << "\n";

#endif
