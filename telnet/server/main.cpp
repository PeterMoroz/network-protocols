#include <cstdint>
#include <cstdlib>


#include <exception>
#include <iostream>
#include <string>

#include "telnet_server.h"

static const std::uint16_t TELNET_PORT = 10023;

void clientConnected(TelnetSession* session)
{
	std::cout << "Client connected.\n";
	session->sendLine("Welcome to My Telnet server!");
}

void lineComplete(TelnetSession* session, const std::string& line)
{
	std::cout << "Complete line received: " << line << std::endl;
	// session->sendLine("Got " + line);
}

int main(int argc, char* argv[])
{
	try
	{
		TelnetServer telnetServer;

		telnetServer.setConnectedCB(clientConnected);
		telnetServer.setLineCompleteCB(lineComplete);
		telnetServer.start("127.0.0.1", TELNET_PORT);
	}
	catch (const std::exception& ex)
	{
		std::cerr << __FILE__ << ':' << __LINE__ 
			<< " - Exception: " << ex.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::exit(EXIT_SUCCESS);
	return 0;
}
