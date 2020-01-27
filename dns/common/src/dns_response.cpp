#include "dns_response.h"

#include <arpa/inet.h>

#include <algorithm>
#include <sstream>

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
	
	std::size_t n = getQCount();
	_questions.resize(n);
	for (std::size_t i = 0; i < n; i++)
	{
		Question question;
		std::size_t offset = 0;
		bytesCount += DNSMessage::decodeDomainName(buffer.data() + bytesCount, question._name, offset);
		while (offset != 0)
		{
			std::string name;
			DNSMessage::decodeDomainName(buffer.data() + offset, name, offset);
			if (!question._name.empty())
			{
				question._name.push_back('.');
			}
			question._name.append(name);
		}

		const std::uint16_t* src = reinterpret_cast<const std::uint16_t*>(buffer.data() + bytesCount);

		question._type = ntohs(*src);
		bytesCount += sizeof(std::uint16_t);
		src += 1;

		question._cls = ntohs(*src);
		bytesCount += sizeof(std::uint16_t);
		src += 1;

		_questions[i] = question;
	}


	n = getACount();
	_answerResourceRecords.resize(n);
	for (std::size_t i = 0; i < n; i++)
	{
		ResourceRecord record;
		bytesCount += readResourceRecord(buffer.data(), bytesCount, record);
		_answerResourceRecords[i] = record;
	}


	n = getNSCount();
	_authorityResourceRecords.reserve(n);
	for (std::size_t i = 0; i < n; i++)
	{
		ResourceRecord record;
		bytesCount += readResourceRecord(buffer.data(), bytesCount, record);
		_authorityResourceRecords[i] = record;
	}


	n = getARCount();
	_additionalResourceRecords.reserve(n);
	for (std::size_t i = 0; i < n; i++)
	{
		ResourceRecord record;
		bytesCount += readResourceRecord(buffer.data(), bytesCount, record);
		_additionalResourceRecords[i] = record;
	}

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
	// os << "\nname: " << _name
	// 	<< "\ntype: " << _type
	// 	<< "\nclass: " << _cls
	// 	<< "\nttl: " << _ttl
	// 	<< "\ndata length: " << _data.length()
	// 	<< "\ndata: " << _data;
	// // for (std::size_t i = 0; i < _data.size(); i++)
	// // {
	// // 	os.put(_data[i]);
	// // }

	os << "\nQuestions: " << std::endl;
	std::for_each(_questions.cbegin(), _questions.cend(), [&os](const Question& q){ q.dump(os); });

	os << "\nAnswers: " << std::endl;
	std::for_each(_answerResourceRecords.cbegin(), _answerResourceRecords.cend(), [&os](const ResourceRecord& rr){ rr.dump(os); });

	os << "\nName servers: " << std::endl;
	std::for_each(_authorityResourceRecords.cbegin(), _authorityResourceRecords.cend(), [&os](const ResourceRecord& rr){ rr.dump(os); });

	os << "\nAdditional records: " << std::endl;
	std::for_each(_additionalResourceRecords.cbegin(), _additionalResourceRecords.cend(), [&os](const ResourceRecord& rr){ rr.dump(os); });		
}


std::string DNSResponse::responseCodeToString(std::uint8_t rcode)
{
	switch (rcode)
	{
	case Rcode::NoError:
		return "no error";		
	case Rcode::FormatError:
		return "query format error";
	case Rcode::ServerFailure:
		return "server failure";
	case Rcode::NameError:
		return "the name is not exist";
	case Rcode::NotImplemented:
		return "not implemented";
	case Rcode::Refused:
		return "refused";
	default:
		return "unknown";
	}
	return "";
}

void DNSResponse::setRCode(std::uint8_t rcode)
{
	DNSMessage::setFieldRcode(rcode);
}


bool DNSResponse::isAuthoritative() const
{
	return DNSMessage::getFlagAA();
}

bool DNSResponse::isRecursionRequired() const
{
	return DNSMessage::getFlagRA();
}

