#include "dns_response.h"

#include <arpa/inet.h>

DNSResponse::DNSResponse()
	: DNSMessage(0, true)
{

}

DNSResponse::DNSResponse(std::uint16_t id)
	: DNSMessage(id, true)
{

}

std::size_t DNSResponse::decode(const std::vector<std::uint8_t>& buffer)
{
	std::size_t bytesCount = DNSMessage::decode(buffer);
	// TO DO: need to be implemented for DNS client
	return bytesCount;
}

std::vector<std::uint8_t> DNSResponse::encode() const
{
	std::vector<std::uint8_t> result(DNSMessage::encode());
	std::vector<std::uint8_t> name(DNSMessage::encodeDomainName(_name));

	// assume, that data contains domain name.
	// this assumption is correct for query with type A (1)
	std::vector<std::uint8_t> data(DNSMessage::encodeDomainName(_data));

	result.reserve(result.size() 									// header
				+ name.size() + 2 * sizeof(std::uint16_t)			// question section
				// answer section
				+ name.size() + 2 * sizeof(std::uint16_t) + sizeof(std::uint32_t) + sizeof(std::uint16_t) + data.size());

	// write question section
	std::uint16_t type = htons(_type);
	std::uint16_t cls = htons(_cls);
	result.insert(result.end(), name.cbegin(), name.cend());
	const std::uint8_t* src = reinterpret_cast<std::uint8_t*>(&type);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));
	src = reinterpret_cast<std::uint8_t*>(&cls);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));

	// write answer section
	std::uint32_t ttl = htonl(_ttl);

	result.insert(result.end(), name.cbegin(), name.cend());
	src = reinterpret_cast<std::uint8_t*>(&type);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));
	src = reinterpret_cast<std::uint8_t*>(&cls);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));
	src = reinterpret_cast<std::uint8_t*>(&ttl);
	result.insert(result.end(), src, src + sizeof(std::uint32_t));

	std::uint16_t rlength = htons(static_cast<std::uint16_t>(data.size()));
	src = reinterpret_cast<std::uint8_t*>(&rlength);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));

	result.insert(result.end(), data.cbegin(), data.cend());	

	return result;
}

void DNSResponse::dump(std::ostream& os) const
{
	DNSMessage::dump(os);
	os << "\nname: " << _name
		<< "\ntype: " << _type
		<< "\nclass: " << _cls
		<< "\nttl: " << _ttl
		<< "\ndata length: " << _data.length()
		<< "\ndata: " << _data;
	// for (std::size_t i = 0; i < _data.size(); i++)
	// {
	// 	os.put(_data[i]);
	// }
}

void DNSResponse::setRCode(std::uint8_t rcode)
{
	DNSMessage::setFieldRcode(rcode);
}
