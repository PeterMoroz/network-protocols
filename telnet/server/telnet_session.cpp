#include <cassert>
#include <cctype>

#include <algorithm>
#include <iostream>

#include <array>

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
	_optionsToConfirm.push_back(Echo);
	_optionsToConfirm.push_back(SuppressGoAhead);

	const std::uint8_t willEcho[3] = { IAC, WILL, Echo };
	std::cout << "Send WILL ECHO\n";
	asio::async_write(_socket, asio::const_buffer(willEcho, 3),
		[this](std::error_code ec, std::size_t sz)
		{
			if (!ec)
			{
				const std::uint8_t dontEcho[3] = { IAC, DONT, Echo };
				std::cout << "Send DONT ECHO\n";
				asio::async_write(_socket, asio::const_buffer(dontEcho, 3),
					[this](std::error_code ec, std::size_t sz)
					{
						if (!ec)
						{
							const std::uint8_t willSuppressGoAhead[3] = { IAC, WILL, SuppressGoAhead };
							std::cout << "Send WILL SUPPRESS GO AHEAD\n";
							asio::async_write(_socket, asio::const_buffer(willSuppressGoAhead, 3),
								[this](std::error_code ec, std::size_t sz)
								{
									if (!ec)
									{
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
	std::error_code ec;
	std::size_t n = _socket.write_some(asio::const_buffer(line.c_str(), line.length()), ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__
			<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
		return false;
	}
	return true;
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
					switch (_state)
					{
					case Negotiation:
					{
						std::size_t i = 0;
						while (i < sz)
						{
							std::cout << " i = " << i << " byte = " << static_cast<std::uint16_t>(_recvBuffer[i]) << std::endl;
							if (_recvBuffer[i] == DO)
							{
								i += 1;
								if (i < sz && _recvBuffer[i] != IAC)
								{
									const std::size_t x = _recvBuffer[i];
									_optionsToConfirm.remove_if([x](const std::size_t v){ return x == v; });
								}
							}
							i += 1;
						}

						if (_optionsToConfirm.empty())
						{
							std::cout << "All options confirmed, start autentification.\n";
							startAuthentication();
						}
					}
					break;

					case Authentication:
					{
						if (sz == 1 && ::isprint(_recvBuffer[0]))
						{
							_buffer.push_back(_recvBuffer[0]);
							send('*');
						}
						else if (sz == 2)
						{
							if (_recvBuffer[0] == CarriageReturn && _recvBuffer[1] == NUL)
							{
								const std::uint8_t endOfLine[2] = { CarriageReturn, LineFeed };
								send(endOfLine, 2);
								if (_username.empty())
								{
									_username = _buffer;
									_buffer.clear();
									sendLine("Password: ");
								}
								else
								{
									_password = _buffer;
									_buffer.clear();

									if (authorize())
									{
										std::cout << "Authentication - success.\n";
										startInteraction();
									}
									else
									{
										startAuthentication();
									}
								}
							}
						}
					}
					break;

					case Interaction:
						if (sz == 1)
						{
							// std::cout << " received " << static_cast<std::uint16_t>(_recvBuffer[0] & 0xFF) << std::endl;
							if (::isprint(_recvBuffer[0]))
							{
								_buffer.push_back(_recvBuffer[0]);
								send(_recvBuffer[0]);
							}
							else 
							{
								if (_recvBuffer[0] == BackSpace)
								{
									_buffer = _buffer.substr(0, _buffer.length() - 1);
									sendEraseLine();
									sendLine(_buffer);
								}
							}
						}
						else if (sz == 2)
						{
							// std::cout << " received "
							// 	<< static_cast<std::uint16_t>(_recvBuffer[0] & 0xFF)
							// 	<< " " << static_cast<std::uint16_t>(_recvBuffer[1] & 0xFF)
							// 	<< std::endl;

							if (_recvBuffer[0] == CarriageReturn && _recvBuffer[1] == NUL)
							{
								if (_lineCompleteCB)
								{
									_lineCompleteCB(this, _buffer);
								}

								addToHistory(_buffer);
								_buffer.clear();
								sendEraseLine();
							}
							else
							{

							}
						}
						else if (sz == 3)
						{
							// std::cout << " received "
							// 	<< static_cast<std::uint16_t>(_recvBuffer[0] & 0xFF)
							// 	<< " " << static_cast<std::uint16_t>(_recvBuffer[1] & 0xFF)
							// 	<< " " << static_cast<std::uint16_t>(_recvBuffer[2] & 0xFF)
							// 	<< std::endl;
							if (_recvBuffer[0] == 0x1B)
							{
								processEscapeSequence(std::string(reinterpret_cast<char*>(_recvBuffer.data()), 3));
							}
						}
					break;

					default:
						std::cerr << __FILE__ << ':' << __LINE__ << "\tUnexpected state!\n";
						assert(false);
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

bool TelnetSession::send(std::uint8_t b)
{
	const std::uint8_t buffer[1] = { b };
	std::error_code ec;
	std::size_t n = _socket.write_some(asio::const_buffer(buffer, 1), ec);
	if (ec)
	{
		std::cerr << __FILE__ << ':' << __LINE__
			<< " - Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
		return false;
	}
	return true;
}

bool TelnetSession::send(const std::uint8_t* data, std::size_t size)
{
	std::error_code ec;
	std::size_t n = _socket.write_some(asio::const_buffer(data, size), ec);
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
	std::vector<std::string>::const_iterator it = std::find(_history.cbegin(), _history.cend(), line);
	if (it == _history.cend())
	{
		_history.push_back(line);
		if (_history.size() > MAX_HISTORY_LEGTH)
		{
			_history.erase(_history.cbegin());
		}
		_historyPos = _history.size() - 1;		
	}
	else
	{
		_historyPos = std::distance(_history.cbegin(), it);
	}
}

void TelnetSession::startAuthentication()
{
	_buffer.clear();
	_username.clear();
	_password.clear();

	_state = Authentication;
	sendLine("Username: ");
}

void TelnetSession::startInteraction()
{
	_buffer.clear();
	_state = Interaction;
}

bool TelnetSession::authorize()
{
	return (_username == "testuser" && _password == "testpassword");
}

void TelnetSession::sendEraseLine()
{
	const std::string ERASE_LINE("\x1B[2K");
	sendLine(ERASE_LINE);
	const std::string HOME("\x1B[80D");
	sendLine(HOME);
}

void TelnetSession::processEscapeSequence(const std::string& escSequence)
{
	const std::string ARROW_UP("\x1b\x5b\x41");
	const std::string ARROW_DOWN("\x1b\x5b\x42");
	const std::string ARROW_RIGHT("\x1b\x5b\x43");
	const std::string ARROW_LEFT("\x1b\x5b\x44");

	if (escSequence == ARROW_LEFT)
	{
		std::cout << "Left arrow\n";
	}
	else if (escSequence == ARROW_RIGHT)
	{
		std::cout << "Right arrow\n";
	}
	else if (escSequence == ARROW_DOWN)
	{
		// std::cout << "Down arrow\n";
		if (_history.size() > 1)
		{
			if (_historyPos != _history.size() - 1)
			{
				_historyPos += 1;
			}
			else
			{
				_historyPos = 0;
			}

			_buffer = _history[_historyPos];
			sendEraseLine();
			sendLine(_buffer);
		}		
	}
	else if (escSequence == ARROW_UP)
	{
		// std::cout << "Up arrow\n";
		if (_history.size() > 1)
		{
			if (_historyPos != 0)
			{
				_historyPos -= 1;
			}
			else
			{
				_historyPos = _history.size() - 1;
			}

			_buffer = _history[_historyPos];
			sendEraseLine();
			sendLine(_buffer);
		}		
	}
}
