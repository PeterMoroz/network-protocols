#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <exception>

#include "log.h"
#include "smtp_server.h"

static const std::uint16_t SMTP_PORT = 10025;

int main(int argc, char* argv[])
{

	try
	{
		SMTPServer smtpServer;

		smtpServer.start("0.0.0.0", SMTP_PORT);
		// smtpServer.stop();
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " Exception '" << ex.what() << "'\n";
		std::exit(-1);
	}
	return 0;
}