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

Document::Status System::execute( Job *job, JSONDoc &result )
{
	Document::CMD cmd = (Document::CMD) job->header.cmd;
	switch ( cmd )
	{
		case Document::CMD::FlushCache:
			Cache::flushAll();
			break;
		default:
			result.SetBool( false );
			return Document::LogicError;
			break;
	}

	result.SetBool( true );
	return Document::OK;
}
