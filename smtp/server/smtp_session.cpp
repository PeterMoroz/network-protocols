#include "smtp_session.h"
#include "smtp_server.h"
#include "log.h"

#include <cassert>

#include <exception>
#include <iomanip>
#include <string>
#include <sstream>


SMTPSession::SMTPSession(std::uint32_t id, SMTPServer* server, tcp::socket&& socket)
	: _id(id)
	, _server(server)
	, _socket(std::move(socket))
{
	assert(_server != NULL);
}

SMTPSession::~SMTPSession()
{

}

void SMTPSession::start()
{
	sendResponse(220);
}

void SMTPSession::close()
{	
	// std::cout << " the session " << getId() << " is closing.\n";

	asio::error_code ec;

	_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	if (ec)
	{
		LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	_socket.close(ec);
	if (ec)
	{
		LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
	}

	_server->closeSession(getId());
}

void SMTPSession::receive()
{
	_socket.async_read_some(asio::buffer(_recvBuffer),
		[this](const asio::error_code& ec, std::size_t sz)
		{
			if (!ec)
			{
				_recvLength = sz;
				// std::cout << " the number of received bytes " << sz << std::endl;
				// std::cout << " received command: ";
				// for (std::size_t i = 0; i < sz; i++)
				// {
				// 	std::cout.put(_recvBuffer[i]);
				// }
				// std::cout << "----------------\n";

				if (_receiveMessage)
				{
					processMessage();
				}
				else
				{
					processCommand();
				}
			}
			else
			{
				LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
				close();
			}
		});
}

void SMTPSession::sendResponse(std::uint16_t responseCode)
{
	// std::cout << " send response code " << responseCode << std::endl;

	asio::streambuf sb;
	std::ostream oss(&sb);
	oss << std::setw(3) << std::setfill('0') << responseCode;

	// TO DO:
	// correct text explnations in accordance with table here
	// https://blog.mailtrap.io/smtp-commands-and-responses/
	switch (responseCode)
	{
	case 220:
		oss << ' ' << _domainName << " Simple Mail Transfer Service ready\r\n";
		break;
	case 221:
		oss << " The server closes the transmission channel\r\n";
		break;
	case 250:
		oss << " OK\r\n";
		break;
	case 354:
		oss << " Start mail input; end with <CRLF>.<CRLF>\r\n";
		break;
	case 451:
		oss << " The server aborted the command due to a local error\r\n";
		break;		
	case 500:
		oss << " Syntax error, can not recognize command or command too long\r\n";
		break;		
	case 501:
		oss << " Syntax error in parameters or arguments\r\n";
		break;
	case 502:
		oss << " The server has not implemented the command\r\n";
		break;
	case 503:
		oss << " Improper sequence of commands\r\n";
		break;
	case 504:
		oss << " The server has not implemented a command parameter\r\n";
		break;		
	case 510:
		oss << " Invalid email address\r\n";
		break;		
	case 523:
		oss << " The total size of your mailing exceeds the recipient server limits\r\n";
		break;		
	case 550:
		oss << " Mailbox is unavailable. Server aborted the command because the mailbox was not found or for policy reason\r\n";
		break;
	case 551:
		oss << " User not local. The <forward-path> will be specified\r\n";
		break;
	case 552:
		oss << " The server aborted the command because mailbox is full\r\n";
		break;		
	default:
		assert(false);
	}

	async_write(_socket, sb, [this, responseCode](const asio::error_code& ec, std::size_t sz)
		{
			if (!ec)
			{
				// std::cout << " the number of sent bytes " << sz << std::endl;
				if (responseCode == 221)	// probably some other codes lead to closing the session
				{
					close();
					return;
				}
				receive();
			}
			else
			{
				LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
				close();
			}
		});
}

std::string SMTPSession::getAddress()
{
	std::string address;

	std::size_t i = 0;
	const char* addrBegin = _recvBuffer.cbegin();
	while (i < COMMAND_BUFFER_SIZE && *addrBegin != '<'
		&& *addrBegin != '\r' && *addrBegin != '\n')
	{
		++i;
		++addrBegin;
	}

	if (i == COMMAND_BUFFER_SIZE || *addrBegin != '<')
	{
		return address;
	}

	const char* addrEnd = ++addrBegin;
	while (i < COMMAND_BUFFER_SIZE && *addrEnd != '>'
		&& *addrEnd != '\r' && *addrEnd != '\n')
	{
		++i;
		++addrEnd;
	}

	if (i == COMMAND_BUFFER_SIZE || *addrEnd != '>')
	{
		return address;
	}

	return std::string(addrBegin, (addrEnd - addrBegin));
}

std::string SMTPSession::getCommand()
{
	// TO DO: implement
	std::string command;
	return command;
}

void SMTPSession::processCommand()
{
	// TO DO: use getCommand()
	const char* p = _recvBuffer.cbegin();
	const char* s = _recvBuffer.cbegin();

	std::size_t i = 0;
	while (i < COMMAND_BUFFER_SIZE && *p != ' ' && *p != '\r' && *p != '\n')
	{
		++i;
		++p;
	}

	if (i == COMMAND_BUFFER_SIZE)
	{
		sendResponse(500);
		return;
	}

	if (*p == ' ')
	{
		while (i < COMMAND_BUFFER_SIZE && *p != ' ' && *p != '\r' && *p != '\n')
		{	
			++i;
			++p;
		}

		if (i == COMMAND_BUFFER_SIZE)
		{
			sendResponse(500);
			return;
		}

		if (*p == ' ')
		{
			std::size_t n = p - s;
			if (!std::strncmp(s, "MAIL FROM", n))
			{
				processMailFrom();
			}
			else if (!std::strncmp(s, "RCPT TO", n))
			{
				processRcptTo();
			}
			else
			{
				sendResponse(502);
			}
		}
		else
		{
			sendResponse(500);
		}
	}
	else
	{
		std::size_t n = p - s;
		if (n != 4)
		{
			sendResponse(500);
			return;
		}

		if (!std::strncmp(s, "HELO", n))
		{
			processHelo();
		}
		else if (!std::strncmp(s, "DATA", n))
		{
			processData();
		}
		else if (!std::strncmp(s, "RSET", n))
		{
			processRset();
		}
		else if (!std::strncmp(s, "VRFY", n))
		{
			processVrfy();
		}
		else if (!std::strncmp(s, "NOOP", n))
		{
			processNoop();
		}
		else if (!std::strncmp(s, "QUIT", n))
		{
			processQuit();
		}
		else
		{
			sendResponse(502);
		}
	}
}

void SMTPSession::processMessage()
{
	if (_recvLength == 3 && !std::strncmp(_recvBuffer.data(), ".\r\n", 3))
	{
		_messageOut.close();
		deliverMessage();
		deleteMessage();
		sendResponse(250);
	}
	else
	{
		_messageOut.write(_recvBuffer.data(), _recvLength);
		receive();
	}
}

void SMTPSession::processHelo()
{
	_rcptCount = 0;
	_returnAddress.clear();

	if (!createMessage())
	{
		// Now return 451 is 'local error'.
		// But I believe that create message failure may cause other errors
		sendResponse(451);
		return;
	}

	_lastCommand = Command::Helo;
	sendResponse(250);
}

void SMTPSession::processData()
{
	if (_lastCommand != Command::Data)
	{
		_receiveMessage = true;
		_lastCommand = Command::Data;
		sendResponse(354);
	}
}

void SMTPSession::processRset()
{
	_rcptCount = 0;
	_returnAddress.clear();
	_receiveMessage= false;	

	std::cout << "RSET\n";
	deleteMessage();

	_lastCommand = Command::Rset;
	sendResponse(250);
}

void SMTPSession::processVrfy()
{

}

void SMTPSession::processNoop()
{
	std::cout << "NOOP\n";
	sendResponse(250);
}

void SMTPSession::processQuit()
{
	std::cout << "QUIT\n";
	sendResponse(221);
}

void SMTPSession::processMailFrom()
{
	if (_lastCommand != Command::Helo && _lastCommand != Command::Rset)
	{
		sendResponse(503);
		return;
	}

	std::string address(getAddress()); // const std::string address(getAddress());
	if (address.empty())
	{
		sendResponse(501);
		return;
	}

	std::cout << "MAIL FROM <" << address << ">\n";
	_returnAddress = std::move(address);

	_lastCommand = Command::MailFrom;
	sendResponse(250);
}

void SMTPSession::processRcptTo()
{
	if (_lastCommand != Command::MailFrom)
	{
		sendResponse(501);
		return;
	}

	if (_rcptCount >= MAX_RCPT_ALLOWED)
	{
		sendResponse(523);
		return;
	}

	std::string address(getAddress());// const std::string address(getAddress());
	if (address.empty())
	{
		sendResponse(501);
		return;
	}

	std::size_t p = address.find_first_of('@');
	if (p == std::string::npos)
	{
		sendResponse(501);
		return;
	}

	const std::string user(address.substr(0, p));
	const std::string domain(address.substr(p + 1));

	std::cout << "RCPT TO <" << address << ">, user: " << user << ", domain: " << domain << std::endl;

	namespace fs = std::experimental::filesystem;	

	try
	{
		fs::path domainPath = _mailboxesRoot / domain;
		if (!fs::exists(domainPath) || !fs::is_directory(domainPath))
		{
			sendResponse(551);
			return;
		}

		fs::path mboxPath = domainPath / user;
		if (!fs::exists(domainPath) || !fs::is_directory(domainPath))
		{
			sendResponse(550);
			return;
		}

		_recipients[_rcptCount] = std::make_pair(std::move(address), mboxPath.string());
		_rcptCount += 1;
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
		sendResponse(451);
		return;
	}

	_lastCommand != Command::RcptTo;
	sendResponse(250);
}

bool SMTPSession::createMessage()
{
	static const char MESSAGES_ROOT[] = "./messages";
	try 
	{
		std::ostringstream oss;
		oss << "message-" << std::setw(5) << std::setfill('0') << getId() << ".eml";
		_messagePath = MESSAGES_ROOT;
		_messagePath /= oss.str();
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
		return false;
	}

	_messageOut.open(_messagePath.string());
	if (!_messageOut)
	{
		LOG_ERROR() << " - Error 'Could not open file " << _messagePath.string() << "'\n";
		_messagePath.clear();
		return false;
	}

	return true;
}

void SMTPSession::deleteMessage()
{
	_messageOut.close();

	namespace fs = std::experimental::filesystem;
	try
	{
		fs::remove(_messagePath);
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";

	}
}

void SMTPSession::deliverMessage()
{
	_unknownRecipients.clear();

	namespace fs = std::experimental::filesystem;

	for (std::uint8_t i = 0; i < _rcptCount; i++)
	{
		try
		{
			fs::path dstPath(_recipients[i].second);
			if (fs::exists(dstPath))
			{
				std::error_code ec;
				fs::copy(_messagePath, dstPath, ec);
				if (ec)
				{
					LOG_ERROR() << " - Error " << ec.message() << '(' << ec.value() << ')' << std::endl;
				}
			}
			else
			{
				_unknownRecipients.push_back(_recipients[i].first);
			}
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
		}
	}

	// std::cout << " Unknown recipients: \n";
	// for (const std::string& s : _unknownRecipients)
	// {
	// 	std::cout << s << std::endl;
	// }
	// std::cout << "--------------------\n";	
}
