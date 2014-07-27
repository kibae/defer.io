//
//  Document.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "Document.h"
#include "Cache.h"

std::vector<Document *> Document::destroyPool;

Document::Document( const Key &k ): key(k), content(), bucket(NULL), _created(false), _destroyed(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
}

Document::Document( const Key &k, const std::string &v ): key(k), content(v), bucket(NULL), _created(false), _destroyed(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
}

Document::Document( DB::VBucket *b, const Key &k ): key(k), content(), bucket(b), _created(false), _destroyed(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
}

Document::Document( DB::VBucket *b, const Key &k, const std::string &v ): key(k), content(v), bucket(b), _created(false), _destroyed(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
}




void Document::setBucket( DB::VBucket *b )
{
	bucket = b;
}

void Document::setData( const std::string &v )
{
	content.parse(v);
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
		if ( bucket->set( key, out() ) )
		{
			_changed = false;
			return true;
		}
		return false;
	}
	return true;
}

void Document::destroy()
{
	_changed = false;
	_destroyed = true;
	_timeChanged = Util::microtime();
	destroyPool.push_back( this );
}

bool Document::changed()
{
	return _changed;
}

void Document::touch()
{
	if ( _destroyed )
		throw std::runtime_error( "This document instance has been destroyed." );
	_changed = true;
	_timeChanged = Util::microtime();
}

void Document::created()
{
	_created = true;
}

std::string Document::out()
{
	if ( _timeOutGenerated == 0 || _timeOutGenerated < _timeChanged )
	{
		_out = content.toString();
		_timeOutGenerated = Util::microtime();
	}
	return _out;
}

Document *Document::get( const Key &k )
{
	uint64_t	seq = Cache::seq;

	Document *doc = Cache::get( k );
	if ( doc != NULL )
		return doc;
	
	DB::VBucket		*bucket = DB::getBucket( k );
	if ( bucket == NULL )
		return NULL;

	std::string	v;

	k.rlock();
	if ( seq != Cache::seq )
	{
		//check cache again
		doc = Cache::get( k );
		if ( doc != NULL )
		{
			k.unlock();
			return doc;
		}
	}

	bool res = bucket->get( k, &v );
	if ( !res )
	{
		k.unlock();
		return NULL;
	}

	doc = new Document( bucket, k, v );
	Cache::push( k, doc );

	k.unlock();

	return doc;
}

Document *Document::getOrCreate( const Key &k )
{
	uint64_t	seq = Cache::seq;

	Document *doc = Cache::get( k );
	if ( doc != NULL )
		return doc;

	DB::VBucket		*bucket = DB::getBucket( k );
	if ( bucket == NULL )
		return NULL;

	std::string	v;

	k.wlock();
	if ( seq != Cache::seq )
	{
		//check cache again
		doc = Cache::get( k );
		if ( doc != NULL )
		{
			k.unlock();
			return doc;
		}
	}

	bool res = bucket->get( k, &v );
	if ( res )
		doc = new Document( bucket, k, v );
	else
	{
		//create
		doc = new Document( bucket, k );
		doc->created();
	}
	Cache::push( k, doc );

	k.unlock();

	return doc;
}

Document *Document::set( const Key &k, const std::string &v )
{
	k.wlock();
	Document *doc = Cache::get( k );
	if ( doc != NULL )
	{
		doc->setData( v );
		doc->touch();
		k.unlock();
		return doc;
	}

	DB::VBucket		*bucket = DB::getBucket( k );
	if ( bucket == NULL )
	{
		k.unlock();
		return NULL;
	}

	doc = new Document( bucket, k, v );
	doc->touch();
	Cache::push( k, doc );

	k.unlock();

	return doc;
}

inline bool isReadonlyCmd( Document::CMD cmd )
{
	switch ( cmd )
	{
		case Document::Exists:
		case Document::Get:
		case Document::ArraySlice:
		case Document::ArrayCount:
		case Document::StringLength:
		case Document::ObjectKeys:
			return true;

		default:
			return false;
			break;
	}
}

