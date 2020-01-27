#include "dns_client.h"
#include "dns_query.h"
#include "dns_response.h"

#include <cstdint>

#include <array>
#include <vector>


DNSClient::DNSClient(const std::string& srvAddress, std::uint16_t srvPort)
	: _srvAddress(srvAddress)
	, _srvPort(srvPort)
	, _ioContext(1)
	, _socket(_ioContext, udp::v4())
{

}

DNSClient::~DNSClient()
{

}

std::string DNSClient::resolve(const std::string& addr)
{
	DNSQuery dnsQuery;

	dnsQuery.setId(0xAAAA);	// arbitrary value
	if (_useRecursion)
	{
		dnsQuery.setUseRecursion(_useRecursion);
	}

    dnsQuery.setType(DNSMessage::QType::A);

	dnsQuery.setQCount(1);
	dnsQuery.setName(addr);

	std::vector<std::uint8_t> dataToSend(dnsQuery.encode());

    asio::ip::udp::endpoint ep(asio::ip::address::from_string(_srvAddress), _srvPort);

	std::cout << " Sending query to " << _srvAddress << ':' << _srvPort << std::endl;
	std::cout << " The query length is " << dataToSend.size() << " bytes.\n";

    asio::error_code ec;
    std::size_t n = 0;
    n = _socket.send_to(asio::const_buffer(dataToSend.data(), dataToSend.size()), ep, 0, ec);

    if (ec)
    {
    	std::cout << __FILE__ << ':' << __LINE__ << " Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
        return std::string();
    }

    std::cout << " Sent " << n << " bytes.\n";

    std::array<std::uint8_t, 512> recvBuffer;

    std::cout << " Receiving response from " << _srvAddress << ':' << _srvPort << std::endl;
    n = _socket.receive_from(asio::buffer(recvBuffer), ep, 0, ec);

    if (ec)
    {
    	std::cout << __FILE__ << ':' << __LINE__ << " Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
        return std::string();
    }

    std::cout << " Received " << n << " bytes. ";

    DNSResponse dnsResponse;

    n = dnsResponse.decode(std::vector<std::uint8_t>(recvBuffer.cbegin(), recvBuffer.cend()));
    std::cout << " Decoded " << n << " bytes.\n";

    if (dnsResponse.getId() == 0xAAAA)
    {
        dnsResponse.dump(std::cout);
        return "";
    }
    else
    {
    	std::cout << "Received unexpected response, ID = " << std::hex << dnsResponse.getId() << std::endl;
    	return std::string();
    }

    return std::string();
}
