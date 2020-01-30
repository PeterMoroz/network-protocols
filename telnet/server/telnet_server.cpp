#include <csignal>

#include <functional>
#include <iostream>


#include "telnet_server.h"


TelnetServer::TelnetServer()
	: _ioContext(1)
	, _signalSet(_ioContext, SIGINT, SIGTERM)
	, _acceptor(_ioContext)
{

}

TelnetServer::~TelnetServer()
{

}

void TelnetServer::start(const std::string& address, std::uint16_t port)
{
	tcp::endpoint ep(asio::ip::make_address(address), port);
	_acceptor.open(asio::ip::tcp::v4());
	_acceptor.bind(ep);
	_acceptor.listen();

	waitSignal();
	acceptConnection();

	_ioContext.restart();
	_ioContext.run();
}

void TelnetServer::acceptConnection()
{
	_acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket)
		{
			if (!_acceptor.is_open())
			{
				return;
			}

			if (!ec)
			{
				try
				{
					std::cout << "Accepted connection from " 
						<< socket.remote_endpoint().address().to_string() << ':'
						<< socket.remote_endpoint().port() << std::endl;

					std::unique_ptr<TelnetSession> session = std::make_unique<TelnetSession>(std::move(socket));

					if (!_prompt.empty())
					{
						session->setPrompt(_prompt);
					}

					if (_connectedCB)
					{
						session->setConnectedCB(_connectedCB);
					}

					if (_lineCompleteCB)
					{
						session->setLineCompleteCB(_lineCompleteCB);
					}

					using namespace std::placeholders;
					session->setShutdownCB(std::bind(&TelnetServer::sessionFinished, this, _1));

					_sessions.push_back(std::move(session));
					_sessions.back()->start();
				}
				catch (const std::exception& ex)
				{
					std::cerr << __FILE__ << ':' << __LINE__ 
						<< " - Exception: " << ex.what() << std::endl;
				}
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__ << " - Error "
					<< ec.message() << '(' << ec.value() << ')' << std::endl;
			}

			acceptConnection();
		});
}

void TelnetServer::waitSignal()
{
	_signalSet.async_wait([this](std::error_code ec, int signo)
		{
			if (!ec)
			{
				std::cout << "Signal #" << signo << std::endl;
				shutdown();
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__ << " - Error "
					<< ec.message() << '(' << ec.value() << ')' << std::endl;
			}
		});
}

void TelnetServer::sessionFinished(TelnetSession* session)
{

}

void TelnetServer::shutdown()
{
	std::error_code ec;

	_acceptor.cancel(ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << " - Error: "
			<< ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	_acceptor.close(ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << " - Error: "
			<< ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	// TO DO: shutdown all sessions

	try
	{
		_ioContext.stop();
	}
	catch (const std::exception& ex)
	{
		std::cerr << __FILE__ << ':' << __LINE__ 
			<< " - Exception: " << ex.what() << std::endl;
	}
}
