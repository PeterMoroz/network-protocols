#include "ftp_session.h"
#include "log.h"
#include "string_utils.h"
#include "users_db.h"


#include <cctype>
#include <ctime>

#include <stdlib.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>


FTPSession::FTPSession(tcp::socket ctrlSocket, std::uint16_t dataPort, const std::string& rootDir)
	: _ctrlSocket(std::move(ctrlSocket))
	, _dataPort(dataPort)
	, _ioContext(1)
	, _dataSocket(_ioContext)
	, _acceptor(_ioContext)
	, _rootDir(rootDir)
	, _currDir(rootDir)
	, _worker()
{

}

FTPSession::~FTPSession()
{
	try
	{
		_quitCmdLoop = true;
		_worker.join();
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
	}
}

void FTPSession::start()
{
	_worker = std::thread(&FTPSession::worker, this);
}

void FTPSession::worker()
{
	std::cout << "session worker started.\n";
	try
	{
		sendMessageToClient("220 myhost.mydomain FTP-server (version 1.0) ready");
		asio::streambuf sb;
		while (!_quitCmdLoop)
		{
			asio::streambuf sb;
			std::error_code ec;
			asio::read_until(_ctrlSocket, sb, "\r\n", ec);
			if (ec)
			{
				LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
			}
			else
			{			
				std::ostringstream oss;
				oss << &sb;
				std::string cmd(oss.str());
				std::size_t pos = cmd.find("\r\n");
				if (pos != std::string::npos)
				{
					cmd = cmd.substr(0, pos);
				}
				executeCommand(cmd);
			}

			if (ec.value() == 2)	// end of file, opposite side closed connection
			{
				break;
			}
		}
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
	}	

	std::error_code ec;

	_ctrlSocket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
	}

	_ctrlSocket.close(ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
	}	

	std::cout << "session worker finished.\n";
}

void FTPSession::sendMessageToClient(const std::string& msg)
{
	std::string message(msg);
	message += "\r\n";

	std::error_code ec;
	asio::write(_ctrlSocket, asio::buffer(message.c_str(), message.length()), ec);
	if (ec)
	{
		LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
	}
}

void FTPSession::sendAsciiDataToClient(const std::string& data)
{
	std::string s(data);
	s.append("\r\n");

	std::error_code ec;
	asio::write(_dataSocket, asio::buffer(s.c_str(), s.length()), ec);
	if (ec)
	{
		LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
	}
}

bool FTPSession::sendDataToClient(const char* data, std::size_t amount)
{
	std::error_code ec;
	asio::write(_dataSocket, asio::buffer(data, amount), ec);
	if (ec)
	{
		LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		return false;
	}
	return true;
}

void FTPSession::executeCommand(const std::string& cmd)
{
	std::size_t p = cmd.find_first_of(' ');

	std::string command;
	std::string args;

	if (p != std::string::npos)
	{
		command = cmd.substr(0, p);
		args = cmd.substr(p + 1);
	}
	else
	{
		command = cmd;
	}

	std::transform(command.begin(), command.end(), command.begin(),
		[](char x){ return static_cast<char>(std::toupper(x)); });

	if (command == "CWD")
	{
		handleCwd(args);
	}
	else if (command == "DELE")
	{
		handleDele(args);
	}
	else if (command == "EPRT")
	{
		const std::string extendedArgs(parseExtendedArguments(args));
		handlePort(extendedArgs);
	}
	else if (command == "EPSV")
	{
		handleEpsv();
	}
	else if (command == "FEAT")
	{
		handleFeat();
	}
	else if (command == "LIST")
	{
		handleNlst(args);
	}
	else if (command == "NLST")
	{
		handleNlst(args);
	}
	else if (command == "MKD")
	{
		handleMkd(args);
	}
	else if (command == "PASS")
	{
		handlePass(args);
	}
	else if (command == "PASV")
	{
		handlePasv(args);
	}
	else if (command == "PORT")
	{
		std::size_t pos = args.find_first_of('(');
		if (pos != std::string::npos)
		{
			args = args.substr(pos + 1);
		}

		pos = args.find_first_of(')');
		if (pos != std::string::npos)
		{
			args = args.substr(0, pos);
		}

		handlePort(args);
	}
	else if (command == "PWD")
	{
		handlePwd();
	}
	else if (command == "QUIT")
	{
		handleQuit();
	}
	else if (command == "RETR")
	{
		handleRetr(args);
	}
	else if (command == "RMD")
	{
		handleRmd(args);
	}
	else if (command == "SYST")
	{
		handleSyst();
	}
	else if (command == "STOR")
	{
		handleStor(args);
	}
	else if (command == "TYPE")
	{
		handleType(args);
	}
	else if (command == "USER")
	{
		handleUser(args);
	}
	else 
	{
		sendMessageToClient("500 Unknown command");
	}
}


