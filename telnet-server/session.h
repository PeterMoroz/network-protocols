#pragma once

#include <ctime>
#include <cstdint>

#include <string>


#include "asio.hpp"
using asio::ip::tcp;


namespace telnet_server
{

	enum class AuthStatus
	{
		Guest = 0,
		Authenticating,
		LoggedIn,
	};


	class Session final
	{

	public:
		Session(const Session&) = delete;
		Session& operator=(const Session&) = delete;

		Session(std::uint32_t id, tcp::socket&& socket);

		std::uint32_t getId() const { return _id; }
		tcp::socket& getSocket() { return _socket; }
		const tcp::endpoint& getRemoteEp() const { return _remoteEp; }
		AuthStatus getAuthStatus() const { return _authStatus; }
		const std::string& getReceivedData() const { return _receivedData; }
		std::time_t getConnectionTime() const { return _connectionTime; }

		void setAuthStatus(AuthStatus status)
		{
			_authStatus = status;
		}

		void setReceivedData(const std::string& data)
		{
			_receivedData = data;
		}

		void appendReceivedData(const std::string& data)
		{
			_receivedData.append(data);
		}

		void appendReceivedData(const char* data, std::size_t size)
		{
			_receivedData.append(data, size);
		}

		void clearReceivedData()
		{
			_receivedData.clear();
		}

		void removeLastReceivedCharacter()
		{
			_receivedData = _receivedData.substr(0, _receivedData.length() - 1);
		}

		std::string toString() const;

	private:
		std::uint32_t _id;
		tcp::socket _socket;
		tcp::endpoint _remoteEp;
		AuthStatus _authStatus;
		std::string _receivedData;
		std::time_t _connectionTime;
	};

}