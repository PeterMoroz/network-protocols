#include "dns_message.h"

#include <arpa/inet.h>

#include <iomanip>


DNSMessage::DNSMessage(std::uint16_t id, bool isResponse /*= false*/)
	: _id(id)
{
	if (isResponse)
	{
		setFlagQR(true);
	}
}

DNSMessage::~DNSMessage()
{

}

std::size_t DNSMessage::decode(const std::vector<std::uint8_t>& buffer)
{
	std::size_t bytesCount = 0;
	const std::uint16_t* src = reinterpret_cast<const std::uint16_t*>(buffer.data());	

	_id = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	_flags = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	_qCount = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	_aCount = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	_nsCount = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	_arCount = ntohs(*src);
	src += 1;
	bytesCount += sizeof(std::uint16_t);

	return bytesCount;
}

std::vector<std::uint8_t> DNSMessage::encode() const
{
	std::vector<std::uint8_t> result(6 * sizeof(std::uint16_t));

	std::uint16_t* dst = reinterpret_cast<std::uint16_t*>(result.data());
	*dst = htons(_id);
	dst += 1;
	*dst = htons(_flags);
	dst += 1;
	*dst = htons(_qCount);
	dst += 1;
	*dst = htons(_aCount);
	dst += 1;
	*dst = htons(_nsCount);
	dst += 1;
	*dst = htons(_arCount);

	return result;
}

void DNSMessage::dump(std::ostream& os) const
{
	os << "ID: " << std::hex << std::setw(4) << std::setfill('0') << std::showbase << _id
		<< std::noshowbase << ", FLAGS: " << _flags
		<< "\nQuestion count: " << std::dec << _qCount
		<< "\nAnswer record count: " << _aCount
		<< "\nName server (Authority record) count: " << _nsCount
		<< "\nAdditional record count: " << _arCount;
}


bool DNSMessage::getFlagQR() const
{
	return ((_flags >> 15) == 1);
}

std::uint8_t DNSMessage::getFieldOpcode() const
{
	return ((_flags >> 11) & 0xF);
}

bool DNSMessage::getFlagAA() const
{
	return (((_flags >> 10) & 0x1) == 1);
}

bool DNSMessage::getFlagTC() const
{
	return (((_flags >> 9) & 0x1) == 1);
}

bool DNSMessage::getFlagRD() const
{
	return (((_flags >> 8) & 0x1) == 1);
}

bool DNSMessage::getFlagRA() const
{
	return (((_flags >> 7) & 0x1) == 1);
}

std::uint8_t DNSMessage::getFieldRcode() const
{
	return (_flags & 0xF);
}


void DNSMessage::setFlagQR(bool on)
{
	if (on)
	{
		_flags |= 1 << 15;
	}
	else
	{
		_flags &= ~(1 << 15);
	}
}

void DNSMessage::setFieldOpcode(std::uint8_t opcode)
{
	_flags &= 0x87FF; // ~(0x7800);
	_flags |= (opcode & 0xF) << 11;
}

void DNSMessage::setFlagAA(bool on)
{
	if (on)
	{
		_flags |= 1 << 10;
	}
	else
	{
		_flags &= ~(1 << 10);
	}
}

void DNSMessage::setFlagTC(bool on)
{
	if (on)
	{
		_flags |= 1 << 9;
	}
	else
	{
		_flags &= ~(1 << 9);
	}
}

void DNSMessage::setFlagRD(bool on)
{
	if (on)
	{
		_flags |= 1 << 8;
	}
	else
	{
		_flags &= ~(1 << 8);
	}
}

void DNSMessage::setFlagRA(bool on)
{
	if (on)
	{
		_flags |= 1 << 7;
	}
	else
	{
		_flags &= ~(1 << 7);
	}
}

void DNSMessage::setFieldRcode(std::uint8_t rcode)
{
	_flags &= 0xFFF0;
	_flags |= (rcode & 0xF);
}

// std::size_t DNSMessage::decodeDomainName(const std::uint8_t* data, std::string& name)
// {
// 	std::size_t bytesCount = 0;
// 	name.clear();

// 	std::size_t length = *data;
// 	while (length != 0)
// 	{
// 		bytesCount += 1;
// 		data += 1;

// 		for (; length != 0; length--, data++, bytesCount++)
// 		{
// 			char x = *reinterpret_cast<const char*>(data);
// 			name.push_back(x);			
// 		}

// 		length = *data;
// 		if (length != 0)
// 		{
// 			name.push_back('.');
// 		}
// 	}

// 	return bytesCount + 1;
// }

std::size_t DNSMessage::decodeDomainName(const std::uint8_t* data, std::string& name, std::size_t& offset)
{
	std::size_t bytesCount = 0;
	name.clear();
	offset = 0;

	if (*data & 0xC0)
	{
		offset = (*data) & 0x3F;
		offset <<= 8;
		offset += *(data + 1);
		return 2;
	}

	std::size_t length = *data;
	while (length != 0)
	{
		bytesCount += 1;
		data += 1;

		for (; length != 0; length--, data++, bytesCount++)
		{
			char x = *reinterpret_cast<const char*>(data);
			name.push_back(x);			
		}

		if (*data & 0xC0)
		{
			offset = (*data) & 0x3F;
			offset <<= 8;
			offset += *(data + 1);
			return bytesCount + 2;
		}		

		length = *data;
		if (length != 0)
		{
			name.push_back('.');
		}
	}

	return bytesCount + 1;
}

std::vector<std::uint8_t> DNSMessage::encodeDomainName(const std::string& name)
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
