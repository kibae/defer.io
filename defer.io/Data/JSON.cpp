//
//  JSON.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#include "JSON.h"

const Json Json::Null;
const Json Json::Zero(0);
const Json Json::True(true);
const Json Json::False(false);

/*
ssize_t JSONMem::used = 0;
pthread_mutex_t	JSONMem::_lock = PTHREAD_MUTEX_INITIALIZER;

void JSONMem::retain( ssize_t sz )
{
	lock();
	used += sz;
	unlock();
}

void JSONMem::release( ssize_t sz )
{
	lock();
	used -= sz;
	unlock();
}

ssize_t JSONMem::size()
{
	return used;
}

JSON::Doc::Doc(): doc()
{
}

JSON::Doc::~Doc()
{
	JSONMem::release( doc.GetAllocator().Capacity() );
}
*/

Json::Json(): data(json_null())
{
}

Json::Json( bool v ): data(json_boolean(v))
{
}

Json::Json( int v ): data(json_integer(v))
{
}

Json::Json( unsigned v ): data(json_integer(v))
{
}

Json::Json( int64_t v ): data(json_integer(v))
{
}

Json::Json( double v ): data(json_real(v))
{
}

Json::Json( const char *v ): data(json_string(v))
{
}

Json::Json( std::string &v ): data(json_string(v.c_str()))
{
}

Json::Json( json_type v ): data(NULL)
{
	switch ( v )
	{
		case JSON_ARRAY:	data = json_array();		break;
		case JSON_OBJECT:	data = json_object();		break;
		case JSON_FALSE:	data = json_boolean(false);	break;
		case JSON_TRUE:		data = json_boolean(true);	break;
		case JSON_INTEGER:	data = json_integer(0);		break;
		case JSON_REAL:		data = json_real(0);		break;
		case JSON_STRING:	data = json_string("");		break;
		default:			data = json_null();			break;
	}
}

Json::Json( json_t *rhd ): data(rhd)
{
	if ( data == NULL )
		data = json_null();
	else
		json_incref( data );
}

Json::Json( Json const &rhd ): data(rhd.data)
{
	if ( data == NULL )
		data = json_null();
	else
		json_incref( data );
}

Json::~Json()
{
	json_decref( data );
}

json_type Json::type() const
{
	return json_typeof( data );
}

bool Json::isArray() const
{
	return json_is_array( data );
}

bool Json::isObject() const
{
	return json_is_object( data );
}

bool Json::isNumber() const
{
	return json_is_number( data );
}

bool Json::isReal() const
{
	return json_is_real( data );
}

bool Json::isString() const
{
	return json_is_string( data );
}

bool Json::isBoolean() const
{
	return json_is_true( data ) || json_is_false( data );
}

bool Json::isNull() const
{
	return json_is_null( data );
}

bool Json::asBoolean()
{
	switch ( type() )
	{
		case JSON_TRUE:		return true;	break;
		case JSON_NULL:
		case JSON_FALSE:	return false;	break;
		case JSON_REAL:		return json_real_value(data) != 0.0;	break;
		case JSON_INTEGER:	return json_integer_value(data) != 0;	break;
		case JSON_ARRAY:	return json_array_size(data) > 0;		break;
		case JSON_OBJECT:	return json_object_size(data) > 0;		break;
		case JSON_STRING:	return strlen(json_string_value(data)) > 0;	break;
	}
}

int64_t Json::asNumber()
{
	if ( type() == JSON_INTEGER )
		return json_integer_value( data );
	else if ( type() == JSON_REAL )
		return json_real_value( data );
	else if ( type() == JSON_STRING )
	{
		const char *str = json_string_value( data );
		if ( str != NULL )
			return atoll( str );
	}
	return 0;
}

double Json::asReal()
{
	if ( type() == JSON_INTEGER )
		return json_integer_value( data );
	else if ( type() == JSON_REAL )
		return json_real_value( data );
	else if ( type() == JSON_STRING )
	{
		const char *str = json_string_value( data );
		if ( str != NULL )
			return atof( str );
	}
	return 0;
}

