#include "dns_query.h"

#include <arpa/inet.h>


std::size_t DNSQuery::decode(const std::vector<std::uint8_t>& buffer)
{
	std::size_t bytesCount = DNSMessage::decode(buffer);
	bytesCount += decodeName(buffer.data() + bytesCount, buffer.size() - bytesCount);

	const std::uint16_t* src = reinterpret_cast<const std::uint16_t*>(buffer.data() + bytesCount);

	_type = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	_cls = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);	

	return bytesCount;
}

std::vector<std::uint8_t> DNSQuery::encode() const
{
	std::vector<std::uint8_t> result(DNSMessage::encode());
	std::vector<std::uint8_t> name(encodeDomainName(_name));

	result.reserve(result.size() + name.size() + 2 * sizeof(std::uint16_t));

	std::uint16_t type = htons(_type);
	std::uint16_t cls = htons(_cls);
	result.insert(result.end(), name.cbegin(), name.cend());
	const std::uint8_t* src = reinterpret_cast<std::uint8_t*>(&type);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));
	src = reinterpret_cast<std::uint8_t*>(&cls);
	result.insert(result.end(), src, src + sizeof(std::uint16_t));

	return result;
}

void DNSQuery::setUseRecursion(bool useRecursion)
{
	DNSMessage::setFlagRD(useRecursion);
}

void DNSQuery::dump(std::ostream& os) const
{
	DNSMessage::dump(os);
	os << "\nQuestion name: " << _name
		<< "\nQuestion type: " << _type
		<< "\nQuestion class: " << _cls;
}

std::size_t DNSQuery::decodeName(const std::uint8_t* data, std::size_t size)
{
	std::size_t bytesCount = 0;
	_name.clear();

	std::size_t length = *data;
	while (length != 0)
	{
		bytesCount += 1;
		data += 1;

		for (; length != 0; length--, data++, bytesCount++)
		{
			char x = *reinterpret_cast<const char*>(data);
			_name.push_back(x);			
		}

		length = *data;
		if (length != 0)
		{
			_name.push_back('.');
		}
	}

	return bytesCount + 1;
}
