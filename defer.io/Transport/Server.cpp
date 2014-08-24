//
//  Server.cpp
//  defer.io
//
//  Created by Kevin Shin on 2014. 6. 29..
//
//

#include "Server.h"
#include "Client.h"

ev::async				Server::aio;
pthread_mutex_t			Server::_lock = PTHREAD_MUTEX_INITIALIZER;
std::queue<Client *>	Server::sendClients;

Server::Server( short _port, ev::loop_ref _loop ): io(), port(_port), sock(0), loop(_loop)
{
	struct sockaddr_in addr;

	sock = socket(PF_INET, SOCK_STREAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons( port );
	addr.sin_addr.s_addr = INADDR_ANY;

	int yes = 1;
	if ( setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int) ) == -1 || bind( sock, (struct sockaddr *)&addr, sizeof(addr) ) != 0 )
	{
		throw std::runtime_error( strerror(errno) );
	}

	fcntl( sock, F_SETFL, fcntl( sock, F_GETFL, 0 ) | O_NONBLOCK );
	listen( sock, 256 );
}

void Server::accept_cb( ev::io &watcher, int revents )
{
	if (EV_ERROR & revents)
	{
		perror( "got invalid event" );
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_sd = accept( watcher.fd, (struct sockaddr *)&client_addr, &client_len );
	if ( client_sd < 0 )
	{
		perror("accept error");
		return;
	}

	new Client( client_sd, loop );
}

int Server::start()
{
	io.set<Server, &Server::accept_cb>(this);
	io.set( loop );
	io.start( sock, ev::READ );

	aio.set<&Server::aio_cb>();
	aio.set( loop );
	aio.start();

	return 0;
}

void Server::end()
{
	io.stop();
	aio.stop();
}


//static
void Server::sendClient( Client *client )
{
	if ( client == NULL )
		return;
	lock();
	sendClients.push( client );
	aio.send();
	unlock();
}

void Server::aio_cb( ev::async &watcher, int revents )
{
	while ( !sendClients.empty() )
	{
		lock();
		Client *client = sendClients.front();
		sendClients.pop();
		unlock();

		if ( client == NULL )
			break;

		client->wioStart();
	}
}
