//
//  Client.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#include "Client.h"
#include "Job.h"
#include "ThreadPool.h"
#include "Status.h"
#include "Server.h"

Client::Client( int _sock, ev::loop_ref loop ): sock(_sock), _connected(true), authorized(0), requested(0), working(0), rio(), rbuf(), rbuf_off(0), wio(), wbuf(), wbuf_off(0)
{
	Status::clientRetain();

	fcntl( sock, F_SETFL, fcntl( sock, F_GETFL, 0 ) | O_NONBLOCK );

	rbuf.reserve( 1024 );

	rio.set<Client, &Client::read_cb>(this);
	rio.set( loop );
	rio.start( sock, ev::READ );

	wio.set<Client, &Client::write_cb>(this);
	wio.set( loop );
	wio.set( sock, ev::WRITE ); //not start
}

void Client::error()
{
	disconnect();
}

void Client::disconnect()
{
	if ( _connected )
	{
		Status::clientRelease();
		_connected = false;
		rio.stop();
		wio.stop();

		if ( sock > -1 )
		{
			close( sock );
			sock = -1;
		}
	}

	finalize();
}

void Client::finalize()
{
	if ( _connected )
		return disconnect();

	if ( working <= 0 && wbuf.length() <= 0 )
		delete this;
}

#define RECV_BUF_SIZE	(1024*16)
void Client::read_cb( ev::io &watcher, int revents )
{
	char		buf[RECV_BUF_SIZE];

	ssize_t res = ::read( sock, buf, RECV_BUF_SIZE );
	if ( res < 0 )
	{
		perror( "read error" );
		return disconnect();
	}

	if ( res == 0 )
	{
		std::cout << "disconnected\n";
		return disconnect();
	}

	rbuf.append(buf, res);
	if ( rbuf.length()-rbuf_off >= sizeof(Job::Header) )
	{
		try
		{
			Job *job = Job::parse( rbuf, &rbuf_off, this );
			while ( job != NULL )
			{
				working++;
				ThreadPool::push( job );

				job = Job::parse( rbuf, &rbuf_off, this );
			}

			if ( rbuf.length() <= rbuf_off )
			{
				rbuf.clear();
				rbuf_off = 0;
			}
		}
		catch( const std::length_error& le )
		{
			error();
		}
	}
}

void Client::writeFinish()
{
	wio.stop();
	wbuf.clear();
	wbuf_off = 0;

	if ( !_connected )
		finalize();
}

ssize_t sent = 0;
void Client::write_cb( ev::io &watcher, int revents )
{
	if ( !_connected )
		return writeFinish();

	ssize_t size = wbuf.length() - wbuf_off;
	if ( size > 0 )
	{
		ssize_t nsend = ::send( watcher.fd, &(wbuf[wbuf_off]), size, 0 );
		if ( nsend < 0 )
		{
			perror( "send error" );
			disconnect();
			return;
		}

		if ( nsend == 0 )
			return disconnect();

		sent += nsend;

		wbuf_off += nsend;
		if ( wbuf_off >= wbuf.length() )
			writeFinish();
	}
	else
		writeFinish();
}

//friend method for server
void Client::wioStart()
{
	wlock();
	working--;
	unlock();
	if ( _connected )
	{
		if ( wbuf.length() > 0 && !wio.is_active() )
			wio.start();
	}
	else
		writeFinish();
}

//public methods for thread
void Client::jobFinish()
{
	working--;
	if ( !_connected )
		finalize();
}

bool Client::response( std::string &buf )
{
	if ( _connected )
	{
		wlock();
		wbuf.append( buf );
		working++;
		unlock();

		Server::sendClient( this );
		return true;
	}
	else
		return false;
}

