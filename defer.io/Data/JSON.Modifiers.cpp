//
//  JSON.Modifiers.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 8. 20..
//
//

#include "JSON.h"

inline const Json getArg( const Json &arg, unsigned int i )
{
	if ( arg.isArray() && i < arg.size() )
		return *arg[i];

	return *Json::Null;
}

inline void assertArray( Json &root )
{
	if ( !root.isArray() )
		throw Json::Error( Json::LogicError, "The root is not an array." );
}

inline void assertObject( Json &root )
{
	if ( !root.isObject() )
		throw Json::Error( Json::LogicError, "The root is not an object." );
}

inline void assertNumber( Json &root )
{
	if ( !root.isNumber() )
		throw Json::Error( Json::LogicError, "The root is not a number." );
}

inline void assertString( Json &root )
{
	if ( !root.isString() )
		throw Json::Error( Json::LogicError, "The root is not a string." );
}

inline void assertBool( Json &root )
{
	if ( !root.isBoolean() )
		throw Json::Error( Json::LogicError, "The root is not a boolean." );
}

inline void assertArg( const bool v, std::string msg )
{
	if ( !v )
		throw Json::Error( Json::LogicError, msg );
}

#define CHANGED	{if (changed != NULL) *changed=true;}
//global
Json Json::getOrCreate( Json &arg, bool *changed )
{
	if ( isNull() && arg.isArray() && arg.size() > 0 )
	{
		json_t *cur = data;
		data = arg[0].data;

		json_incref( data );
		json_decref( cur );
		CHANGED;
	}
	return *this;
}

Json Json::set( Json &arg, bool *changed )
{
	json_t *cur = data;
	data = arg[0].data;

	json_incref( data );
	json_decref( cur );
	CHANGED;
	return *this;
}

Json Json::getSet( Json &arg, bool *changed )
{
	Json res( this );

	json_t *cur = data;
	data = arg[0].data;

	json_incref( data );
	json_decref( cur );
	CHANGED
	return *res;
}

Json Json::setIfNotExists( Json &arg, bool *changed )
{
	if ( isNull() )
	{
		json_t *cur = data;
		data = arg[0].data;

		json_incref( data );
		json_decref( cur );
		CHANGED
	}
	return *this;
}

//Array/getter
Json Json::arraySlice( Json &arg )
{
	assertArray(*this);
	assertArg(arg.size() > 0, "No arguments specified." );
	assertArg( arg[0].isNumber(), "The first argument must be an unsigned integer." );
	int64_t size = arg[0].asNumber();

	int64_t off = 0;
	if ( arg.size() > 1 )
	{
		assertArg( arg[1].isNumber(), "The second argument must be an Integer." );
		off = arg[1].asNumber();
	}

	if ( off < 0 && -off >= this->size() )
		off = 0;

	Json res( JSON_ARRAY );
	if ( off < 0 )
	{
		for ( int64_t i=this->size()+off; i < this->size() && res.size() < size; i++ )
			json_array_append( res.data, this[i].data );
	}
	else
	{
		for ( int64_t i=off; i < this->size() && res.size() < size; i++ )
			json_array_append( res.data, this[i].data );
	}
	return *res;
}

Json Json::arrayCount( Json &arg )
{
	assertArray(*this);

	Json res( (int64_t) size() );
	return *res;
}

Json Json::arrayRandGet( Json &arg )
{
	assertArray(*this);
	if ( size() <= 0 )
		return *Json::Null;

	return *(this[Util::microtime()%size()]);
}


//Array/setter
Json Json::arrayPush( Json &arg, bool *changed )
{
	assertArray(*this);
	assertArg(arg.size() > 0, "No arguments specified." );

	for ( int i=0; i < arg.size(); i++ )
		json_array_append( data, arg[i].data );

	CHANGED
	return *this;
}

Json Json::arrayPop( Json &arg, bool *changed )
{
	assertArray(*this);
	if ( size() <= 0 )
		return *Json::Null;

	size_t idx = json_array_size(data)-1;
	Json res( json_array_get(data, idx ) );
	json_array_remove( data, idx );

	CHANGED
	return *res;
}

Json Json::arrayUnshift( Json &arg, bool *changed )
{
	assertArray(*this);
	assertArg(arg.size() > 0, "No arguments specified." );

	for ( int i=(int)arg.size(); i--; )
		json_array_insert( data, 0, arg[i].data );

	CHANGED
	return *this;
}

Json Json::arrayShift( Json &arg, bool *changed )
{
	assertArray(*this);
	if ( size() <= 0 )
		return *Json::Null;

	size_t idx = 0;
	Json res( json_array_get(data, idx ) );
	json_array_remove( data, idx );
	CHANGED

	return *res;
}

