#include <cassert>

#include <algorithm>
#include <iostream>

#include "telnet_session.h"


TelnetSession::TelnetSession(asio::ip::tcp::socket&& socket)
	: _socket(std::move(socket))
{
}


TelnetSession::~TelnetSession()
{

}

void TelnetSession::start()
{
	// const std::uint8_t willEcho[3] = { 0xFF, 0xFB, 0x01 };
	const std::uint8_t willEcho[3] = { IAC, WILL, 0x01 };
	asio::async_write(_socket, asio::const_buffer(willEcho, 3),
		[this](std::error_code ec, std::size_t sz)
		{
			if (!ec)
			{
				// const std::uint8_t dontEcho[3] = { 0xFF, 0xFE, 0x01 };
				const std::uint8_t dontEcho[3] = { IAC, DONT, 0x01 };
				asio::async_write(_socket, asio::const_buffer(dontEcho, 3),
					[this](std::error_code ec, std::size_t sz)
					{
						if (!ec)
						{
							// const std::uint8_t willSuppressGoAhead[3] = { 0xFF, 0xFB, 0x03 };
							const std::uint8_t willSuppressGoAhead[3] = { IAC, WILL, 0x03 };
							asio::async_write(_socket, asio::const_buffer(willSuppressGoAhead, 3),
								[this](std::error_code ec, std::size_t sz)
								{
									if (!ec)
									{
										if (_connectedCB)
										{
											_connectedCB(this);
										}

										receive();
									}
									else
									{
										std::cerr << __FILE__ << ':' << __LINE__
											<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
										// TO DO: shutdown session
									}
								});
						}
						else
						{
							std::cerr << __FILE__ << ':' << __LINE__
								<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
							// TO DO: shutdown session
						}

					});
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__
					<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
				// TO DO: shutdown session
			}
		});
}

bool TelnetSession::sendLine(const std::string& line)
{
	const std::string dataToSend = line + "\r\n";
	
	std::error_code ec;
	std::size_t n = _socket.write_some(asio::const_buffer(dataToSend.c_str(), dataToSend.length()), ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__
			<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
		return false;
	}
	return true;
}

void TelnetSession::stripNVT(std::string& s)
{
	// std::size_t pos = 0;
	// do
	// {
	// 	pos = s.find_first_of(static_cast<char>(IAC));
	// 	if (pos != std::string::npos && (pos + 2) <= (s.length() - 1))
	// 	{
	// 		s.erase(pos, 3);
	// 	}
	// }
	// while (pos != std::string::npos);

	std::size_t pos = 0;
	while ((pos = s.find_first_of(static_cast<char>(IAC))) != std::string::npos)
	{
		if (pos <= (s.length() - 3))
		{
			s.erase(pos, 3);
		}
		else
		{
			break;
		}
	}
}

std::vector<std::string> TelnetSession::getCompleteLines(std::string& s)
{
	std::vector<std::string> lines;

	std::size_t pos = 0;
	while ((pos = s.find("\r\n")) != std::string::npos)
	{
		lines.push_back(s.substr(0, pos));
		s.erase(0, pos + 2);
	}

	return lines;
}

void TelnetSession::receive()
{
	_socket.async_read_some(asio::buffer(_recvBuffer, RECV_BUFFER_SIZE),
		[this](std::error_code ec, std::size_t sz)
		{
			if (!ec)
			{
				if (sz != 0)
				{
					sendEcho(sz);
					std::replace(_recvBuffer.begin(), _recvBuffer.begin() + sz, 0x00, 0x0A);
					_buffer.append(_recvBuffer.data(), sz);

					stripNVT(_buffer);

					if (!_prompt.empty())
					{
						;
					}

					std::vector<std::string> lines(getCompleteLines(_buffer));
					for (const std::string& line : lines)
					{
						if (_lineCompleteCB)
						{
							_lineCompleteCB(this, line);
						}

						addToHistory(line);
					}
				}

				receive();
			}
			else
			{
				std::cerr << __FILE__ << ':' << __LINE__ 
					<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
				// TO DO: shutdown session
			}
		});
}

bool TelnetSession::sendEcho(std::size_t numOfBytes)
{
	assert(numOfBytes < RECV_BUFFER_SIZE);

	if (_recvBuffer[0] == IAC)
	{
		return true;
	}

	std::error_code ec;
	std::size_t n = _socket.write_some(asio::const_buffer(_recvBuffer.data(), numOfBytes), ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__
			<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
		return false;
	}
	return true;
}

void TelnetSession::addToHistory(const std::string& line)
{

}