bool DNSResponse::isRecursionAvailable() const
{
	return DNSMessage::getFlagRA();
}

std::uint8_t DNSResponse::getRcode() const
{
	return DNSMessage::getFieldRcode();
}


void DNSResponse::Question::dump(std::ostream& os) const
{
	os << " name: " << _name << ", type: " << _type << ", class: " << _cls << std::endl;
}

void DNSResponse::ResourceRecord::dump(std::ostream& os) const
{
	os << " name: " << _name << ", type: " << _type << ", class: " << _cls 
		<< ", TTL: " << _ttl << ", rlength: " << _data.size();
	// 	<< ", rdata: ";
	// for (const std::uint8_t x : _data)
	// {
	// 	os << std::hex << static_cast<std::uint16_t>(x) << ' ';
	// }
	switch (_type)
	{
	case static_cast<std::uint8_t>(QType::A):
		os << ", Address: " << _text;
	break;

	case static_cast<std::uint8_t>(QType::CNAME):
		os << ", CNAME: " << _text;
	break;

	default:
		{
			os << ", rdata: ";
			for (const std::uint8_t x : _data)
			{
				os << std::hex << static_cast<std::uint16_t>(x) << ' ';
			}
		}
	}
	os << std::endl;
}

std::size_t DNSResponse::readResourceRecord(const std::uint8_t* msgBegin, std::size_t recOffset, ResourceRecord& record)
{	
	std::size_t bytesCount = 0;

	std::size_t nameOffset = 0;
	bytesCount += DNSMessage::decodeDomainName(msgBegin + recOffset, record._name, nameOffset);
	while (nameOffset != 0)
	{
		std::string name;
		DNSMessage::decodeDomainName(msgBegin + nameOffset, name, nameOffset);
		if (!record._name.empty())
		{
			record._name.push_back('.');
		}
		record._name.append(name);
	}

	const std::uint16_t* src16 = reinterpret_cast<const std::uint16_t*>(msgBegin + (recOffset + bytesCount));

	record._type = ntohs(*src16);
	bytesCount += sizeof(std::uint16_t);
	src16 += 1;

	record._cls = ntohs(*src16);
	bytesCount += sizeof(std::uint16_t);
	src16 += 1;

	const std::uint32_t* src32 = reinterpret_cast<const std::uint32_t*>(msgBegin + (recOffset + bytesCount));
	record._ttl = ntohl(*src32);
	bytesCount += sizeof(std::uint32_t);

	src16 = reinterpret_cast<const std::uint16_t*>(msgBegin + (recOffset + bytesCount));
	std::uint16_t rdataLength = ntohs(*src16);
	bytesCount += sizeof(std::uint16_t);

	record._data.resize(rdataLength);
	std::copy_n(msgBegin + (recOffset + bytesCount), rdataLength, record._data.begin());
	bytesCount += rdataLength;

	switch (record._type)
	{
	case static_cast<std::uint8_t>(QType::A):
	{
		std::ostringstream oss;
		std::size_t i = 0, n = record._data.size() - 1;
		for (; i < n; i++)
		{
			oss << static_cast<std::uint16_t>(record._data[i]) << '.';
		}
		oss << static_cast<std::uint16_t>(record._data[i]);
		record._text = oss.str();
	}
	break;

	case static_cast<std::uint8_t>(QType::CNAME):
	{
		DNSMessage::decodeDomainName(record._data.data(), record._text, nameOffset);
		while (nameOffset != 0)
		{
			std::string name;
			DNSMessage::decodeDomainName(msgBegin + nameOffset, name, nameOffset);
			if (!record._text.empty())
			{
				record._text.push_back('.');
			}
			record._text.append(name);
		}		
	}
	break;

	default:
		std::cerr << __FILE__ << ':' << __LINE__ 
			<< "\tError: no way to transform RR of type " << record._type << " into text.\n";
	}	

	return bytesCount;
}
