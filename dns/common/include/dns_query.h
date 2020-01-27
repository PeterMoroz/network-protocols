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

	void setName(const std::string& name)
	{
		_name = name;
	}

	const std::string getName() const 
	{
		return _name;
	}

	void setType(QType qtype)
	{
		_type = static_cast<std::uint8_t>(qtype);
	}

	std::uint16_t getType() const
	{
		return _type;
	}

	std::uint16_t getClass() const
	{
		return _cls;
	}

	void setUseRecursion(bool useRecursion);

	void dump(std::ostream& os) const;	

private:
	// TO DO: remove, because it is obsolette.
	// use DNSMessage::deodeDomainName instead.
	std::size_t decodeName(const std::uint8_t* data, std::size_t size);

private:
	std::string _name;
	std::uint16_t _type = 0;
	std::uint16_t _cls = 1;	// query class = 1 (IN) in 99% of cases
};
