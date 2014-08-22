//
//  Document.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "Document.h"
#include "Cache.h"

Document::Document( const Key &k, uint64_t timeSave ): key(k), content(), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0), _timeSave(timeSave), bucket(NULL)
{
}

Document::Document( const Key &k, const std::string &v, uint64_t timeSave ): key(k), content(), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0), _timeSave(timeSave), bucket(NULL)
{
	setData( v );
}

Document::Document( DB::VBucket *b, const Key &k, uint64_t timeSave ): key(k), content(), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0), _timeSave(timeSave), bucket(b)
{
}

Document::Document( DB::VBucket *b, const Key &k, const std::string &v, uint64_t timeSave ): key(k), content(), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0), _timeSave(timeSave), bucket(b)
{
	setData( v );
}

Document::~Document()
{
}



void Document::setBucket( DB::VBucket *b )
{
	bucket = b;
}

void Document::setData( const std::string &v )
{
	content.parse( v );
}

Key &Document::getKey()
{
	return key;
}

bool Document::save()
{
	if ( !bucket )
		return false;

	if ( _changed )
	{
		std::string buf;
		buf.assign( (char *) &_timeSave, sizeof(uint64_t) );
		buf.append( out() );
		if ( bucket->set( key, buf ) )
		{
			_changed = false;
			return true;
		}
		return false;
	}
	return true;
}

bool Document::changed()
{
	return _changed;
}

void Document::touch()
{
	_timeChanged = Util::microtime();
	setTimeSave( _timeChanged );

	DB::touched( this );
	if ( !_changed )
	{
		_changed = true;
		DB::changed();
	}
}


uint64_t Document::timeSave()
{
	return _timeSave;
}

void Document::setTimeSave( uint64_t v )
{
	_timeSave = v;
}

void Document::created()
{
	_created = true;
}

std::string Document::out()
{
	if ( _timeOutGenerated == 0 || _timeOutGenerated < _timeChanged )
	{
		_out.assign( content.stringify() );
		_timeOutGenerated = Util::microtime();
	}
	return _out;
}

Document *Document::getOrCreate( const Key &k )
{
	Cache		*cache = Cache::getCachePool( k );
	uint64_t	seq = cache->seq;

	k.wlock();
	Document *doc = cache->get( k );
	if ( doc != NULL )
	{
		doc->wlock();
		k.unlock();
		return doc;
	}

	DB::VBucket	*bucket = DB::getBucket( k );
	if ( bucket == NULL )
	{
		k.unlock();
		return NULL;
	}

	if ( seq != cache->seq )
	{
		//check cache again
		doc = cache->get( k );
		if ( doc != NULL )
		{
			doc->wlock();
			k.unlock();
			return doc;
		}
	}

	std::string	v;
	bool res = false;
	try
	{
		res = bucket->get( k, &v );
	}
	catch ( const std::logic_error &le )
	{
		//TODO: log
		k.unlock();
		return NULL;
	}

	if ( res )
	{
		uint64_t timeSave = *((uint64_t *) v.c_str());
		doc = new Document( bucket, k, v.substr(sizeof(uint64_t)), timeSave );
	}
	else
	{
		//create
		doc = new Document( bucket, k );
		doc->created();
	}
	cache->push( k, doc );

	doc->wlock();
	k.unlock();

	return doc;
}

inline bool isReadonlyCmd( Document::CMD cmd )
{
	switch ( cmd )
	{
		case Document::Sync:
		case Document::Exists:
		case Document::Get:
		case Document::ArraySlice:
		case Document::ArrayCount:
		case Document::ArrayRandGet:
		case Document::StringLength:
		case Document::StringSub:
		case Document::ObjectKeys:
			return true;

		default:
			return false;
			break;
	}
}

inline bool isNeedParentSetterCmd( Document::CMD cmd )
{
	switch ( cmd )
	{
		case Document::Set:
		case Document::GetSet:
		case Document::SetIfNotExists:
		case Document::ArrayPush:
		case Document::ArrayUnshift:
		case Document::ArraySplice:
		case Document::ListPush:
		case Document::ListUnshift:
		case Document::ListPushUniq:
		case Document::ListUnshiftUniq:
		case Document::ObjectSet:
		case Document::ObjectSetIfNotExists:
		case Document::NumberIncr:
		case Document::NumberDecr:
		case Document::StringAppend:
		case Document::StringPrepend:
		case Document::BoolToggle:
			return true;
			break;

		default:
			return false;
			break;
	}
}

