//
//  Document.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 21..
//
//

#include "Document.h"
#include "Cache.h"

Document::Document( const Key &k ): key(k), content(), bucket(NULL), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
}

Document::Document( const Key &k, const std::string &v ): key(k), content(), bucket(NULL), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
	setData( v );
}

Document::Document( DB::VBucket *b, const Key &k ): key(k), content(), bucket(b), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
{
}

Document::Document( DB::VBucket *b, const Key &k, const std::string &v ): key(k), content(), bucket(b), _created(false), _changed(false), _timeChanged(0), _out(), _timeOutGenerated(0)
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
	JSON::parse( v, content );
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

bool Document::changed()
{
	return _changed;
}

void Document::touch()
{
	if ( !_changed )
	{
		_changed = true;
		DB::changed();
	}
	_timeChanged = Util::microtime();
}

void Document::created()
{
	_created = true;
}

rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &Document::allocator()
{
	return content.allocator;
}

std::string Document::out()
{
	if ( _timeOutGenerated == 0 || _timeOutGenerated < _timeChanged )
	{
		_out.assign( JSON::stringify( content ) );
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

	std::string	v;
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
		doc = new Document( bucket, k, v );
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
		case Document::ArrayPush:
		case Document::ArrayUnshift:
		case Document::ArraySplice:
		case Document::ListPush:
		case Document::ListUnshift:
		case Document::ListPushUniq:
		case Document::ListUnshiftUniq:
		case Document::ObjectSet:
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

Document::Status Document::execute( const std::string &expr, CMD cmd, JSONDoc &arg, JSONDoc &res )
{
	if ( expr == "." )
		return manipulate( content, cmd, arg, res );

	//TODO: subpath
	/*
	Json::Path	path( expr );
	if ( path.resolve(content).isNull() )
	{
		if ( isReadonlyCmd(cmd) )
			return cmd == CMD::Exists ? JSON::False : JSON::Null;

		if ( !isNeedParentSetterCmd(cmd) )
		{
			if ( cmd == ArrayCount || cmd == StringLength )
				return JSON(0);
			return JSON::Null;
		}

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
					throw Error( 500, "The root is not a object." );

				if ( !parent->isMember(key) )
				{
					switch ( cmd )
					{
						case GetOrCreate: case Set: case GetSet:
							(*parent)[key] = Json::Value(Json::ValueType::nullValue);
							break;
						case ArrayPush: case ArrayUnshift: case ArraySplice:
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
							assert(false);
							return JSON::Null;
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
					throw Error( 500, "Invalid array index." );
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
					throw Error( 500, "The root is not a object." );

				if ( !parent->isValidIndex(idx) )
				{
					switch ( cmd )
					{
						case GetOrCreate: case Set: case GetSet:
							(*parent)[idx] = Json::Value(Json::ValueType::nullValue);
							break;
						case ArrayPush: case ArrayUnshift: case ArraySplice:
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
							assert(false);
							return JSON::Null;
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
	 */
	return OK;
}

Document::Status Document::manipulate( JSONDoc &sub, CMD cmd, JSONDoc &arg, JSONDoc &res )
{
	Status result = OK;
	if ( cmd == CMD::Exists )
	{
		res.SetBool( !sub.IsNull() );
		return result;
	}

	if ( isReadonlyCmd(cmd) )
	{
		if ( sub.IsNull() )
		{
			res.SetNull();
			return result;
		}

		try
		{
			switch ( cmd )
			{
				case Sync:
					res.SetBool( save() );
					break;
				case Get:
					res.CopyFrom( sub, allocator() );
					break;
				case ArraySlice:
					result = arraySlice( sub, arg, res );
					break;
				case ArrayCount:
					result = arrayCount( sub, arg, res );
					break;
				case StringLength:
					result = stringLength( sub, arg, res );
					break;
				case StringSub:
					result = stringSub( sub, arg, res );
					break;
				case ObjectKeys:
					result = objectKeys( sub, arg, res );
				default:
					break;
			}
		}
		catch ( const Error le )
		{
			throw le;
		}
	}
	else
	{
		try
		{
			switch ( cmd )
			{
				case GetOrCreate:
					result = getOrCreate( sub, arg, res );
					break;
				case Set:
					result = set( sub, arg, res );
					break;
				case GetSet:
					result = getSet( sub, arg, res );
					break;

				//array
				case ArrayPush:
					result = arrayPush( sub, arg, res );
					break;
				case ArrayPop:
					result = arrayPop( sub, arg, res );
					break;
				case ArrayUnshift:
					result = arrayUnshift( sub, arg, res );
					break;
				case ArrayShift:
					result = arrayShift( sub, arg, res );
					break;
				case ArraySplice:
					result = arraySplice( sub, arg, res );
					break;
				case ArrayCut:
					result = arrayCut( sub, arg, res );
					break;
				case ArrayRandGet:
					result = arrayRandGet( sub, arg, res );
					break;
				case ArrayRandPop:
					result = arrayRandPop( sub, arg, res );
					break;

				//list
				case ListPush:
					result = listPush( sub, arg, res );
					break;
				case ListUnshift:
					result = listUnshift( sub, arg, res );
					break;
				case ListPushUniq:
					result = listPush( sub, arg, res, true );
					break;
				case ListUnshiftUniq:
					result = listUnshift( sub, arg, res, true );
					break;

				//object
				case ObjectSet:
					result = objectSet( sub, arg, res );
					break;
				case ObjectDel:
					result = objectDel( sub, arg, res );
					break;

				//number
				case NumberIncr:
					result = numberIncr( sub, arg, res );
					break;
				case NumberDecr:
					result = numberDecr( sub, arg, res );
					break;

				//string
				case StringAppend:
					result = stringAppend( sub, arg, res );
					break;
				case StringPrepend:
					result = stringPrepend( sub, arg, res );
					break;

				//boolean
				case BoolToggle:
					result = boolToggle( sub, arg, res );
					break;

				default:
					break;
			}
		}
		catch ( const Error le )
		{
			throw le;
		}
	}

	return OK;
}

inline const JSONVal &getArg( const JSONDoc &arg, int i )
{
	if ( arg.IsArray() && i < arg.Size() )
		return arg[i];

	return JSON::Null;
}

//private
inline void assertArray( JSONDoc &root )
{
	if ( !root.IsArray() )
		throw Document::Error( Document::LogicError, "The root is not an array." );
}

inline void assertObject( JSONDoc &root )
{
	if ( !root.IsObject() )
		throw Document::Error( Document::LogicError, "The root is not an object." );
}

inline void assertNumber( JSONDoc &root )
{
	if ( !root.IsNumber() )
		throw Document::Error( Document::LogicError, "The root is not a number." );
}

inline void assertString( JSONDoc &root )
{
	if ( !root.IsString() )
		throw Document::Error( Document::LogicError, "The root is not a string." );
}

inline void assertBool( JSONDoc &root )
{
	if ( !root.IsBool() )
		throw Document::Error( Document::LogicError, "The root is not a boolean." );
}

inline void assertArg( const bool v, std::string msg )
{
	if ( !v )
		throw Document::Error( Document::LogicError, msg );
}

Document::Status Document::getOrCreate( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	if ( root.IsNull() && arg.IsArray() && arg.Size() > 0 )
	{
		root.CopyFrom( getArg( arg, 0 ), allocator() );
		touch();
	}
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::set( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	root.CopyFrom( getArg( arg, 0 ), allocator() );
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::setIfNotExists( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	if ( root.IsNull() )
	{
		root.CopyFrom( getArg( arg, 0 ), allocator() );
		touch();
	}
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::getSet( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	res.CopyFrom( root, allocator() );
	root.CopyFrom( getArg( arg, 0 ), allocator() );
	touch();

	return OK;
}

//array
Document::Status Document::arrayPush( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	assertArg(arg.Size() > 0, "No arguments specified." );

	for ( unsigned int i=0; i < arg.Size(); i++ )
	{
		root.PushBack( arg[i], allocator() );
	}
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::arrayPop( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	if ( root.Size() <= 0 )
	{
		res.SetNull();
		return OK;
	}

	res.CopyFrom( *(root.End()), allocator() );
	root.PopBack();

	touch();

	return OK;
}

Document::Status Document::arrayUnshift( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	assertArg(arg.Size() > 0, "No arguments specified." );

	JSONVal newArray( rapidjson::Type::kArrayType );
	newArray.Reserve( arg.Size()+root.Size(), allocator() );
	for ( int i=0; i < arg.Size(); i++ )
		newArray.PushBack( arg[i], allocator() );

	for ( int i=0; i < root.Size(); i++ )
		newArray.PushBack( root[i], allocator() );

	root.Swap( newArray );
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::arrayShift( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	if ( root.Size() <= 0 )
	{
		res.SetNull();
		return OK;
	}

	res.CopyFrom( root[0u], allocator() );
	JSONVal newArray( rapidjson::Type::kArrayType );
	newArray.Reserve( root.Size()-1, allocator() );

	for ( int i=1; i < root.Size(); i++ )
		newArray.PushBack( root[i], allocator() );

	root.Swap(newArray);
	touch();

	return OK;
}

Document::Status Document::arraySplice( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	assertArg(arg.Size() > 1, "No arguments specified." );

	assertArg( arg[0u].IsNumber(), "The first argument must be an unsigned integer." );
	assertArg( arg[1u].IsNumber(), "The second argument must be an unsigned integer." );

	int off = arg[0u].GetInt();
	int delCnt = arg[1u].GetInt();

	JSONVal newArray( rapidjson::Type::kArrayType );
	for ( int i=0; i < root.Size() && i < off; i++ )
		newArray.PushBack( root[i], allocator() );
	for ( int i=2; i < arg.Size(); i++ )
		newArray.PushBack( arg[i], allocator() );
	for ( int i=off+delCnt; i < root.Size(); i++ )
		newArray.PushBack( root[i], allocator() );

	root.Swap(newArray);
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::arrayCut( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	assertArg(arg.Size() > 0, "No arguments specified." );

	assertArg( arg[0u].IsUint(), "The first argument must be an unsigned integer." );
	unsigned int size = arg[0u].GetUint();

	int off = 0;
	if ( arg.Size() > 1 )
	{
		assertArg( arg[1u].IsInt(), "The second argument must be an integer." );
		off = arg[1u].GetInt();
	}

	if ( off < 0 && -off >= root.Size() )
		off = 0;

	if ( off == 0 )
	{
		if ( size < root.Size() )
		{
			while ( size < root.Size() )
				root.PopBack();
			touch();
		}
	}
	else if ( off < 0 )
	{
		JSONVal newArray( rapidjson::Type::kArrayType );
		for ( unsigned int i=root.Size()+off; i < root.Size() && newArray.Size() < size; i++ )
			newArray.PushBack( root[i], allocator() );
		root.Swap( newArray );
		touch();
	}
	else
	{
		assertArg( off < root.Size(), "Invalid offset value." );
		JSONVal newArray( rapidjson::Type::kArrayType );
		for ( unsigned int i=off; i < root.Size() && newArray.Size() < size; i++ )
			newArray.PushBack( root[i], allocator() );
		root.Swap( newArray );
		touch();
	}
	res.CopyFrom( root, allocator() );
	return OK;
}

Document::Status Document::arraySlice( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	assertArg(arg.Size() > 0, "No arguments specified." );
	assertArg( arg[0u].IsUint(), "The first argument must be an unsigned integer." );
	unsigned int size = arg[0u].GetUint();

	int off = 0;
	if ( arg.Size() > 1 )
	{
		assertArg( arg[1u].IsInt(), "The second argument must be an Integer." );
		off = arg[1u].GetInt();
	}

	if ( off < 0 && -off >= root.Size() )
		off = 0;

	res.SetArray();
	if ( off < 0 )
	{
		for ( unsigned int i=root.Size()+off; i < root.Size() && res.Size() < size; i++ )
			res.PushBack( root[i], allocator() );
	}
	else
	{
		for ( unsigned int i=off; i < root.Size() && res.Size() < size; i++ )
			res.PushBack( root[i], allocator() );
	}
	return OK;
}

Document::Status Document::arrayCount( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	res.SetUint( root.Size() );

	return OK;
}

Document::Status Document::arrayRandGet( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	if ( root.Size() <= 0 )
	{
		res.SetNull();
		return OK;
	}

	res.CopyFrom( root[Util::microtime()%root.Size()], allocator() );
	return OK;
}

Document::Status Document::arrayRandPop( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertArray(root);
	if ( root.Size() <= 0 )
	{
		res.SetNull();
		return OK;
	}

	unsigned idx = Util::microtime()%root.Size();
	res.CopyFrom( root[idx], allocator() );

	JSONVal newArray( rapidjson::Type::kArrayType );
	newArray.Reserve( root.Size()-1, allocator() );
	for ( unsigned int i=0; i < idx; i++ )
		newArray.PushBack( root[i], allocator() );
	for ( unsigned int i=idx+1; i < root.Size(); i++ )
		newArray.PushBack( root[i], allocator() );

	root.Swap(newArray);
	touch();

	return OK;

}


//list
void Document::listUniqArg( JSONDoc &root, JSONDoc &arg, JSONVal &res )
{
	res.SetArray();
	for ( unsigned int i=1; i < arg.Size(); i++ )
		assertArg( (arg[i].IsString() || arg[i].IsNumber()), "Value argument must be a string or number." );
	for ( unsigned int i=1; i < arg.Size(); i++ )
	{
		std::string v(arg[i].GetString());
		bool same = false;
		for ( unsigned int p=0; p < root.Size(); p++ )
		{
			if ( root[p].GetString() == v )
			{
				same = true;
				break;
			}
		}
		if ( !same )
			res.PushBack( arg[i], allocator() );
	}
}

Document::Status Document::listPush( JSONDoc &root, JSONDoc &arg, JSONDoc &res, bool checkUniq )
{
	assertArray(root);
	assertArg(arg.Size() > 1, "No arguments specified." );

	assertArg( arg[0u].IsUint(), "The first argument must be an unsigned integer." );
	unsigned int limit = arg[0u].GetUint();
	assertArg( limit > 0, "The first argument must be greater than 0." );

	JSONVal args;
	if ( checkUniq )
	{
		listUniqArg( root, arg, args );
		if ( args.Size() <= 0 )
		{
			res.CopyFrom( root, allocator() );
			return OK;
		}
	}
	else
		args.Swap( arg );

	if ( root.Size()+args.Size()-1 <= limit )
	{
		for ( unsigned int i=1; i < args.Size(); i++ )
			root.PushBack( args[i], allocator() );
	}
	else if ( args.Size()-1 >= limit )
	{
		JSONVal newArray( rapidjson::Type::kArrayType );
		for ( unsigned int i=args.Size()-limit; i < args.Size() && newArray.Size() < limit; i++ )
			newArray.PushBack( args[i], allocator() );
		root.Swap( newArray );
	}
	else
	{
		JSONVal newArray( rapidjson::Type::kArrayType );
		for ( unsigned int i=(root.Size()+args.Size()-1)-limit; i < root.Size(); i++ )
			newArray.PushBack( root[i], allocator() );
		for ( unsigned int i=1; i < args.Size(); i++ )
			newArray.PushBack( args[i], allocator() );
		root.Swap( newArray );
	}
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::listUnshift( JSONDoc &root, JSONDoc &arg, JSONDoc &res, bool checkUniq )
{
	assertArray(root);
	assertArg(arg.Size() > 1, "No arguments specified." );

	assertArg( arg[0u].IsUint(), "The first argument must be an unsigned integer." );
	unsigned int limit = arg[0u].GetUint();
	assertArg( limit > 0, "The first argument must be greater than 0." );

	JSONVal args;
	if ( checkUniq )
	{
		listUniqArg( root, arg, args );
		if ( args.Size() <= 0 )
		{
			res.CopyFrom( root, allocator() );
			return OK;
		}
	}
	else
		args.Swap( arg );

	JSONVal newArray( rapidjson::Type::kArrayType );
	for ( unsigned int i=1; i < args.Size() && newArray.Size() < limit; i++ )
		newArray.PushBack( args[i], allocator() );
	for ( unsigned int i=0; i < root.Size() && newArray.Size() < limit; i++ )
		newArray.PushBack( root[i], allocator() );

	root.Swap( newArray );
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

//object
Document::Status Document::objectSet( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertObject(root);
	assertArg( arg.Size() > 0, "No arguments specified." );

	bool changed = false;
	for ( int i=0; i < arg.Size(); i+=2 )
	{
		if ( arg[i].IsString() )
		{
			changed = true;
			if ( root.HasMember( arg[i] ) )
				root.RemoveMember( arg[i] );
			root.AddMember( arg[i], arg[i+1], allocator() );
		}
	}
	if ( changed )
		touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::objectSetIfNotExists( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertObject(root);
	assertArg( arg.Size() > 0, "No arguments specified." );

	bool changed = false;
	for ( int i=0; i < arg.Size(); i+=2 )
	{
		if ( arg[i].IsString() )
		{
			changed = true;
			if ( !root.HasMember( arg[i] ) )
				root.AddMember( arg[i], arg[i+1], allocator() );
		}
	}
	if ( changed )
		touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::objectDel( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertObject(root);
	assertArg( arg.Size() > 0, "No arguments specified." );

	bool changed = false;
	for ( int i=0; i < arg.Size(); i++ )
	{
		if ( arg[i].IsString() )
		{
			if ( root.HasMember( arg[i] ) )
			{
				changed = true;
				root.RemoveMember( arg[i] );
			}
		}
	}
	if ( changed )
		touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::objectKeys( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertObject(root);

	bool		search = false;
	std::string	term;
	if ( arg.Size() > 0 && arg[0u].IsString() )
	{
		search = true;
		term = arg[0u].GetString();
	}

	res.SetArray();
	for ( rapidjson::Value::MemberIterator it = root.MemberBegin(); it != root.MemberEnd(); ++it )
	{
		if ( search )
		{
			if ( (*it).name.GetStringLength() < term.length() || std::string( (*it).name.GetString() ).find( term ) == std::string::npos )
				continue;
		}
		res.PushBack( (*it).name, allocator() );
	}

	return OK;
}

//number
Document::Status Document::numberIncr( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertNumber(root);

	double val = 1.0f;
	if ( arg.Size() > 0 )
	{
		assertArg( arg[0u].IsNumber(), "The first argument must be a number." );
		val = arg[0u].GetDouble();
	}

	double v = root.GetDouble()+val;
	if ( Util::isInteger(v) )
	{
		root.SetInt64( v );
	}
	else
	{
		root.SetDouble( v );
	}
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::numberDecr( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertNumber(root);

	double val = 1.0f;
	if ( arg.Size() > 0 )
	{
		assertArg( arg[0u].IsNumber(), "The first argument must be a number." );
		val = arg[0u].GetDouble();
	}

	double v = root.GetDouble()-val;
	if ( Util::isInteger(v) )
	{
		root.SetInt64( v );
	}
	else
	{
		root.SetDouble( v );
	}
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

//string
Document::Status Document::stringAppend( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertString(root);
	assertArg( arg.Size() > 0 && arg[0u].IsString(), "The first argument must be a string." );

	std::string val( root.GetString() );
	val.append( arg[0u].GetString() );

	root.SetString( val.c_str(), allocator() );
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::stringPrepend( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertString(root);
	assertArg( arg.Size() > 0 && arg[0u].IsString(), "The first argument must be a string." );

	std::string val( arg[0u].GetString() );
	val.append( root.GetString() );

	root.SetString( val.c_str(), allocator() );
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}

Document::Status Document::stringLength( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertString(root);
	res.SetUint( root.GetStringLength() );

	return OK;
}

Document::Status Document::stringSub( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertString(root);

	std::string		val = root.GetString();
	ssize_t			off = 0;
	ssize_t			len = val.length();

	if ( arg.Size() > 0 )
	{
		assertArg( arg[0u].IsInt(), "The first argument must be an integer." );
		off = arg[0u].GetInt();
	}
	if ( arg.Size() > 1 )
	{
		assertArg( arg[1u].IsUint(), "The second argument must be an unsigned integer." );
		len = arg[1u].GetUint();
	}

	if ( off >= 0 )
		res.SetString( val.substr(MIN(off,val.length()), len).c_str(), allocator() );
	else
		res.SetString( val.substr(MAX(0,val.length()+off), len).c_str(), allocator() );
	return OK;
}

//boolean
Document::Status Document::boolToggle( JSONDoc &root, JSONDoc &arg, JSONDoc &res )
{
	assertBool(root);

	root.SetBool( !root.GetBool() );
	touch();
	res.CopyFrom( root, allocator() );

	return OK;
}




//exception
Document::Error::Error( const Status st, const std::string &err ): std::runtime_error(err), _status(st)
{
}

Document::Error::Error( const Status st, const char *err ): std::runtime_error(err), _status(st)
{
}

Document::Status Document::Error::status() const
{
	return _status;
}