JSON Document::execute( const std::string &expr, CMD cmd, const std::string arg )
{
	return execute( expr, cmd, JSON(arg) );
}

JSON Document::execute( const std::string &expr, CMD cmd, const JSON &arg )
{
	if ( expr == "." )
		return manipulate( content, cmd, arg );

	Json::Path	path( expr );
	if ( path.resolve(content).isNull() )
	{
		if ( isReadonlyCmd(cmd) )
			return cmd == CMD::Exists ? JSON::False : JSON::Null;

		std::string tmp(expr);
		ssize_t offO = tmp.rfind( "." );
		ssize_t offA = tmp.rfind( "[" );
		if ( offO >= 0 || offA >= 0 )
		{
			if ( offO > offA )
			{
				if ( offO+1 >= tmp.length() )
					return JSON::Null;

				std::string parentPath( tmp.substr(0, offO) );
				std::string key( tmp.substr(offO+1) );
				Json::Value *parent = NULL;
				if ( parentPath.length() <= 0 || parentPath == "." )
					parent = &content;
				else
				{
					Json::Path p(parentPath);
					if ( !p.resolve(content).isNull() )
						parent = &(p.make(content));
				}

				if ( parent == NULL || !parent->isObject() )
					throw std::runtime_error( "The root is not a object." );

				if ( !parent->isMember(key) )
				{
					switch ( cmd )
					{
						case Set:
							(*parent)[key] = Json::Value(Json::ValueType::nullValue);
							break;
						case ArrayPush: case ArrayPop: case ArrayUnshift: case ArrayShift: case ArrayCut:
						case ListPush: case ListUnshift: case ListPushUniq: case ListUnshiftUniq:
							(*parent)[key] = Json::Value(Json::ValueType::arrayValue);
							break;
						case ObjectSet: case ObjectDel:
							(*parent)[key] = Json::Value(Json::ValueType::objectValue);
							break;
						case NumberIncr: case NumberDecr:
							(*parent)[key] = Json::Value(0);
							break;
						case StringAppend: case StringPrepend:
							(*parent)[key] = Json::Value("");
							break;
						case BoolToggle:
							(*parent)[key] = Json::Value(false);
							break;
						default:
							break;
					}
				}
			}
			else
			{
				if ( offA+2 >= tmp.length() )
					return JSON::Null;

				std::string parentPath( tmp.substr(0, offA) );
				std::string key( tmp.substr(offA+1) );
				int idx = atoi( key.c_str() );
				if ( idx < 0 )
					throw std::runtime_error( "Invalid array index." );
				Json::Value *parent = NULL;
				if ( parentPath.length() <= 0 || parentPath == "." )
					parent = &content;
				else
				{
					Json::Path p(parentPath);
					if ( !p.resolve(content).isNull() )
						parent = &(p.make(content));
				}

				if ( parent == NULL || !parent->isArray() )
					throw std::runtime_error( "The root is not a object." );

				if ( !parent->isValidIndex(idx) )
				{
					switch ( cmd )
					{
						case Set:
							(*parent)[idx] = Json::Value(Json::ValueType::nullValue);
							break;
						case ArrayPush: case ArrayPop: case ArrayUnshift: case ArrayShift: case ArrayCut:
						case ListPush: case ListUnshift: case ListPushUniq: case ListUnshiftUniq:
							(*parent)[idx] = Json::Value(Json::ValueType::arrayValue);
							break;
						case ObjectSet: case ObjectDel:
							(*parent)[idx] = Json::Value(Json::ValueType::objectValue);
							break;
						case NumberIncr: case NumberDecr:
							(*parent)[idx] = Json::Value(0);
							break;
						case StringAppend: case StringPrepend:
							(*parent)[idx] = Json::Value("");
							break;
						case BoolToggle:
							(*parent)[idx] = Json::Value(false);
							break;
						default:
							break;
					}
				}
			}
			return manipulate( path.make(content), cmd, arg );
		}
		else
			return JSON::Null;
	}
	else
		return manipulate( path.make(content), cmd, arg );
}

