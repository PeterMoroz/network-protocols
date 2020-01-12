#pragma once

#include <cstdint>
#include <string>

#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/ip/udp.hpp>

using asio::ip::udp;

class DNSResolver;

class DNSServer final
{
public:
	DNSServer(const std::string& addr, std::uint16_t port, bool reuseAddr = false);
	~DNSServer();

	DNSServer(const DNSServer&) = delete;
	DNSServer& operator=(const DNSServer&) = delete;

	void start();
	void stop();

	void setResolver(DNSResolver* resolver)
	{
		_resolver = resolver;
	}

private:	
	void receive();
	void sendResponse(std::vector<std::uint8_t>&& buffer);

	void waitSignal();

private:
	std::string _addr;
	std::uint16_t _port = 0;
	asio::io_context _ioContext;
	asio::signal_set _signal;
	udp::socket _socket;
	udp::endpoint _clientEndpoint;
	DNSResolver* _resolver = NULL;
};
