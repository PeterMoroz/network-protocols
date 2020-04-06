#include "users_db.h"

#include <stdexcept>


UsersDB& UsersDB::getInstance()
{
	static UsersDB instance;
	return instance;
}

bool UsersDB::usernameIsKnown(const std::string& username) const
{
	return _usersWithPasswords.count(username) != 0;
}

bool UsersDB::passwordIsValid(const std::string& username, const std::string& password) const
{
	std::map<std::string, std::string>::const_iterator it = _usersWithPasswords.find(username);
	if (it == _usersWithPasswords.cend())
	{
		throw std::logic_error("UsersDB could not check password - username is unknown.");
	}
	return it->second == password;
}

void UsersDB::loadFromFile(const std::string& filename)
{
	// not implemented yet...
	_usersWithPasswords["testuser"] = "testpassword";
}
