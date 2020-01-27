#pragma once

#include <cstdint>

#include <iostream>
#include <vector>


class DNSMessage
{
	// enum class FlagBitPos
	// {
	// 	QR_POS = 15,
	// 	OPCODE_POS = 14,
	// 	AA_POS = 10,
	// 	tc_POS = 9,
	// 	RD_POS = 8,
	// 	RA_POS = 7,
	// 	RCODE_POS = 3,
	// };

	// enum class : bool QRFlag
	// {
	// 	Query = false,
	// 	Response = true
	// };

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


protected:
	DNSMessage() = default;
	DNSMessage(std::uint16_t id, bool isResponse = false);

public:	
	~DNSMessage();

	DNSMessage(const DNSMessage&) = delete;
	DNSMessage& operator=(const DNSMessage&) = delete;

protected:
	std::size_t decode(const std::vector<std::uint8_t>& buffer);
	std::vector<std::uint8_t> encode() const;

	void dump(std::ostream& os) const;

public:
	std::uint16_t getId() const { return _id; }
	std::uint16_t getQCount() const { return _qCount; }
	std::uint16_t getACount() const { return _aCount; }
	std::uint16_t getNSCount() const { return _nsCount; }
	std::uint16_t getARCount() const { return _arCount; }

	void setId(std::uint16_t id) { _id = id; }
	void setQCount(std::uint16_t qCount) { _qCount = qCount; }
	void setACount(std::uint16_t aCount) { _aCount = aCount; }
	void setNSCount(std::uint16_t nsCount) { _nsCount = nsCount; }
	void setARCount(std::uint16_t arCount) { _arCount = arCount; }


public:
	bool getFlagQR() const;
	std::uint8_t getFieldOpcode() const;
	bool getFlagAA() const;
	bool getFlagTC() const;
	bool getFlagRD() const;
	bool getFlagRA() const;
	std::uint8_t getFieldRcode() const;

protected:
	void setFlagQR(bool on);
	void setFieldOpcode(std::uint8_t opcode);
	void setFlagAA(bool on);
	void setFlagTC(bool on);
	void setFlagRD(bool on);
	void setFlagRA(bool on);
	void setFieldRcode(std::uint8_t rcode);

protected:
	// static std::size_t decodeDomainName(const std::uint8_t* data, std::string& name);
	static std::size_t decodeDomainName(const std::uint8_t* data, std::string& name, std::size_t& offset);
	static std::vector<std::uint8_t> encodeDomainName(const std::string& name);

private:
	std::uint16_t _id = 0;		// identifier
	std::uint16_t _flags = 0;	// flags and codes
	std::uint16_t _qCount = 0;	// question count
	std::uint16_t _aCount = 0;	// answer record count
	std::uint16_t _nsCount = 0;	// name server (authority record) count
	std::uint16_t _arCount = 0;	// additional record count
};