Json Json::arraySplice( Json &arg, bool *changed )
{
	assertArray(*this);
	assertArg( arg.size() > 1, "No arguments specified." );

	assertArg( arg[0].isNumber(), "The first argument must be an unsigned integer." );
	assertArg( arg[1].isNumber(), "The second argument must be an unsigned integer." );

	int off = (int) arg[0].asNumber();
	int delCnt = (int) arg[1].asNumber();

	for ( int i=0; i < delCnt; i++ )
		json_array_remove( data, off+i );
	for ( int i=(int)arg.size(); i-- > 1; )
		json_array_insert( data, off, arg[i].data );

	CHANGED
	return *this;

}

Json Json::arrayCut( Json &arg, bool *changed )
{
	assertArray(*this);
	assertArg( arg.size() > 0, "No arguments specified." );

	assertArg( arg[0].isNumber(), "The first argument must be an unsigned integer." );
	long size = arg[0].asNumber();

	long off = 0;
	if ( arg.size() > 1 )
	{
		assertArg( arg[1].isNumber(), "The second argument must be an integer." );
		off = arg[1].asNumber();
	}

	if ( size == 0 )
	{
		json_array_clear( data );
		CHANGED
	}
	else if ( off == 0 )
	{
		while ( size < this->size() )
			json_array_remove( data, json_array_size(data)-1 );
		CHANGED
	}
	else if ( off < 0 )
	{
		long idx = this->size()+off;
		if ( idx < 0 )
			idx = 0;
		for ( int i=0; i < idx; i++ )
			json_array_remove( data, 0 );
		while ( size < this->size() )
			json_array_remove( data, json_array_size(data)-1 );
		CHANGED
	}
	else
	{
		assertArg( off < this->size(), "Invalid offset value." );
		for ( int i=0; i < off; i++ )
			json_array_remove( data, 0 );
		while ( size < this->size() )
			json_array_remove( data, json_array_size(data)-1 );
		CHANGED
	}

	return *this;
}

Json Json::arrayRandPop( Json &arg, bool *changed )
{
	assertArray(*this);
	if ( size() <= 0 )
		return *Json::Null;

	long idx = Util::microtime()%size();
	Json res( this[idx] );
	json_array_remove( data, idx );

	CHANGED
	return *res;
}

//List/setter
void Json::listUniqArg( Json &arg )
{
	for ( int i=1; i < arg.size(); i++ )
		assertArg( (arg[i].isString() || arg[i].isNumber()), "Value argument must be a string or number." );
	for ( int i=(int)arg.size(); i-- > 0; )
	{
		std::string v(arg[i].asString());
		bool same = false;
		for ( int p=0; p < size(); p++ )
		{
			if ( this[p].asString() == v )
			{
				same = true;
				break;
			}
		}
		if ( same )
			json_array_remove( arg.data, i );
	}
}

Json Json::listPush( Json &arg, bool *changed, bool checkUniq )
{
	assertArray(*this);
	assertArg( size() > 1, "No arguments specified." );

	assertArg( arg[0].isNumber(), "The first argument must be an unsigned integer." );
	long limit = arg[0].asNumber();
	assertArg( limit > 0, "The first argument must be greater than 0." );

	if ( checkUniq )
	{
		listUniqArg( arg );
		if ( arg.size() <= 0 )
			return *this;
	}

	for ( int i=1; i < arg.size(); i++ )
		json_array_append( data, arg[i].data );
	while ( size() > limit )
		json_array_remove( data, 0 );
	CHANGED
	return *this;
}

Json Json::listUnshift( Json &arg, bool *changed, bool checkUniq )
{
	assertArray(*this);
	assertArg( size() > 1, "No arguments specified." );

	assertArg( arg[0].isNumber(), "The first argument must be an unsigned integer." );
	long limit = arg[0].asNumber();
	assertArg( limit > 0, "The first argument must be greater than 0." );

	if ( checkUniq )
	{
		listUniqArg( arg );
		if ( arg.size() <= 0 )
			return *this;
	}

	for ( int i=1; i < arg.size(); i++ )
		json_array_insert( data, 0, arg[i].data );
	while ( size() > limit )
		json_array_remove( data, size()-1 );
	CHANGED
	return *this;
}

//Object/getter
Json Json::objectKeys( Json &arg )
{
	assertObject(*this);

	bool search = false;
	std::string	term;
	if ( arg.size() > 0 && arg[0].isString() )
	{
		search = true;
		term = arg[0].asString();
	}

	Json res( JSON_ARRAY );

	const char *k;
	json_t *v;
	json_object_foreach( data, k, v ) {
		if ( search )
		{
			if ( strlen(k) < term.length() || strncasecmp( k, term.c_str(), term.length() ) != 0 )
				continue;
		}
		json_array_append( res.data, json_string(k) );
	}
	return *res;
}