JSON Document::manipulate( Json::Value &sub, CMD cmd, const JSON &arg )
{
	if ( cmd == CMD::Exists )
	{
		return sub.isNull() ? JSON::False : JSON::True;
	}

	JSON	result = JSON::Null;
	if ( isReadonlyCmd(cmd) )
	{
		if ( sub.isNull() )
			return JSON::Null;
		
		rlock();
		switch ( cmd )
		{
			case Get:
				result = &sub;
				break;
			case ArraySlice:
				result = arraySlice( sub, arg );
				break;
			case ArrayCount:
				result = arrayCount( sub, arg );
				break;
			case StringLength:
				result = stringLength( sub, arg );
				break;
			case ObjectKeys:
				result = objectKeys( sub, arg );
			default:
				break;
		}
	}
	else
	{
		wlock();
		switch ( cmd )
		{
			case Set:
				result = set( sub, arg );
				break;
			case GetSet:
				result = getSet( sub, arg );
				break;

			//array
			case ArrayPush:
				result = arrayPush( sub, arg );
				break;
			case ArrayPop:
				result = arrayPop( sub, arg );
				break;
			case ArrayUnshift:
				result = arrayUnshift( sub, arg );
				break;
			case ArrayShift:
				result = arrayShift( sub, arg );
				break;
			case ArraySplice:
				result = arraySplice( sub, arg );
				break;
			case ArrayCut:
				result = arrayCut( sub, arg );
				break;

			//list
			case ListPush:
				result = listPush( sub, arg );
				break;
			case ListUnshift:
				result = listUnshift( sub, arg );
				break;
			case ListPushUniq:
				result = listPush( sub, arg, true );
				break;
			case ListUnshiftUniq:
				result = listUnshift( sub, arg, true );
				break;

			//object
			case ObjectSet:
				result = objectSet( sub, arg );
				break;
			case ObjectDel:
				result = objectDel( sub, arg );
				break;

			//number
			case NumberIncr:
				result = numberIncr( sub, arg );
				break;
			case NumberDecr:
				result = numberDecr( sub, arg );
				break;

			//string
			case StringAppend:
				result = stringAppend( sub, arg );
				break;
			case StringPrepend:
				result = stringPrepend( sub, arg );
				break;

			//boolean
			case BoolToggle:
				result = boolToggle( sub, arg );
				break;

			default:
				break;
		}
	}
	unlock();

	return result;
}

inline const JSON getArg( const JSON &arg, int i )
{
	if ( arg.isArray() && arg.isValidIndex(i) )
		return arg[i];

	return JSON::Null;
}

//private
inline void assertArray( Json::Value &root )
{
	if ( !root.isArray() )
		throw std::runtime_error( "The root is not an array." );
}

inline void assertObject( Json::Value &root )
{
	if ( !root.isObject() )
		throw std::runtime_error( "The root is not an object." );
}

inline void assertNumber( Json::Value &root )
{
	if ( !root.isConvertibleTo(Json::ValueType::realValue) )
		throw std::runtime_error( "The root is not a number." );
}

inline void assertString( Json::Value &root )
{
	if ( !root.isConvertibleTo(Json::ValueType::stringValue) )
		throw std::runtime_error( "The root is not a string." );
}

inline void assertBool( Json::Value &root )
{
	if ( !root.isConvertibleTo(Json::ValueType::booleanValue) )
		throw std::runtime_error( "The root is not a boolean." );
}

inline void assertArg( bool v, std::string msg )
{
	if ( !v )
		throw std::runtime_error( msg );
}

JSON Document::set( Json::Value &root, const JSON &arg )
{
	touch();

	root = getArg( arg, 0 );
	return root;
}

