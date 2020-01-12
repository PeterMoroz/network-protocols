#pragma once

#include "dns_message.h"

#include <string>

class DNSQuery : public DNSMessage
{
public:
	DNSQuery() = default;
	~DNSQuery() = default;


	std::size_t decode(const std::vector<std::uint8_t>& buffer);
	std::vector<std::uint8_t> encode() const;

	const std::string getName() const 
	{
		return _name;
	}

	std::uint16_t getType() const
	{
		return _type;
	}

	std::uint16_t getClass() const
	{
		return _cls;
	}

	void dump(std::ostream& os) const;	

private:
	std::size_t decodeName(const std::uint8_t* data, std::size_t size);

private:
	std::string _name;
	std::uint16_t _type = 0;
	std::uint16_t _cls = 0;
};
