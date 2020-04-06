#include <iostream>
#include <exception>
#include <memory>

#include "log.h"
#include "ftp_server.h"
#include "ftp_session.h"
#include "users_db.h"


FTPServer::FTPServer()
	: _ioContext(1)
	, _acceptor(_ioContext)
	, _worker()
{
	UsersDB::getInstance().loadFromFile("users-db.txt");
}

FTPServer::~FTPServer()
{

}

void FTPServer::start(const std::string& address, std::uint16_t port)
{
	tcp::endpoint ep(asio::ip::make_address(address), port);
	_acceptor.open(asio::ip::tcp::v4());
	_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
	_acceptor.bind(ep);
	_acceptor.listen();

	acceptConnections();
	_worker = std::thread(&FTPServer::worker, this);
}

void FTPServer::stop()
{
	std::error_code ec;
	_acceptor.cancel(ec);
	if (ec)
	{
		LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << "'\n";
	}

	_acceptor.close(ec);
	if (ec)
	{
		LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << "'\n";
	}

	try 
	{
		_ioContext.stop();
		_worker.join();		
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
	}
}

static const std::uint16_t FTP_DATA_CONNECTION_PORT = 10020;

void FTPServer::acceptConnections()
{
	_acceptor.async_accept(
		[this](std::error_code ec, asio::ip::tcp::socket socket)
		{
			if (!_acceptor.is_open())
			{
				return;
			}

			if (!ec)
			{
				LOG_INFO() << " - New connection accepted.\n";
				const char* homeDir = getenv("HOME");	// need to be configured
				if (homeDir != NULL)
				{
					try 
					{
						// std::make_shared<FTPSession>(std::move(socket), FTP_DATA_CONNECTION_PORT, homeDir)->start();
						std::unique_ptr<FTPSession> session = std::make_unique<FTPSession>(std::move(socket), FTP_DATA_CONNECTION_PORT, homeDir);
						_sessions.push_back(std::move(session));
						_sessions.back()->start();
					}
					catch (const std::exception& ex)
					{
						LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
					}
				}
			}
			else
			{
				LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
			}

			acceptConnections();
		});
}

void FTPServer::worker()
{
	try
	{
		_ioContext.restart();
		_ioContext.run();
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
	}
}
