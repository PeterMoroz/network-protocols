#pragma once

#include <list>
#include <string>


class DNSQuery;
class DNSResponse;

class DNSResolver final
{
	using Record = std::pair<std::string, std::string>;

public:
	DNSResolver() = default;
	~DNSResolver() = default;

	DNSResolver(const DNSResolver&) = delete;
	DNSResolver& operator=(const DNSResolver&) = delete;

	void process(const DNSQuery& query, DNSResponse& response);

public:
	void loadRecordsFromFile(const std::string& filename);

	void printRecords() const;


private:
	void addLine(const std::string& line);

	std::string findAddress(const std::string& domainName) const;
	std::string findDomainName(const std::string& ipAddr) const;

	static std::string getIpAddrFromQname(const std::string& qname);

private:
	std::list<Record> _records;
};
