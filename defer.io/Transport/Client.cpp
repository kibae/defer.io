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

Client::Client( int _sock, ev::loop_ref loop ): sock(_sock), connected(true), requested(0), working(0), rio(), rbuf(), rbuf_off(0), wio(), wbuf(), wbuf_off(0)
{
	fcntl( sock, F_SETFL, fcntl( sock, F_GETFL, 0 ) | O_NONBLOCK );

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
	if ( connected )
	{
		connected = false;
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
	if ( working <= 0 )
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
	if ( rbuf.length()-rbuf_off > sizeof(Job::Header) )
	{
		try
		{
			Job *job = Job::parse( rbuf, &rbuf_off );
			while ( job != NULL )
			{
				working++;
				job->client = this;
				ThreadPool::push( job );
				job = Job::parse( rbuf, &rbuf_off );
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

void Client::jobFinish( Job *job )
{
	if ( working > 0 )
		working--;
	if ( !connected )
	{
		return finalize();
	}

	wbuf.append( job->result );
	if ( !wio.is_active() )
	{
		wio.start();
	}
}

void Client::jobFinish( std::string *buf )
{
	if ( working > 0 )
		working--;
	if ( !connected )
	{
		return finalize();
	}

	wbuf.append( buf->c_str(), buf->length() );
	if ( !wio.is_active() )
	{
		wio.start();
	}
}

void Client::writeFinish()
{
	wio.stop();
	wbuf.clear();
	wbuf_off = 0;
}

ssize_t sent = 0;
void Client::write_cb( ev::io &watcher, int revents )
{
	if ( !connected )
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