//Object/setter
Json Json::objectSet( Json &arg, bool *changed )
{
	assertObject(*this);
	assertArg( arg.size() > 0, "No arguments specified." );

	bool _changed = false;
	for ( int i=0; i < arg.size(); i+=2 )
	{
		if ( arg[i].isString() )
		{
			std::string k = arg[i].asString();
			if ( k.length() > 0 )
			{
				_changed = true;
				json_object_set( data, k.c_str(), arg[i+1].data );
			}
		}
	}
	if ( _changed )
		CHANGED
		return *this;
}

Json Json::objectSetIfNotExists( Json &arg, bool *changed )
{
	assertObject(*this);
	assertArg( arg.size() > 0, "No arguments specified." );

	bool _changed = false;
	for ( int i=0; i < arg.size(); i+=2 )
	{
		if ( arg[i].isString() )
		{
			std::string k = arg[i].asString();
			if ( k.length() > 0 && json_object_get( data, k.c_str() ) == NULL )
			{
				_changed = true;
				json_object_set( data, k.c_str(), arg[i+1].data );
			}
		}
	}
	if ( _changed )
		CHANGED
		return *this;
}

Json Json::objectDel( Json &arg, bool *changed )
{
	assertObject(*this);
	assertArg( arg.size() > 0, "No arguments specified." );

	bool _changed = false;
	for ( int i=0; i < arg.size(); i++ )
	{
		if ( arg[i].isString() )
		{
			std::string k = arg[i].asString();
			if ( json_object_get( data, k.c_str() ) != NULL )
			{
				_changed = true;
				json_object_del( data, k.c_str() );
			}
		}
	}
	if ( _changed )
		CHANGED
		return *this;
}

//Number/setter
Json Json::numberIncr( Json &arg, bool *changed )
{
	assertNumber(*this);
	double val = 1.0f;
	if ( !arg.isNull() && arg.size() > 0 )
	{
		assertArg( arg[0].isNumber(), "The first argument must be a number." );
		val = arg[0].asReal();
	}

	double v = asReal()+val;
	if ( Util::isInteger(v) )
	{
		data->type = JSON_INTEGER;
		json_integer_set( data, v );
	}
	else
	{
		data->type = JSON_REAL;
		json_real_set( data, v );
	}

	CHANGED
	return *this;
}

Json Json::numberDecr( Json &arg, bool *changed )
{
	assertNumber(*this);
	double val = 1.0f;
	if ( !arg.isNull() && arg.size() > 0 )
	{
		assertArg( arg[0].isNumber(), "The first argument must be a number." );
		val = arg[0].asReal();
	}

	double v = asReal()-val;
	if ( Util::isInteger(v) )
		json_real_set( data, v );
	else
		json_integer_set( data, v );

	CHANGED
	return *this;
}

//String/getter
Json Json::stringLength( Json &arg )
{
	assertString(*this);

	return *Json( (int) asString().length() );
}

Json Json::stringSub( Json &arg )
{
	assertString(*this);

	std::string		val = asString();
	ssize_t			off = 0;
	ssize_t			len = val.length();

	if ( arg.size() > 0 )
	{
		assertArg( arg[0].isNumber(), "The first argument must be an integer." );
		off = arg[0].asNumber();
	}
	if ( arg.size() > 1 )
	{
		assertArg( arg[1].isNumber(), "The second argument must be an unsigned integer." );
		len = arg[1].asNumber();
	}

	std::string res;
	if ( off >= 0 )
		res.assign( val.substr(MIN(off,val.length()), len) );
	else
		res.assign( val.substr(MAX(0,val.length()+off), len) );
	return *Json( res );
}

//String/setter
Json Json::stringAppend( Json &arg, bool *changed )
{
	assertString(*this);
	assertArg( arg.size() > 0 && arg[0].isString(), "The first argument must be a string." );

	std::string val = asString();
	val.append( arg[0].asString() );

	json_string_set( data, val.c_str() );
	CHANGED
	return *this;
}

Json Json::stringPrepend( Json &arg, bool *changed )
{
	assertString(*this);
	assertArg( arg.size() > 0 && arg[0].isString(), "The first argument must be a string." );

	std::string val = arg[0].asString();
	val.append( asString() );

	json_string_set( data, val.c_str() );
	CHANGED

	return *this;
}

//Boolean/setter
Json Json::boolToggle( Json &arg, bool *changed )
{
	assertBool(*this);
	data->type = data->type == JSON_TRUE ? JSON_FALSE : JSON_TRUE;
	CHANGED
	return *this;
}

