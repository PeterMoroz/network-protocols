#pragma once

#include "smtp_session.h"
#include "sessions_manager.h"

#include <cstdint>

#include <memory>
#include <string>
#include <unordered_map>

#include <asio.hpp>

using asio::ip::tcp;

class SMTPServer final : public SessionsManager<SMTPSession>
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

	void startSession() override;
	void closeSession(std::uint32_t id) override;

private:
	asio::io_context _ioContext;
	asio::signal_set _signal;
	asio::ip::tcp::acceptor _acceptor;
	std::uint32_t _sessionId = 0;

	std::unordered_map<std::uint32_t, std::unique_ptr<SMTPSession>> _sessions;
};
