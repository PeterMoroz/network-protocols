#include "server.h"

#include <exception>
#include <iostream>

using namespace telnet_server;

Server::Server(const std::string& address, std::uint16_t port)
	: _ioContext(1)
	, _acceptor(_ioContext, tcp::endpoint(asio::ip::make_address(address), port))
{

}

Server::~Server()
{

}

void Server::start()
{
	try 
	{
		accept();
		_worker = std::thread(&Server::worker, this);		
	}
	catch (const std::exception& ex)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tException: " << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tUnknown exception." << std::endl;
	}	
}

void Server::stop()
{
	std::error_code ec;
	_acceptor.cancel(ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	_acceptor.close(ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	_ioContext.stop();

	try 
	{
		_worker.join();
	}
	catch (const std::exception& ex)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tException: " << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tUnknown exception." << std::endl;
	}
}


void Server::sendMessageToAll(const std::string& message)
{

}

void Server::sendMessageToClient(std::shared_ptr<Session> session, const std::string& message)
{

}

void Server::removeClient(std::shared_ptr<Session> session)
{
	_sessions.erase(session);

	tcp::socket& socket = session->getSocket();
	std::error_code ec;

	socket.cancel(ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	socket.close(ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}	
}

void Server::clearClientScreen(std::shared_ptr<Session> session)
{

}

void Server::accept()
{
	_acceptor.async_accept([this]
		(std::error_code ec, tcp::socket socket)
		{
			if (!ec)
			{
				handleConnection(std::move(socket));
				accept();
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
					<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}
		});
}

void Server::worker()
{
	try
	{
		_ioContext.restart();
		std::error_code ec;
		_ioContext.run(ec);

		if (ec)
		{
			std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
				<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tException: " << ex.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
			<< "\tUnknown exception." << std::endl;
	}
}

void Server::handleConnection(tcp::socket&& socket)
{
	tcp::endpoint ep(socket.remote_endpoint());
	std::uint32_t id = _sessions.size() + 1;
	std::shared_ptr<Session> session = std::make_shared<Session>(id, std::move(socket));
	//_sessions[session] = socket; //std::move(socket);
	_sessions.insert(session);

	_onConnected(session);

	sendBytesToClient(session, 
		{
			0xFF, 0xFD, 0x01,	// Do echo
			0xFF, 0xFD, 0x21,	// Do remote flow control
			0xFF, 0xFB, 0x01,	// Will echo
			0xFF, 0xFB, 0x03,	// Will suppress go ahead
		});
}

void Server::sendBytesToClient(std::shared_ptr<Session> session, std::vector<std::uint8_t>&& bytes)
{
	tcp::socket& socket = session->getSocket();	
	asio::async_write(socket, asio::buffer(bytes.data(), bytes.size()),
		[this, session](std::error_code ec, std::size_t sz)
		{
			if (!ec)
			{
				std::cout << "Send " << sz << " bytes - complete." << std::endl;
				receiveAsync(session);
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
					<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}

		});
}

#include <iomanip>

void Server::receiveAsync(std::shared_ptr<Session> session)
{
	tcp::socket& socket = session->getSocket();	
	socket.async_read_some(asio::buffer(_recvBuffer, RECV_BUFFER_SIZE),
		[this, session](std::error_code ec, std::size_t sz)
		{
			if (!ec)
			{
				// std::cout << "Received " << sz << " bytes." << std::endl;
				// for (std::size_t i = 0; i < sz; i++)
				// {
				// 	std::uint16_t x = _recvBuffer[i];
				// 	std::cout << std::setfill('0') << std::setw(4) 
				// 		<< std::setbase(16) << x << ' ';
				// }
				// std::cout << "-----------------------------------------\n";
				if (sz == 0)
				{
					removeClient(session);
					return;
				}

				if (_recvBuffer[0] < 0xF0)
				{
					std::string receivedData(session->getReceivedData());
					// 0x2E = '.', 0x0D is carriage return <CR>, 0x0A is new line <LF>
					if ((_recvBuffer[0] == 0x2E && _recvBuffer[1] == 0x0D && receivedData.empty())
						|| (_recvBuffer[0] == 0x0D && _recvBuffer[1] == 0x0A))
					{
						receivedData.append(_recvBuffer.cbegin(), sz);
						// send clear screen command
						_onMessageReceived(session, receivedData);
						session->clearReceivedData();
					}
					else
					{
						// 0x08 is backspace character
						if (_recvBuffer[0] == 0x08)
						{
							if (receivedData.empty())
							{
								receiveAsync(session);
							}
							else
							{
								session->removeLastReceivedCharacter();
								sendBytesToClient(session, { 0x08, 0x20, 0x08 });
							}
						}
						else if (_recvBuffer[0] == 0x7F)
						{
							// 0x7F is delete character
							receiveAsync(session);
						}
						else
						{
							session->appendReceivedData(_recvBuffer.cbegin(), sz);
							// if client is not typing password - echo back the received character
							// othrewise - echo back astericks
							if (session->getAuthStatus() != AuthStatus::Authenticating)
							{
								sendBytesToClient(session, std::vector<std::uint8_t>(1, _recvBuffer[0]));
								// sendBytesToClient(session, std::vector<std::uint8_t>(1, 
								// 				static_cast<std::uint8_t>(_recvBuffer[0])));
							}
							else
							{
								sendBytesToClient(session, { '*' });
							}
							receiveAsync(session);
						}
					}
				}
				else
				{
					receiveAsync(session);
				}
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__ << '\t' << __FUNCTION__
					<< "\tError: " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}
		});
}
