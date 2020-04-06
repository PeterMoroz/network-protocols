#pragma once

#include <cstdint>
#include <array>
#include <fstream>
#include <experimental/filesystem>
#include <list>

#include <asio.hpp>

using asio::ip::tcp;

class SMTPServer;

class SMTPSession final
{
	static const std::size_t COMMAND_BUFFER_SIZE = 512;
	static const std::size_t RECV_BUFFER_SIZE = 512;
	static const std::size_t MAX_RCPT_ALLOWED = 100;

	enum class Command
	{
		Undefined = 0,
		Helo,
		MailFrom,
		RcptTo,
		Data,
		Rset,
		Vrfy,
		Noop,
		Quit,
	};


public:
	SMTPSession(std::uint32_t id, SMTPServer* server, tcp::socket&& socket);
	~SMTPSession();

	std::uint32_t getId() const { return _id; }	

	void start();
	void close();

private:
	void receive();	
	void sendResponse(std::uint16_t responseCode);

	void processCommand();
	void processMessage();

	std::string getAddress();
	std::string getCommand();

	void processHelo();
	void processData();
	void processRset();
	void processVrfy();
	void processNoop();
	void processQuit();

	void processMailFrom();
	void processRcptTo();

	bool createMessage();
	void deleteMessage();
	void deliverMessage();


private:
	std::uint32_t _id{0};
	SMTPServer* _server = NULL;
	tcp::socket _socket;
	const std::string _domainName{"smtp.localhost.localdomain"};
	const std::string _serverName{"My SMTP server"};
	std::array<char, RECV_BUFFER_SIZE> _recvBuffer;
	std::size_t _recvLength{0};
	bool _receiveMessage = false;

	std::string _returnAddress;
	std::array<std::pair<std::string, std::string>, MAX_RCPT_ALLOWED> _recipients;
	std::uint8_t _rcptCount{0};
	Command _lastCommand = Command::Undefined;
	std::experimental::filesystem::path _mailboxesRoot{"./mailboxes"};
	std::experimental::filesystem::path _messagePath;
	std::ofstream _messageOut;
	std::list<std::string> _unknownRecipients;
};