void FTPSession::handleCwd(const std::string& args)
{
	namespace fs = std::experimental::filesystem;
	fs::path currDir = _currDir;

	if (args == "..")
	{
		currDir = currDir.parent_path();
	}
	else if (!args.empty() && args != ".")
	{
		currDir /= args;
	}

	if (fs::exists(currDir) && fs::is_directory(currDir))
	{
		_currDir = currDir;
		std::string msg("250 The current directory has been changed to ");
		msg.append(currDir);
		sendMessageToClient(msg);
	}
	else
	{
		sendMessageToClient("550 Requested action not taken, directory unavailable");
	}
}

void FTPSession::handleDele(const std::string& args)
{
	namespace fs = std::experimental::filesystem;

	std::regex fname("(.+?)(\\.[^.]*$|$)");
	if (!args.empty() && std::regex_match(args, fname))
	{
		fs::path filePath = _currDir / args;
		std::error_code ec;

		if (fs::exists(filePath) && fs::is_regular_file(filePath))
		{
			if (fs::remove(filePath, ec))
			{
				sendMessageToClient("250 file successfully deleted");
			}
			else
			{
				sendMessageToClient("550 could not delete file");
				LOG_ERROR() << "Could not delete file '" << args << "'\n";
			}
		}
		else
		{
			sendMessageToClient("550 File doesn't exist");
			LOG_ERROR() << " - Error 'File '" << args << "' doesn't exist or isn't a regular file'\n";
		}
	}
	else
	{
		sendMessageToClient("550 Invalid name");
	}
}

void FTPSession::handleEpsv()
{
	sendMessageToClient("229 Entering Extended Passive Mode (|||" + std::to_string(_dataPort) + "|)");
	if (!openDataConnectionPassive(_dataPort))
	{
		LOG_ERROR() << " - Error 'Could not open data connection (passive mode)'\n";
	}
}

void FTPSession::handleFeat()
{
	sendMessageToClient("211-Features supported:");
	sendMessageToClient(" nothing essential now ");
	sendMessageToClient("211 End");
}

void FTPSession::handleMkd(const std::string& args)
{
	namespace fs = std::experimental::filesystem;

	std::regex fname("^[a-zA-Z0-9]+$");
	if (!args.empty() && std::regex_match(args, fname))
	{
		fs::path dir = _currDir / args;
		std::error_code ec;
		if (fs::create_directory(dir, ec))
		{
			sendMessageToClient("250 Directory successfully created");
		}
		else
		{
			sendMessageToClient("550 Could not create directory");
			LOG_ERROR() << " - Error 'Could not create directory: '" << args << "''\n";
		}
	}
	else
	{
		sendMessageToClient("550 Invalid name");
	}
}

void FTPSession::handleNlst(const std::string& args)
{
	if (!_dataSocket.is_open())
	{
		sendMessageToClient("425 No data connection");
		return;
	}

	try
	{
		std::list<std::string> dirContent(readDirectory(args));

		if (dirContent.empty())
		{
			sendMessageToClient("550 The directory does not exist");
			closeDataConnection();
			return;
		}

		sendMessageToClient("125 Data connection is ready to transfer files list");
		for (const std::string& s : dirContent)
		{
			sendAsciiDataToClient(s);
		}

		sendMessageToClient("226 Transfer complete");
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR() << " - Exception '" << ex.what() << "'\n";
	}

	closeDataConnection();
}

void FTPSession::handlePass(const std::string& args)
{
	std::string password(args);
	if (!_username.empty())
	{
		if (UsersDB::getInstance().passwordIsValid(_username, password))
		{
			sendMessageToClient("230 User " + _username + " logged in");
			return;
		}
		else
		{
			sendMessageToClient("530 login authentication failed");
			return;	
		}
	}
}

void FTPSession::handlePasv(const std::string& args)
{
	const std::string addr("127.0.0.1");	// TO DO: obtain IP address of any physical interface
	std::vector<std::string> h = string_utils::split(addr, '.');

	std::uint16_t p1 = _dataPort / 256;
	std::uint16_t p2 = _dataPort % 256;

	std::ostringstream oss;
	oss << "227 Entering Passive Mode (" 
		<< h[0] << ',' << h[1] << ',' << h[2] << ',' << h[3] << ',' << p1 << ',' << p2 << ").";

	sendMessageToClient(oss.str());

	if (!openDataConnectionPassive(_dataPort))
	{
		LOG_ERROR() << " - Error 'Could not open data connection (passive mode)'\n";
	}
}

