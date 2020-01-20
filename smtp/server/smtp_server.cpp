#include "smtp_server.h"
#include "log.h"

#include <csignal>


SMTPServer::SMTPServer()
	: _ioContext(1)
	, _signal(_ioContext, SIGINT, SIGTERM)
	, _acceptor(_ioContext)
{

}

SMTPServer::~SMTPServer()
{
	if (_acceptor.is_open())
	{
		stop();
	}
}


void SMTPServer::start(const std::string& addr, std::uint16_t port)
{
	tcp::endpoint ep(asio::ip::make_address(addr), port);
	_acceptor.open(asio::ip::tcp::v4());
	_acceptor.bind(ep);
	_acceptor.listen();

	waitSignal();
	acceptConnection();

	std::cout << " starting...\n";
	_ioContext.restart();
	_ioContext.run();
	std::cout << " start - finished\n";
}

void SMTPServer::stop()
{
	std::error_code ec;

	_acceptor.cancel(ec);
	if (ec)
	{
		LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	_acceptor.close(ec);
	if (ec)
	{
		LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	try
	{
		_ioContext.stop();
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception " << ex.what() << std::endl;
	}

	std::cout << " stop() - done\n";
}

void SMTPServer::acceptConnection()
{
	_acceptor.async_accept([this](std::error_code ec, tcp::socket socket)
		{
			if (!ec)
			{
				tcp::endpoint ep = socket.remote_endpoint();

				LOG_INFO() << " - Accepted connection from " 
					<< ep.address() << ':' << ep.port() << std::endl;
				std::uint32_t sessionId = _nextSessionId + 1;
				std::unique_ptr<SMTPSession> session = std::make_unique<SMTPSession>(sessionId, &_sessionsManager, std::move(socket));
				_sessionsManager.startSession(std::move(session));
				_nextSessionId = sessionId;
				// _sessions[_sessionId] = std::move(std::make_unique<SMTPSession>(_sessionId, this, std::move(socket)));
				// _sessions[_sessionId]->start();
			}
			else
			{
				LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}

			acceptConnection();
		});
}

void SMTPServer::waitSignal()
{
	_signal.async_wait([this](std::error_code ec, int signo)
		{
			if (!ec)
			{
				std::cout << " on signal " << signo << std::endl;
				stop();
			}
			else
			{
				LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}			
		});
}