JSON Document::getSet( Json::Value &root, const JSON &arg )
{
	touch();

	JSON old(root);
	root = getArg( arg, 0 );
	
	return old;
}

//array
JSON Document::arrayPush( Json::Value &root, const JSON &arg )
{
	assertArray(root);
	assertArg(arg.size() > 0, "No arguments specified." );

	for ( unsigned int i=0; i < arg.size(); i++ )
	{
		root.append( arg[i] );
	}
	touch();
	return root;
}

JSON Document::arrayPop( Json::Value &root, const JSON &arg )
{
	assertArray(root);
	if ( root.size() <= 0 )
		return JSON::Null;

	Json::ArrayIndex idx = root.size()-1;
	Json::Value val = root[idx];
	touch();
	root.resize( idx );

	return val;
}

JSON Document::arrayUnshift( Json::Value &root, const JSON &arg )
{
	assertArray(root);
	assertArg(arg.size() > 0, "No arguments specified." );

	Json::Value newArray(arg);
	for ( unsigned int i=0; i < root.size(); i++ )
		newArray.append( root[i] );
	touch();
	root.swap(newArray);

	return root;
}

JSON Document::arrayShift( Json::Value &root, const JSON &arg )
{
	assertArray(root);
	if ( root.size() <= 0 )
		return JSON::Null;

	Json::Value val = JSON(root[0]);
	Json::Value newArray(Json::ValueType::arrayValue);

	for ( unsigned int i=1; i < root.size(); i++ )
		newArray.append( root[i] );

	touch();
	root.swap(newArray);

	return val;
}

JSON Document::arraySplice( Json::Value &root, const JSON &arg )
{
	assertArray(root);

	JSON oOff = getArg( arg, 0 );
	JSON oDelCnt = getArg( arg, 1 );

	assertArg( oOff.isConvertibleTo(Json::ValueType::uintValue), "The first argument must be an Unsigned integer." );
	assertArg( oDelCnt.isConvertibleTo(Json::ValueType::uintValue), "The second argument must be an Unsigned integer." );

	unsigned int off = oOff.asUInt();
	unsigned int delCnt = oDelCnt.asUInt();

	Json::Value newArray(Json::ValueType::arrayValue);
	for ( unsigned int i=0; i < root.size() && i < off; i++ )
		newArray.append( root[i] );
	for ( unsigned int i=2; i < arg.size(); i++ )
		newArray.append( arg[i] );
	for ( unsigned int i=off+delCnt; i < root.size(); i++ )
		newArray.append( root[i] );

	touch();
	root.swap(newArray);

	return root;
}

JSON Document::arrayCut( Json::Value &root, const JSON &arg )
{
	assertArray(root);
	JSON oSize = getArg( arg, 0 );
	JSON oOff = getArg( arg, 1 );

	assertArg( oSize.isConvertibleTo(Json::ValueType::uintValue), "The first argument must be an Unsigned integer." );

	unsigned int size = oSize.asUInt();
	int off = 0;
	if ( arg.size() > 1 )
	{
		assertArg( oOff.isConvertibleTo(Json::ValueType::intValue), "The second argument must be an Integer." );
		off = oOff.asInt();
	}

	if ( off < 0 && -off >= root.size() )
		off = 0;

	if ( off == 0 )
	{
		if ( size < root.size() )
		{
			touch();
			root.resize( size );
		}
	}
	else if ( off < 0 )
	{
		Json::Value newArray(Json::ValueType::arrayValue);
		for ( unsigned int i=root.size()+off; i < root.size() && newArray.size() < size; i++ )
			newArray.append( root[i] );
		touch();
		root.swap( newArray );
	}
	else
	{
		assertArg( root.isValidIndex(off), "Invalid offset value." );
		Json::Value newArray(Json::ValueType::arrayValue);
		for ( unsigned int i=off; i < root.size() && newArray.size() < size; i++ )
			newArray.append( root[i] );
		touch();
		root.swap( newArray );
	}
	return root;
}