void FTPSession::handlePort(const std::string& args)
{
	std::vector<std::string> elements = string_utils::split(args, ',');
	std::string host = elements[0] + "." + elements[1] + "." + elements[2] + "." + elements[3];
	std::uint16_t port = std::stoi(elements[4]) * 256 + std::stoi(elements[5]);

	if (openDataConnectionActive(host, port))
	{
		sendMessageToClient("200 PORT command successful");
	}
	else
	{
		LOG_ERROR() << " - Error 'Could not open data connection (active mode)'\n";
	}
}

void FTPSession::handlePwd()
{
	std::string msg("257 \"");
	msg.append(_currDir);
	msg.append("\"");
	sendMessageToClient(msg);
}

void FTPSession::handleQuit()
{
	sendMessageToClient("221 Goodbye.");
	_quitCmdLoop = true;
}


void FTPSession::handleRetr(const std::string& args)
{
	if (!_dataSocket.is_open())
	{
		sendMessageToClient("425 No data connection");
		return;
	}

	namespace fs = std::experimental::filesystem;

	fs::path path = _currDir / args;
	if (!fs::exists(path))
	{
		sendMessageToClient("550 File does not exist");
		return;
	}

	bool transferSuccess = true;	

	if (_transferType == TransferType_Binary)
	{
		sendMessageToClient("150 Data connection (binary mode) is ready to transfer file");

		static const std::size_t BUFFER_SIZE = 1024;
		char buffer[BUFFER_SIZE];

		std::ifstream inFile(path.string(), std::fstream::binary);
		std::int32_t n = inFile.readsome(buffer, BUFFER_SIZE);
		while (n > 0)
		{
			if (!sendDataToClient(buffer, n))
			{
				// send error message (with reason explanation)
				transferSuccess = false;
				break;
			}
			n = inFile.readsome(buffer, BUFFER_SIZE);
		}
	}
	else
	{
		sendMessageToClient("150 Data connection (ASCII mode) is ready to transfer file.");

		std::ifstream inFile(path.string());
		std::string line;

		while (std::getline(inFile, line))
		{
			line.append("\r\n");
			if (!sendDataToClient(line.c_str(), line.length()))
			{
				// send error message (with reason explanation)
				transferSuccess = false;
				break;
			}
		}
	}

	if (transferSuccess)
	{
		sendMessageToClient("226 The file transferred successfully, closing data connection");
	}

	closeDataConnection();
}

void FTPSession::handleRmd(const std::string& args)
{
	namespace fs = std::experimental::filesystem;

	std::regex fname("^[a-zA-Z0-9]+$");
	if (!args.empty() && std::regex_match(args, fname))
	{
		fs::path dir = _currDir / args;
		std::error_code ec;

		if (fs::exists(dir, ec) && fs::is_directory(dir, ec))
		{
			if (fs::remove(dir, ec))
			{
				sendMessageToClient("250 Directory successfully removed");
			}
			else
			{
				if (ec)
				{
					LOG_ERROR() << "Could not remove directory: '" << args << "' because of "
								<< ec.message() << " (" << ec.value() << ")\n";
				}
				sendMessageToClient("550 Could not remove directory");
			}
		}
		else
		{
			if (ec)
			{
				LOG_ERROR() << "Could check existense of directory: '" << args << "' because of "
						<< ec.message() << " (" << ec.value() << ")\n";
			}
			sendMessageToClient("550 Directory not exist or given name is not directory");
		}
	}
	else
	{
		sendMessageToClient("550 Invalid name");
	}
}

void FTPSession::handleSyst()
{
	sendMessageToClient("215 UNIX Type: L8");
}

void FTPSession::handleStor(const std::string& args)
{
	if (!_dataSocket.is_open())
	{
		sendMessageToClient("425 No data connection");
		return;
	}

	namespace fs = std::experimental::filesystem;

	fs::path path = _currDir / args;
	bool transferSuccess = true;

	if (_transferType == TransferType_Binary)
	{
		sendMessageToClient("150 Data connection (binary mode) is ready to transfer file");

		std::ofstream outFile(path.string(), std::fstream::binary);
		// TO DO: check that file is opened

		std::error_code ec;
		std::size_t n = 0;
		do
		{
			std::array<char, 512> buffer;
			n = _dataSocket.read_some(asio::buffer(buffer, 512), ec);
			if (ec)
			{
				LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
				transferSuccess = false;
				break;
			}

			outFile.write(buffer.data(), n);
		} while (n == 512);
	}
	else		
	{
		sendMessageToClient("150 Data connection (ASCII mode) is ready to transfer file");

		std::ofstream outFile(path.string());
		// TO DO: check that file is opened

		std::error_code ec;
		std::size_t n = 0;
		do
		{
			std::array<char, 512> buffer;
			n = _dataSocket.read_some(asio::buffer(buffer, 512), ec);
			if (ec)
			{
				LOG_ERROR() << " - Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
				transferSuccess = false;
				break;
			}

			for (std::size_t i = 0; i < n; i++)
			{
				if (buffer[i] == '\r')
				{
					continue;
				}
				outFile.put(buffer[i]);
			}
		} while (n == 512);
	}

	if (transferSuccess)
	{
		sendMessageToClient("226 The file transferred successfully, closing data connection");
	}
	else
	{
		sendMessageToClient("446 Transfer failed");
	}

	closeDataConnection();
}

