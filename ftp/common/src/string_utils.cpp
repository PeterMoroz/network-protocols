#include "string_utils.h"
#include <sstream>

namespace string_utils
{

std::vector<std::string> split(const std::string& s, char delim)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> result;
	while (std::getline(ss, item, delim))
	{
		result.emplace_back(item);	// resukt.push_back(std::move(item));
	}
	return result;
}

}