JSON Document::arraySlice( Json::Value &root, const JSON &arg )
{
	assertArray(root);
	JSON oSize = getArg( arg, 0 );
	JSON oOff = getArg( arg, 1 );

	assertArg( oSize.isConvertibleTo(Json::ValueType::uintValue), "The first argument must be an Unsigned integer." );

	unsigned int size = oSize.asUInt();
	int off = 0;
	if ( arg.size() > 1 )
	{
		assertArg( oOff.isConvertibleTo(Json::ValueType::intValue), "The second argument must be an Integer." );
		off = oOff.asInt();
	}

	if ( off < 0 && -off >= root.size() )
		off = 0;

	Json::Value newArray(Json::ValueType::arrayValue);
	if ( off < 0 )
	{
		for ( unsigned int i=root.size()+off; i < root.size() && newArray.size() < size; i++ )
			newArray.append( root[i] );
	}
	else
	{
		for ( unsigned int i=off; i < root.size() && newArray.size() < size; i++ )
			newArray.append( root[i] );
	}
	return newArray;
}

JSON Document::arrayCount( Json::Value &root, const JSON &arg )
{
	assertArray(root);

	return JSON((Json::UInt) root.size());
}

//list
JSON Document::listUniqArg( Json::Value &root, const JSON &arg )
{
	JSON args( Json::ValueType::arrayValue );
	for ( unsigned int i=1; i < arg.size(); i++ )
		assertArg( (arg[i].isConvertibleTo(Json::ValueType::stringValue) || arg[i].isConvertibleTo(Json::ValueType::realValue)), "Value argument must be a string or number." );
	for ( unsigned int i=1; i < arg.size(); i++ )
	{
		std::string v(arg[i].asString());
		bool same = false;
		for ( unsigned int p=0; p < root.size(); p++ )
		{
			if ( root[p].asString() == v )
			{
				same = true;
				break;
			}
		}
		if ( !same )
			args.append( arg[i] );
	}
	return args;
}

JSON Document::listPush( Json::Value &root, const JSON &arg, bool checkUniq )
{
	assertArray(root);
	JSON oLimit = getArg( arg, 0 );
	assertArg( oLimit.isConvertibleTo(Json::ValueType::uintValue), "The first argument must be an Unsigned integer." );
	unsigned int limit = oLimit.asUInt();
	assertArg( limit > 0, "The first argument must be greater than 0." );
	assertArg( arg.size() > 1, "No arguments specified." );

	JSON args;
	if ( checkUniq )
	{
		args = listUniqArg( root, arg );
		if ( args.size() <= 0 )
			return root;
	}
	else
		args.swap((Json::Value &) arg);

	touch();
	if ( root.size()+args.size()-1 <= limit )
	{
		for ( unsigned int i=1; i < args.size(); i++ )
			root.append( args[i] );
	}
	else if ( args.size()-1 >= limit )
	{
		Json::Value newArray(Json::ValueType::arrayValue);
		for ( unsigned int i=1; i < args.size() && newArray.size() < limit; i++ )
			newArray.append( args[i] );
		root.swap( newArray );
	}
	else
	{
		Json::Value newArray(Json::ValueType::arrayValue);
		for ( unsigned int i=root.size()-(limit-args.size()-1); i < root.size(); i++ )
			newArray.append( root[i] );
		for ( unsigned int i=1; i < args.size(); i++ )
			newArray.append( args[i] );
		root.swap( newArray );
	}

	return root;
}

