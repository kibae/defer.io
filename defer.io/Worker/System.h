//
//  System.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 18..
//
//

#ifndef __defer_io__System__
#define __defer_io__System__

#include "../include.h"
#include "Job.h"
#include "Document.h"
#include "JSON.h"

class System {
public:
	static Json execute( Job*, Json& );
};

#endif /* defined(__defer_io__System__) */
