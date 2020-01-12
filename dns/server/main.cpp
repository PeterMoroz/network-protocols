#include <cstdint>

#include <iostream>
#include <exception>

#include "dns_resolver.h"
#include "dns_server.h"

static const std::uint16_t DNS_PORT = 10053;
static const char* RECORDS_FILE = "dns-records";

int main(int argc, char* argv[])
{
	try
	{
		DNSResolver dnsResolver;
		dnsResolver.loadRecordsFromFile(RECORDS_FILE);

		DNSServer dnsServer("127.0.0.1", DNS_PORT);
		dnsServer.setResolver(&dnsResolver);
		dnsServer.start();
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}