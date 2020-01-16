#include "dns_resolver.h"

#include <cctype>
#include <algorithm>
#include <fstream>
#include <iostream>

#include "dns_query.h"
#include "dns_response.h"

void DNSResolver::process(const DNSQuery& query, DNSResponse& response)
{
	const std::string qname(query.getName());

	// TO DO: check, if qname ends with suffix .in-addr.arpa
	std::string rdata;

	const std::string ipAddr(getIpAddrFromQname(qname));
	if (!ipAddr.empty())
	{
		rdata = findDomainName(ipAddr);		// const std::string domainName(findDomainName(ipAddr));
	}
	else
	{
		rdata = findAddress(qname);
	}

	std::cout << "Processing DNS query: "
		<< "\nrequested name " << qname
		<< "\nresponse data " << rdata
		<< "\n--------------" << std::endl;

	response.setId(query.getId());
	response.setQCount(1);
	response.setACount(1);
	response.setName(qname);
	response.setType(query.getType());
	response.setClass(query.getClass());
	response.setData(rdata); // response.setData(domainName);

	if (rdata.empty())
	{
		response.setRCode(DNSResponse::Rcode::NameError);
	}
	else
	{
		response.setRCode(DNSResponse::Rcode::NoError);
	}

}

void DNSResolver::loadRecordsFromFile(const std::string& filename)
{
	std::ifstream inFile(filename);
	if (!inFile)
	{
		std::cerr << "ERROR ( DNSResolver::loadRecordsFromFile() ): DNS resolver could not open file '" << filename << "'\n";
		return;
	}

	std::string line;
	while (std::getline(inFile, line))
	{
		addLine(line);
	}
}

// std::string DNSResolver::lookup(const std::string& ipAddr) const
// {
// 	std::list<Record>::const_iterator it = 
// 		std::find_if(_records.cbegin(), _records.cend(), 
// 			[&ipAddr](const Record& record)
// 			{ return record.first == ipAddr; });
// 	return it != _records.cend() ? it->second : std::string();
// }

void DNSResolver::addLine(const std::string& line)
{
	std::size_t p = line.find(' ');
	if (p == std::string::npos)
	{
		std::cerr << "ERROR ( DNSResolver::addLine() ): Invalid line " << line << std::endl;
		return;
	}

	const std::string ipAddr(line.substr(0, p));

	while (p < line.length() && std::isspace(line[p]))
	{
		p += 1;
	}

	if (p == line.length())
	{
		std::cerr << "ERROR ( DNSResolver::addLine() ): Invalid line " << line << std::endl;
		return;
	}

	const std::string domainName(line.substr(p));

	_records.emplace_back(std::make_pair(ipAddr, domainName));
}

std::string DNSResolver::findAddress(const std::string& domainName) const
{
	std::list<Record>::const_iterator it = 
		std::find_if(_records.cbegin(), _records.cend(), 
			[&domainName](const Record& record)
			{ return record.second == domainName; });
	return it != _records.cend() ? it->first : std::string();
}

std::string DNSResolver::findDomainName(const std::string& ipAddr) const
{
	std::list<Record>::const_iterator it = 
		std::find_if(_records.cbegin(), _records.cend(), 
			[&ipAddr](const Record& record)
			{ return record.first == ipAddr; });
	return it != _records.cend() ? it->second : std::string();
}

std::string DNSResolver::getIpAddrFromQname(const std::string& qname)
{
	std::size_t p = qname.find(".in-addr.arpa");
	if (p == std::string::npos)
	{
		return std::string();
	}

	std::string ipAddr;
	std::string tmp(qname, 0, p);
	while ((p = tmp.rfind('.')) != std::string::npos)
	{
		ipAddr.append(tmp, p + 1);
		ipAddr.push_back('.');
		tmp.erase(p);
	}
	ipAddr.append(tmp);

	return ipAddr;
}

void DNSResolver::printRecords() const
{
	std::cout << "TRACE ( DNSResolver::printRecords() ) Records, known to DNS resolver:\n";
	for (const Record& record : _records)
	{
		std::cout << record.first << " ---- " << record.second << std::endl;
	}
	std::cout << "-------------------------------\n";
}
