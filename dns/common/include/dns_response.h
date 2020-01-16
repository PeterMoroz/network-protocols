#pragma once


#include "dns_message.h"

#include <string>


class DNSResponse : public DNSMessage
{
public:
	enum Rcode
	{
		NoError = 0,
		FormatError,
		ServerFailure,
		NameError,
		NotImplemented,
		Refused,
	};

public:
	DNSResponse();
	~DNSResponse() = default;
	explicit DNSResponse(std::uint16_t id);	

	std::size_t decode(const std::vector<std::uint8_t>& buffer);
	std::vector<std::uint8_t> encode() const;

	void setName(const std::string& name) { _name = name; }
	void setType(std::uint16_t type) { _type = type; }
	void setClass(std::uint16_t cls) { _cls = cls; }
	void setData(const std::string& data) { _data = data; }
	void setRCode(std::uint8_t rcode);

	void dump(std::ostream& os) const;

private:
	std::string _name;
	std::uint16_t _type;
	std::uint16_t _cls;
	std::uint32_t _ttl;
	std::string _data;// std::vector<char> _data;
};
