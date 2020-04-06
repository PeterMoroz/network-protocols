#pragma once

#include <map>
#include <string>

class UsersDB final
{
public:
	~UsersDB() = default;
	UsersDB(const UsersDB&) = delete;
	const UsersDB& operator=(const UsersDB&) = delete;

private:
	UsersDB() = default;	

public:
	static UsersDB& getInstance();

	bool usernameIsKnown(const std::string& username) const;
	bool passwordIsValid(const std::string& username, const std::string& password) const;

	void loadFromFile(const std::string& filename);

private:
	std::map<std::string, std::string> _usersWithPasswords;
};