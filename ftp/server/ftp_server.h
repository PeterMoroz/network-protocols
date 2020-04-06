#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>

using asio::ip::tcp;

class FTPSession;

class FTPServer final
{
public:
	FTPServer(const FTPServer&) = delete;
	const FTPServer& operator=(const FTPServer&) = delete;

	FTPServer();
	~FTPServer();

	void start(const std::string& address, std::uint16_t port);
	void stop();

private:
	void acceptConnections();

	void worker();

private:
	asio::io_context _ioContext;
	asio::ip::tcp::acceptor _acceptor;
	std::thread _worker;

	std::list<std::unique_ptr<FTPSession>> _sessions;
};
