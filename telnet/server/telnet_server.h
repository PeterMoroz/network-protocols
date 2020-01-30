#pragma once

#include <cstdint>

#include <list>
#include <memory>
#include <string>


#include <asio.hpp>

using asio::ip::tcp;

#include "telnet_session.h"


class TelnetServer final
{
public:
	TelnetServer(const TelnetServer&) = delete;
	TelnetServer& operator=(const TelnetServer&) = delete;

	TelnetServer();
	~TelnetServer();

	void start(const std::string& address, std::uint16_t port);

	void setPrompt(const std::string& prompt)
	{
		_prompt = prompt;
	}

	void setConnectedCB(TelnetSession::ConnectedCB&& connectedCB)
	{
		_connectedCB = connectedCB; // _connectedCB = std::move(connectedCB);
	}

	void setLineCompleteCB(TelnetSession::LineCompleteCB&& lineCompleteCB)
	{
		_lineCompleteCB = lineCompleteCB; // _lineCompleteCB = std::move(lineCompleteCB);
	}	

private:
	void acceptConnection();
	void waitSignal();

	void sessionFinished(TelnetSession* session);

	void shutdown();

private:
	asio::io_context _ioContext;
	asio::signal_set _signalSet;
	asio::ip::tcp::acceptor _acceptor;

	std::list<std::unique_ptr<TelnetSession>> _sessions;

	std::string _prompt{"ExperimentalTelnet>"};
	TelnetSession::ConnectedCB _connectedCB;
	TelnetSession::LineCompleteCB _lineCompleteCB;
};
