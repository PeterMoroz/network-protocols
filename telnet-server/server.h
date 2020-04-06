#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <unordered_map>

#include "session.h"

#include "asio.hpp"
using asio::ip::tcp;


namespace telnet_server
{


class Server final
{
	static const std::size_t RECV_BUFFER_SIZE = 1024;

public:
	using OnConnectedCB = std::function<void (std::shared_ptr<Session>)>;
	using OnDisconnectedCB = std::function<void (std::shared_ptr<Session>)>;
	// TO DO: remove, because unnecessary method
	using OnBlockedCB = std::function<void (const std::string&, std::uint16_t)>;
	// TO DO: replace 2nd argument by std::string&& to enable moving
	using OnMessageReceivedCB = std::function<void (std::shared_ptr<Session>, const std::string&)>;

public:
	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	Server(const std::string& address, std::uint16_t port);
	~Server();

	void start();
	void stop();

	void setConnectedCB(OnConnectedCB onConnected)
	{
		_onConnected = onConnected;
	}

	void setDisconnectedCB(OnDisconnectedCB onDisconnected)
	{
		_onDisconnected = onDisconnected;
	}

	void setBlockedCB(OnBlockedCB onBlocked)
	{
		_onBlocked = onBlocked;
	}

	void setMessageReceivedCB(OnMessageReceivedCB onMessageReceived)
	{
		_onMessageReceived = onMessageReceived;
	}

	void sendMessageToAll(const std::string& message);
	void sendMessageToClient(std::shared_ptr<Session> session, const std::string& message);
	void removeClient(std::shared_ptr<Session> session);
	void clearClientScreen(std::shared_ptr<Session> session);

private:
	void accept();
	void worker();

	void handleConnection(tcp::socket&& socket);

	void sendBytesToClient(std::shared_ptr<Session> session, std::vector<std::uint8_t>&& bytes);
	void receiveAsync(std::shared_ptr<Session> session);

private:
	asio::io_context _ioContext;
	tcp::acceptor _acceptor;
	std::thread _worker;
	std::array<char, RECV_BUFFER_SIZE> _recvBuffer;

	// struct SessionHasher
	// {
	// 	std::size_t operator()(const std::shared_ptr<Session>& sp) const
	// 	{
	// 		return sp->getId();
	// 	}

	// };

	std::unordered_set<std::shared_ptr<Session>> _sessions;
	// std::unordered_map<std::shared_ptr<Session>, tcp::socket, SessionHasher> _sessions;

	OnConnectedCB _onConnected;
	OnDisconnectedCB _onDisconnected;
	OnBlockedCB _onBlocked;
	OnMessageReceivedCB _onMessageReceived;
};


}
