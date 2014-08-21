//
//  System.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 18..
//
//

#include "System.h"
#include "Document.h"
#include "Cache.h"
#include "Status.h"

Json System::execute( Job *job )
{
	Document::CMD cmd = (Document::CMD) job->header.cmd;
	switch ( cmd )
	{
		case Document::CMD::FlushCache:
			Cache::flushAll();
			break;
		case Document::CMD::Status:
			return *Status::dump();
		default:
			return *Json::False;
			break;
	}

	return *Json::True;
}