JSON Document::listUnshift( Json::Value &root, const JSON &arg, bool checkUniq )
{
	assertArray(root);
	JSON oLimit = getArg( arg, 0 );
	assertArg( oLimit.isConvertibleTo(Json::ValueType::uintValue), "The first argument must be an Unsigned integer." );
	unsigned int limit = oLimit.asUInt();
	assertArg( limit > 0, "The first argument must be greater than 0." );
	assertArg( arg.size() > 1, "No arguments specified." );

	JSON args;
	if ( checkUniq )
	{
		args = listUniqArg( root, arg );
		if ( args.size() <= 0 )
			return root;
	}
	else
		args.swap((Json::Value &) arg);

	Json::Value newArray(Json::ValueType::arrayValue);
	for ( unsigned int i=1; i < args.size() && newArray.size() < limit; i++ )
		newArray.append( args[i] );
	for ( unsigned int i=0; i < root.size() && newArray.size() < limit; i++ )
		newArray.append( root[i] );

	touch();
	root.swap( newArray );

	return root;
}

//object
JSON Document::objectSet( Json::Value &root, const JSON &arg )
{
	assertObject(root);
	assertArg( arg.size() > 1, "No arguments specified." );

	touch();
	for ( int i=0; i < arg.size(); i+=2 )
	{
		if ( arg[i].isString() && arg[i].isConvertibleTo(Json::ValueType::stringValue) )
			root[arg[i].asString()] = getArg( arg, i+1 );
	}
	return root;
}

JSON Document::objectDel( Json::Value &root, const JSON &arg )
{
	assertObject(root);

	bool changed = false;
	for ( int i=0; i < arg.size(); i++ )
	{
		if ( arg[i].isString() && arg[i].isConvertibleTo(Json::ValueType::stringValue) && root.isMember( arg[i].asString() ) )
		{
			if ( !changed ) changed = true;
			root.removeMember( arg[i].asString() );
		}
	}
	if ( changed )
		touch();

	return root;
}

JSON Document::objectKeys( Json::Value &root, const JSON &arg )
{
	assertObject(root);

	JSON result(Json::ValueType::arrayValue);
	std::vector<std::string> members = content.getMemberNames();
	for ( size_t i=0; i < members.size(); i++ )
		result.append( members[i] );

	return result;
}

//number
JSON Document::numberIncr( Json::Value &root, const JSON &arg )
{
	assertNumber(root);

	double val = 1.0f;
	if ( arg.size() > 0 )
	{
		assertArg( arg[0].isConvertibleTo(Json::ValueType::realValue), "The first argument must be a number." );
		val = arg[0].asDouble();
	}

	Json::Value newVal( root.asDouble()+val );
	touch();
	root.swap( newVal );

	return root;
}

JSON Document::numberDecr( Json::Value &root, const JSON &arg )
{
	assertNumber(root);

	double val = 1.0f;
	if ( arg.size() > 0 )
	{
		assertArg( arg[0].isConvertibleTo(Json::ValueType::realValue), "The first argument must be a number." );
		val = arg[0].asDouble();
	}

	Json::Value newVal( root.asDouble()-val );
	touch();
	root.swap( newVal );

	return root;
}

//string
JSON Document::stringAppend( Json::Value &root, const JSON &arg )
{
	assertString(root);
	assertArg( arg.size() > 0 && arg[0].isConvertibleTo(Json::ValueType::stringValue), "The first argument must be a string." );

	std::string val( root.asString() );
	val.append( arg[0].asString() );

	touch();
	Json::Value newVal(val);
	root.swap(newVal);

	return root;
}

JSON Document::stringPrepend( Json::Value &root, const JSON &arg )
{
	assertString(root);
	assertArg( arg.size() > 0 && arg[0].isConvertibleTo(Json::ValueType::stringValue), "The first argument must be a string." );

	std::string val( arg[0].asString() );
	val.append( root.asString() );

	touch();
	Json::Value newVal(val);
	root.swap(newVal);

	return root;
}

JSON Document::stringLength( Json::Value &root, const JSON &arg )
{
	assertString(root);

	return JSON((Json::UInt) root.asString().length());
}

//boolean
JSON Document::boolToggle( Json::Value &root, const JSON &arg )
{
	assertBool(root);

	Json::Value newVal(!root.asBool());
	touch();
	root.swap(newVal);

	return root;
}
