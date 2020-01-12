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

void DNSMessage::getFlagTC(bool on)
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

void DNSMessage::getFlagRD(bool on)
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

void DNSMessage::getFlagRA(bool on)
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
