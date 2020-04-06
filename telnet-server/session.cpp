#include <sstream>

#include "session.h"
#include "date_time_utils.h"


using namespace telnet_server;

namespace
{

std::string authStatusToString(AuthStatus authStatus)
{
	switch (authStatus)
	{
	case AuthStatus::Guest:
		return "Guest";
	case AuthStatus::Authenticating:
		return "Authenticating";
	case AuthStatus::LoggedIn:
		return "LoggedIn";
	default:
		return "Unknown";
	}
	return "None";
}

}

Session::Session(std::uint32_t id, tcp::socket&& socket)
	: _id(id)
	, _socket(std::move(socket))
	, _remoteEp(_socket.remote_endpoint())
	, _authStatus(AuthStatus::Guest)
	, _receivedData()
	, _connectionTime(std::time(NULL))
{

}

std::string Session::toString() const
{
	std::ostringstream oss;
	oss << " session #" << _id << " (from " << _remoteEp.address() << ':' << _remoteEp.port() << "), auth status "
		<< authStatusToString(_authStatus) << ", connected at " << timeLocalToString(_connectionTime);
	return oss.str();
}
