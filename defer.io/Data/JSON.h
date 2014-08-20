//
//  JSON.h
//  defer.io
//
//  Created by Kevin Shin on 2014. 7. 1..
//
//

#ifndef __defer_io__JSON__
#define __defer_io__JSON__

#include "../include.h"

#include <vector>

#include <jansson.h>

/*
class JSONMem
{
	static ssize_t			used;
	static pthread_mutex_t	_lock;
public:
	static void retain( ssize_t sz );
	static void release( ssize_t sz );
	static ssize_t size();

	static inline void lock() {
		pthread_mutex_lock( &_lock );
	}

	static inline void unlock() {
		pthread_mutex_unlock( &_lock );
	}
};
*/

class Json
{
	mutable json_t		*data;
public:
	class Path;

	//static
	const static Json Null;
	const static Json Zero;
	const static Json True;
	const static Json False;

	Json();
	Json( bool );
	Json( int );
	Json( unsigned );
	Json( int64_t );
	Json( double );
	Json( const char* );
	Json( std::string& );
	Json( json_type );
	Json( json_t* );
	Json( Json const & );
	~Json();

	//encode, decode
	bool parse( const std::string& );
	std::string stringify();

	//value
	json_type type() const;
	bool isArray() const;
	bool isObject() const;
	bool isNumber() const;
	bool isReal() const;
	bool isString() const;
	bool isBoolean() const;
	bool isNull() const;
	bool hasMember( std::string& ) const;
	size_t size() const;

	//setter
	Json setObjMember( std::string&k, Json &member );
	Json setArrayMember( int index, Json &member );

	//getter
	bool asBoolean();
	int64_t asNumber();
	double asReal();
	std::string asString();

	//operators
	Json &operator=( const Json& );
	bool operator!() const;
	json_t *operator*() const;

	Json operator[]( const int );
	const Json operator[]( const int ) const;
	Json operator[]( const char* );
	const Json operator[]( const char* ) const;
	Json operator[]( const std::string& );
	const Json operator[]( const std::string& ) const;

	//modifiers
	//Global/setter
	Json getOrCreate( Json &arg, bool *changed=NULL );
	Json set( Json &arg, bool *changed=NULL );
	Json getSet( Json &arg, bool *changed=NULL );
	Json setIfNotExists( Json &arg, bool *changed=NULL );
	//Array/getter
	Json arraySlice( Json &arg );
	Json arrayCount( Json &arg );
	Json arrayRandGet( Json &arg );
	//Array/setter
	Json arrayPush( Json &arg, bool *changed=NULL );
	Json arrayPop( Json &arg, bool *changed=NULL );
	Json arrayUnshift( Json &arg, bool *changed=NULL );
	Json arrayShift( Json &arg, bool *changed=NULL );
	Json arraySplice( Json &arg, bool *changed=NULL );
	Json arrayCut( Json &arg, bool *changed=NULL );
	Json arrayRandPop( Json &arg, bool *changed=NULL );
	//List/setter
		void listUniqArg( Json &arg );
	Json listPush( Json &arg, bool *changed=NULL, bool checkUniq=false );
	Json listUnshift( Json &arg, bool *changed=NULL, bool checkUniq=false );
	//Object/getter
	Json objectKeys( Json &arg );
	//Object/setter
	Json objectSet( Json &arg, bool *changed=NULL );
	Json objectSetIfNotExists( Json &arg, bool *changed=NULL );
	Json objectDel( Json &arg, bool *changed=NULL );
	//Number/setter
	Json numberIncr( Json &arg, bool *changed=NULL );
	Json numberDecr( Json &arg, bool *changed=NULL );
	//String/getter
	Json stringLength( Json &arg );
	Json stringSub( Json &arg );
	//String/setter
	Json stringAppend( Json &arg, bool *changed=NULL );
	Json stringPrepend( Json &arg, bool *changed=NULL );
	//Boolean/setter
	Json boolToggle( Json &arg, bool *changed=NULL );

	//error
	enum Status : uint16_t {
		OK = 200,
		NotFound = 404,
		LogicError = 500,
	};

	class Error : public std::runtime_error {
		Status		_status;
	public:
		Error( const Status, const std::string& );
		Error( const Status, const char* );
		Status status() const;
	};
};

class Json::Path
{
public:
	enum Kind {
		None,
		Object,
		Array
	};

	class Argument {
	public:
		Kind			kind;
		uint32_t		idx;
		std::string		name;

		Argument( uint32_t );
		Argument( std::string );

		std::string toString();
	};
private:
	std::vector<Argument>	arguments;

	bool					_isset;
public:
	Path();
	Path( std::string& );

	Json					parent;
	Json					element;

	void parse( std::string& );
	std::string str();

	bool find( Json& );
	bool isset();

	Argument &getLastArgument();
};

#endif /* defined(__defer_io__JSON__) */
