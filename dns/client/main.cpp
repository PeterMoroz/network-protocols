#include <cstdlib>

#include <exception>
#include <iostream>

#include "dns_client.h"



int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "usage: " << argv[0] << " <address-to-resolve> [<address-to-resolve>] \n";
		std::exit(EXIT_FAILURE);
	}

	try
	{
		DNSClient dnsClient("8.8.8.8", 53);
		
		for (std::size_t i = 1; i < argc; i++)
		{
			std::cout << "Resolving " << argv[i] << " ...\n";
			dnsClient.resolve(argv[i]);
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << __FILE__ << ':' << __LINE__ 
			<< " Exception: " << ex.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::exit(EXIT_SUCCESS);

	return 0;
}
