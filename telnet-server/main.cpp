#include <ctime>

#include <exception>
#include <iostream>


#include "server.h"
#include "session.h"
#include "date_time_utils.h"

namespace telnet_server
{

Server server("0.0.0.0", 10023);

void logIn(std::shared_ptr<Session> session, const std::string& message)
{
	AuthStatus authStatus = session->getAuthStatus();
	if (authStatus == AuthStatus::Guest)
	{
		if (message == "testuser")
		{
			server.sendMessageToClient(session, "\r\nPassword: ");
			session->setAuthStatus(AuthStatus::Authenticating);
		}
		else
		{
			server.removeClient(session);
		}
	}
	else if (authStatus == AuthStatus::Authenticating)
	{
		if (message == "testpassword")
		{
			server.clearClientScreen(session);
			server.sendMessageToClient(session, "Successfully authenticated.\r\n > ");
			session->setAuthStatus(AuthStatus::LoggedIn);
		}
		else
		{
			server.removeClient(session);
		}
	}
}

void onConnected(std::shared_ptr<Session> session)
{
	std::cout << "Connected: " << session->toString() << std::endl;
	server.sendMessageToClient(session, "Telnet Server\r\nLogin:");
}

void onDisconnected(std::shared_ptr<Session> session)
{
	std::cout << "Disconnected: " << session->toString() << std::endl;
}

void onBlocked(const std::string& addr, std::uint16_t port)
{
	std::cout << "Blocked: " << addr << ':' << port
		<< " at " << currTimeLocalToString() << std::endl;
}

void onMessageReceived(std::shared_ptr<Session> session, const std::string& message)
{
	if (session->getAuthStatus() != AuthStatus::LoggedIn)
	{
		logIn(session, message);
		return;
	}

	std::cout << "Message: " << message << std::endl;
	if (message == "exit" || message == "logout")
	{
		server.removeClient(session);
		server.sendMessageToClient(session, "\r\n > ");
	}
	else if (message == "clear")
	{
		server.clearClientScreen(session);
		server.sendMessageToClient(session, " > ");		
	}
	else
	{
		server.sendMessageToClient(session, "\r\n > ");
	}
}


}

int main(int argc, char* argv[])
{
	using namespace telnet_server;
	try
	{
		server.setConnectedCB(onConnected);
		server.setDisconnectedCB(onDisconnected);
		server.setBlockedCB(onBlocked);
		server.setMessageReceivedCB(onMessageReceived);

		server.start();
		std::cout << "Server started: " << currTimeLocalToString() << std::endl;
		char x = std::cin.get();
		do
		{
			if (x == 'b' || x == 'B')
			{
				std::string line;
				std::getline(std::cin, line);
				server.sendMessageToAll(line);
			}
			x = std::cin.get();
		}
		while (x != 'q' && x != 'Q');

		server.stop();
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
