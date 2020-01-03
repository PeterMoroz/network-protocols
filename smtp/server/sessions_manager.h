#pragma once

#include <cstdint>

template <typename SessionType>
class SessionsManager
{
public:
	virtual ~SessionsManager() {}

	virtual void startSession() = 0;
	virtual void closeSession(std::uint32_t id) = 0;
};