void FTPSession::handleType(const std::string& args)
{
	if (args.length() == 1 && std::toupper(args[0]) == 'A')
	{
		_transferType = TransferType_ASCII;
		sendMessageToClient("200 Switched to ASCII mode");
	}
	else if (args.length() == 1 && std::toupper(args[0]) == 'I')
	{
		_transferType = TransferType_Binary;
		sendMessageToClient("200 Switched to Binary mode");
	}
	else
	{
		sendMessageToClient("504 Invalid argument");
	}
}

void FTPSession::handleUser(const std::string& args)
{
	std::string username(args);
	if (UsersDB::getInstance().usernameIsKnown(username))
	{
		sendMessageToClient("331 Password required for " + username);
		_username = username;
		return;
	}
	else
	{
		std::cout << "Unknown username: " << username << std::endl;
	}
}

bool FTPSession::openDataConnectionActive(const std::string& addr, std::uint16_t port)
{
	tcp::endpoint ep(asio::ip::make_address(addr), port);
	std::error_code ec;
	_dataSocket.connect(ep, ec);

	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		return false;
	}

	return true;
}


bool FTPSession::openDataConnectionPassive(std::uint16_t port)
{
	if (_dataSocket.is_open())
	{
		LOG_ERROR() << " Error - Could not open passive data connection, because data socket still opened.\n";
		return false;
	}

	std::error_code ec;
	_acceptor.open(tcp::v4(), ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		return false;
	}

	_acceptor.set_option(tcp::socket::reuse_address(true), ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		return false;
	}		

	_acceptor.bind(tcp::endpoint(tcp::v4(), port), ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		return false;
	}

	_acceptor.listen(asio::socket_base::max_listen_connections, ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		return false;
	}

	_acceptor.accept(_dataSocket, ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";		
		return false;
	}

	return true;
}

void FTPSession::closeDataConnection()
{
	std::error_code ec;

	_dataSocket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
	}

	_dataSocket.close(ec);
	if (ec)
	{
		LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
	}

	if (_acceptor.is_open())
	{
		_acceptor.close(ec);
		if (ec)
		{
			LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		}
	}
}

std::string FTPSession::parseExtendedArguments(const std::string& args)
{
	return args;
}

std::list<std::string> FTPSession::readDirectory(const std::string& dirName)
{
	std::list<std::string> result;

	namespace fs = std::experimental::filesystem;

	fs::path path = _currDir / dirName;

	std::error_code ec;

	if (!fs::exists(path, ec) || !fs::is_directory(path, ec))
	{
		if (ec)
		{
			LOG_ERROR() << " Error '" << ec.message() << '(' << ec.value() << ')' << "'\n";
		}
		return result;
	}

	for (const fs::directory_entry& entry : fs::directory_iterator(path))
	{
		std::ostringstream oss;

		try
		{
			const std::string name(entry.path().filename());
			fs::file_time_type ftime = fs::last_write_time(entry);

			std::time_t ft = fs::file_time_type::clock::to_time_t(ftime);

			if (fs::is_regular_file(entry.path()))
			{
				std::uintmax_t size = fs::file_size(entry);
				oss << std::setw(16) << std::setfill('0') << size
					<< '\t' << std::put_time(std::gmtime(&ft), "%c %Z")
					<< '\t' << name;
			}
			else if (fs::is_directory(entry.path()))
			{
				oss << " -- directory    "
					<< '\t' << std::put_time(std::gmtime(&ft), "%c %Z")
					<< '\t' << name;
			}
			else
			{
				oss << "                "
					<< '\t' << std::put_time(std::gmtime(&ft), "%c %Z")
					<< '\t' << name;
			}
			result.emplace_back(oss.str());
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR() << "Exception - '" << ex.what() << "'\n";
			continue;
		}
	}
	return result;
}
