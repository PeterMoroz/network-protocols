#pragma once

#include <cstdint>
#include <string>

#include <asio.hpp>

using asio::ip::udp;


class DNSClient final
{
public:
	DNSClient(const DNSClient&) = delete;
	DNSClient& operator=(const DNSClient&) = delete;

	DNSClient(const std::string& srvAddress, std::uint16_t srvPort);
	~DNSClient();

	void setUseRecursion(bool useRecursion)
	{
		_useRecursion = useRecursion;
	}

	std::string resolve(const std::string& addr);

private:
	std::string _srvAddress;
	std::uint16_t _srvPort = 0;
	asio::io_context _ioContext;
	udp::socket _socket;
	bool _useRecursion = false;
};
