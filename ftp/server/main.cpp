#include <cstdlib>

#include <iostream>
#include <exception>

#include "log.h"
#include "ftp_server.h"


static const std::uint16_t FTP_CONTROL_CONNECTION_PORT = 10021;

#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
	try
	{
		FTPServer ftpServer;
		std::cout << " Starting FTP server...";
		ftpServer.start("0.0.0.0", FTP_CONTROL_CONNECTION_PORT);
		std::cout << "\t- SUCCESS\n";
		while (1)
		{
			std::this_thread::yield();
		};
		/*
		std::cout << " Starting FTP server...";
		ftpServer.start("0.0.0.0", FTP_CONTROL_CONNECTION_PORT);
		std::cout << "\t- SUCCESS\n";

		std::this_thread::sleep_for(std::chrono::seconds(30));

		std::cout << " Stopping FTP server...";
		ftpServer.stop();
		std::cout << "\t- SUCCESS\n";

		std::this_thread::sleep_for(std::chrono::seconds(10));

		std::cout << " Starting FTP server...";
		ftpServer.start("0.0.0.0", FTP_CONTROL_CONNECTION_PORT);
		std::cout << "\t- SUCCESS\n";

		std::this_thread::sleep_for(std::chrono::seconds(30));

		std::cout << " Stopping FTP server...";
		ftpServer.stop();
		std::cout << "\t- SUCCESS\n";
		*/
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " Exception '" << ex.what() << "'\n";
		std::exit(-1);
	}

	return 0;
}
