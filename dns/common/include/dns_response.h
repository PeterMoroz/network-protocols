#pragma once


#include "dns_message.h"

#include <string>
#include <vector>


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

	bool isAuthoritative() const;
	bool isRecursionRequired() const;
	bool isRecursionAvailable() const;
	std::uint8_t getRcode() const;

	std::uint32_t getTtl() const
	{
		return _ttl;
	}

	const std::string& getRdata() const
	{
		return _data;
	}	

	void dump(std::ostream& os) const;

	static std::string responseCodeToString(std::uint8_t rcode);

private:
	// TO DO: have to be refactored
	// these fields are used when sending response
	std::string _name;
	std::uint16_t _type;
	std::uint16_t _cls;
	std::uint32_t _ttl;
	std::string _data;
	//  these structures and fields are used when response is received
	struct Question
	{
		std::string _name;
		std::uint16_t _type = 0;
		std::uint16_t _cls = 0;

		void dump(std::ostream& os) const;
	};

	struct ResourceRecord
	{
		std::string _name;
		std::uint16_t _type = 0;
		std::uint16_t _cls = 0;
		std::uint32_t _ttl = 0;
		std::vector<std::uint8_t> _data;
		std::string _text;	// data as text

		void dump(std::ostream& os) const;
	};

	static std::size_t readResourceRecord(const std::uint8_t* msgBegin, std::size_t recOffset, ResourceRecord& record);

	std::vector<Question> _questions;
	std::vector<ResourceRecord> _answerResourceRecords;
	std::vector<ResourceRecord> _authorityResourceRecords;
	std::vector<ResourceRecord> _additionalResourceRecords;
};
