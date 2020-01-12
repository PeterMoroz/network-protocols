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
	std::vector<std::uint8_t> name(encodeDomainName(_name));

	// assume, that data contains domain name.
	// this assumption is correct for query with type A (1)
	std::vector<std::uint8_t> data(encodeDomainName(_data));

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

std::vector<std::uint8_t> DNSResponse::encodeDomainName(const std::string& name)
{
	std::vector<std::uint8_t> result;
	result.reserve(name.length() + 2);

	std::size_t p0 = 0, p1 = name.find('.');
	while (p1 != std::string::npos)
	{
		std::size_t n = p1 - p0;
		result.push_back(static_cast<std::uint8_t>(n));
		for (; p0 < p1; p0++)
		{
			result.push_back(static_cast<std::uint8_t>(name[p0]));
		}

		p0 = p1 + 1;
		p1 = name.find('.', p0);
	}	

	std::size_t n = name.length() - p0;
	result.push_back(static_cast<std::uint8_t>(n));
	for (; p0 < name.length(); p0++)
	{
		result.push_back(static_cast<std::uint8_t>(name[p0]));
	}

	result.push_back(0);

	return result;
}
