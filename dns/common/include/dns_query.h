#pragma once

#include "dns_message.h"

#include <string>

class DNSQuery : public DNSMessage
{
public:
	enum class QType : std::uint8_t
	{
		A     = 1,
		NS    = 2,
		CNAME = 5,
		SOA   = 6,
		MB    = 7,
		WKS   = 11,
		PTR   = 12,
		HINFO = 13,
		MINFO = 14,
		MX    = 15,
		TXT   = 16,
		AXFR  = 252,
		ANY   = 255
	};


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

	void dump(std::ostream& os) const;	

private:
	std::size_t decodeName(const std::uint8_t* data, std::size_t size);

private:
	std::string _name;
	std::uint16_t _type = 0;
	std::uint16_t _cls = 1;	// query class = 1 (IN) in 99% of cases
};
