#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>


template <typename SessionType>
class SessionsManager final
{
public:
	SessionsManager() = default;
	~SessionsManager() = default;

	void startSession(std::unique_ptr<SessionType>&& session)
	{
		std::uint32_t sessionId = session->getId();
		_sessions[sessionId] = std::move(session);
		_sessions[sessionId]->start();
	}

	void closeSession(std::uint32_t id)
	{
		// async remove session from container
	}

private:
	std::unordered_map<std::uint32_t, std::unique_ptr<SessionType>> _sessions;
};