std::string Json::asString()
{
	std::stringstream	buf;

	if ( type() == JSON_INTEGER )
		buf << json_integer_value( data );
	else if ( type() == JSON_REAL )
		buf << json_real_value( data );
	else if ( type() == JSON_TRUE )
		buf << "True";
	else if ( type() == JSON_FALSE )
		buf << "False";
	else if ( type() == JSON_NULL )
		buf << "Null";
	else if ( type() == JSON_STRING )
	{
		const char *str = json_string_value( data );
		if ( str != NULL )
			buf << str;
	}

	return buf.str();
}

bool Json::hasMember( std::string &k ) const
{
	if ( json_typeof(data) == JSON_OBJECT )
	{
		json_t *member = json_object_get( data, k.c_str() );
		if ( member == NULL )
			return false;
		else
			return true;
	}
	else
		return false;
}

size_t Json::size() const
{
	switch ( json_typeof(data) )
	{
		case JSON_ARRAY:	return json_array_size( data );				break;
		case JSON_OBJECT:	return json_object_size( data );			break;
		case JSON_STRING:	return strlen( json_string_value(data) );	break;
		default:			return 0;									break;
	}
}

//setter
Json Json::setObjMember( std::string&k, Json &member )
{
	json_object_set( data, k.c_str(), member.data );
	return json_object_get( data, k.c_str() );
}

Json Json::setArrayMember( int index, Json &member )
{
	while ( index >= json_array_size( data ) )
		json_array_append( data, json_null() );
	json_array_set( data, index, member.data );
	return json_array_get( data, index );
}


Json &Json::operator=( const Json &other )
{
	json_t *cur = data;
	data = other.data;

	json_incref( data );
	json_decref( cur );
	return *this;
}

bool Json::operator!() const
{
	return json_is_null(data);
}

json_t *Json::operator*() const
{
	return data;
}

Json Json::operator[]( const int index )
{
	if ( json_typeof(data) == JSON_ARRAY )
	{
		json_t *member = json_array_get( data, index );
		if ( member != NULL )
			return member;
	}
	return *Json::Null;
}
const Json Json::operator[]( const int index ) const
{
	if ( json_typeof(data) == JSON_ARRAY )
	{
		json_t *member = json_array_get( data, index );
		if ( member != NULL )
			return member;
	}
	return *Json::Null;
}

Json Json::operator[]( const char *k )
{
	if ( json_typeof(data) == JSON_OBJECT )
	{
		json_t *member = json_object_get( data, k );
		if ( member != NULL )
			return member;
	}
	return *Json::Null;
}
const Json Json::operator[]( const char *k ) const
{
	if ( json_typeof(data) == JSON_OBJECT )
	{
		json_t *member = json_object_get( data, k );
		if ( member != NULL )
			return member;
	}
	return *Json::Null;
}

Json Json::operator[]( const std::string &k )
{
	return *((*this)[k.c_str()]);
}
const Json Json::operator[]( const std::string &k ) const
{
	return *((*this)[k.c_str()]);
}


//encode, decode
bool Json::parse( const std::string &buf )
{
	json_error_t err;
	json_t *obj = json_loads( buf.c_str(), JSON_DECODE_ANY, &err );
	if ( obj != NULL )
	{
		data = obj;
		return true;
	}

	json_decref( data );
	data = json_null();
	return false;
}

std::string Json::stringify()
{
	char *buf = json_dumps( data, JSON_COMPACT | JSON_ENCODE_ANY );
	if ( buf != NULL )
	{
		std::string res( buf );
		free( buf );
		return res;
	}
	else
	{
		LLOG
		return "";
	}
}

Json::Error::Error( const Status st, const std::string &err ): std::runtime_error(err), _status(st)
{
}

Json::Error::Error( const Status st, const char *err ): std::runtime_error(err), _status(st)
{
}

Json::Status Json::Error::status() const
{
	return _status;
}

