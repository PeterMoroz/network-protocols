#include "date_time_utils.h"


std::string timeLocalToString(std::time_t t)
{
	struct tm bt;
	char buffer[128];
	localtime_r(&t, &bt);
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &bt);
	return std::string(buffer);
}

std::string currTimeLocalToString()
{
	std::time_t t = std::time(NULL);
	return timeLocalToString(t);
}