Json Document::execute( Json::Path &path, CMD cmd, Json &arg )
{
	if ( path.isset() )
	{
		if ( !path.find( content ) )
			throw Json::Error( Json::LogicError, "Cannot find sub object." );

		//DEBUGS( JSON::stringify( *path.parent ) );
		if ( !!path.element )
			return manipulate( path.element, cmd, arg );
		else if ( !!path.parent )
		{
			if ( isReadonlyCmd(cmd) )
			{
				if ( cmd == CMD::Exists )
					return Json::False;
				else
					return Json::Null;
			}

			if ( !isNeedParentSetterCmd(cmd) )
			{
				if ( cmd == ArrayCount || cmd == StringLength )
					return Json::Zero;
				else
					return Json::Null;
			}

			Json::Path::Argument pathArg = path.getLastArgument();
			Json val;
			switch ( cmd )
			{
				case GetOrCreate: case Set: case GetSet:
					break;
				case ArrayPush: case ArrayUnshift: case ArraySplice:
				case ListPush: case ListUnshift: case ListPushUniq: case ListUnshiftUniq:
					val = Json( JSON_ARRAY );
					break;
				case ObjectSet: case ObjectDel:
					val = Json( JSON_OBJECT );
					break;
				case NumberIncr: case NumberDecr:
					val = Json( 0 );
					break;
				case StringAppend: case StringPrepend:
					val = Json( "" );
					break;
				case BoolToggle:
					val = Json( false );
					break;
				default:
					break;
			}

			if ( pathArg.kind == Json::Path::Kind::Object )
			{
				if ( path.parent.isObject() )
					path.element = path.parent.setObjMember( pathArg.name, val );
				else
					throw Json::Error( Json::LogicError, "Parent element is not object." );
			}
			else if ( pathArg.kind == Json::Path::Kind::Array )
			{
				if ( path.parent.isArray() )
					path.element = path.parent.setArrayMember( pathArg.idx, val );
				else
					throw Json::Error( Json::LogicError, "Parent element is not array." );
			}
			else
				throw Json::Error( Json::LogicError, "Runtime error." );
			if ( !!path.element )
				return manipulate( path.element, cmd, arg );
			else
				throw Json::Error( Json::LogicError, "Cannot find sub object." );
		}
		else
			throw Json::Error( Json::LogicError, "Runtime error." );
	}
	else
		return manipulate( content, cmd, arg );
}

Json Document::manipulate( Json &sub, CMD cmd, Json &arg )
{
	if ( cmd == CMD::Exists )
	{
		return !sub ? Json::False : Json::True;
	}

	if ( isReadonlyCmd(cmd) )
	{
		if ( !sub )
			return Json::Null;

		try
		{
			switch ( cmd )
			{
				case Sync:
					return save() ? *Json::True : *Json::False;
					break;
				case Get:
					return sub;
					break;
				case ArraySlice:
					return sub.arraySlice( arg );
					break;
				case ArrayCount:
					return sub.arrayCount( arg );
					break;
				case StringLength:
					return sub.stringLength( arg );
					break;
				case StringSub:
					return sub.stringSub( arg );
					break;
				case ObjectKeys:
					return sub.objectKeys( arg );
				default:
					break;
			}
		}
		catch ( const Json::Error le )
		{
			throw le;
		}
	}
	else
	{
		try
		{
			Json res;
			bool changed = false;
			switch ( cmd )
			{
				case GetOrCreate:
					res = sub.getOrCreate( arg, &changed );
					break;
				case Set:
					res = sub.set( arg, &changed );
					break;
				case GetSet:
					res = sub.getSet( arg, &changed );
					break;
				case SetIfNotExists:
					res = sub.setIfNotExists( arg, &changed );
					break;

				//array
				case ArrayPush:
					res = sub.arrayPush( arg, &changed );
					break;
				case ArrayPop:
					res = sub.arrayPop( arg, &changed );
					break;
				case ArrayUnshift:
					res = sub.arrayUnshift( arg, &changed );
					break;
				case ArrayShift:
					res = sub.arrayShift( arg, &changed );
					break;
				case ArraySplice:
					res = sub.arraySplice( arg, &changed );
					break;
				case ArrayCut:
					res = sub.arrayCut( arg, &changed );
					break;
				case ArrayRandGet:
					res = sub.arrayRandGet( arg );
					break;
				case ArrayRandPop:
					res = sub.arrayRandPop( arg, &changed );
					break;

				//list
				case ListPush:
					res = sub.listPush( arg, &changed );
					break;
				case ListUnshift:
					res = sub.listUnshift( arg, &changed );
					break;
				case ListPushUniq:
					res = sub.listPush( arg, &changed, true );
					break;
				case ListUnshiftUniq:
					res = sub.listUnshift( arg, &changed, true );
					break;

				//object
				case ObjectSet:
					res = sub.objectSet( arg, &changed );
					break;
				case ObjectDel:
					res = sub.objectDel( arg, &changed );
					break;

				//number
				case NumberIncr:
					res = sub.numberIncr( arg, &changed );
					break;
				case NumberDecr:
					res = sub.numberDecr( arg, &changed );
					break;

				//string
				case StringAppend:
					res = sub.stringAppend( arg, &changed );
					break;
				case StringPrepend:
					res = sub.stringPrepend( arg, &changed );
					break;

				//boolean
				case BoolToggle:
					res = sub.boolToggle( arg, &changed );
					break;

				default:
					break;
			}
			if ( changed )
				touch();
			return res;
		}
		catch ( const Json::Error le )
		{
			throw le;
		}
	}

	return Json::Null;
}
