#pragma once

#include "smtp_session.h"
#include "sessions_manager.h"

#include <cstdint>

#include <string>

#include <asio.hpp>

using asio::ip::tcp;

class SMTPServer final
{
public:
	SMTPServer();
	~SMTPServer();

	SMTPServer(const SMTPServer&) = delete;
	SMTPServer& operator=(const SMTPServer&) = delete;

	void start(const std::string& addr, std::uint16_t port);
	void stop();

private:
	void acceptConnection();

	void waitSignal();

private:
	asio::io_context _ioContext;
	asio::signal_set _signal;
	asio::ip::tcp::acceptor _acceptor;

	std::uint32_t _nextSessionId = 0;
	SessionsManager<SMTPSession> _sessionsManager;
};
