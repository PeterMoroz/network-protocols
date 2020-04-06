#pragma once

#include <cstdint>

#include <experimental/filesystem>
#include <list>
#include <thread>

#include <asio.hpp>


using asio::ip::tcp;

class FTPSession final
{
	enum TransferType
	{
		TransferType_ASCII,
		TransferType_Binary,
	};


public:
	FTPSession(tcp::socket ctrlSocket, std::uint16_t dataPort, const std::string& rootDir);
	~FTPSession();

	FTPSession(const FTPSession&) = delete;
	FTPSession& operator=(const FTPSession&) = delete;

	void start();

private:
	void worker();

	void sendMessageToClient(const std::string& msg);
	void sendAsciiDataToClient(const std::string& data);
	bool sendDataToClient(const char* data, std::size_t amount);
	void executeCommand(const std::string& cmd);

	void handleCwd(const std::string& args);
	void handleDele(const std::string& args);
	void handleEpsv();
	void handleFeat();
	void handleMkd(const std::string& args);
	void handleNlst(const std::string& args);
	void handlePass(const std::string& args);
	void handlePasv(const std::string& args);
	void handlePort(const std::string& args);
	void handlePwd();
	void handleQuit();
	void handleRetr(const std::string& args);
	void handleRmd(const std::string& args);
	void handleSyst();
	void handleStor(const std::string& args);
	void handleType(const std::string& args);
	void handleUser(const std::string& args);

	bool openDataConnectionActive(const std::string& addr, std::uint16_t port);
	bool openDataConnectionPassive(std::uint16_t port);
	void closeDataConnection();
	std::string parseExtendedArguments(const std::string& args);

	std::list<std::string> readDirectory(const std::string& dirName);


private:
	tcp::socket _ctrlSocket;
	std::uint16_t _dataPort;

	asio::io_context _ioContext;
	tcp::socket _dataSocket;
	tcp::acceptor _acceptor;

	std::experimental::filesystem::path _rootDir;
	std::experimental::filesystem::path _currDir;

	std::string _username;

	bool _quitCmdLoop = false;
	TransferType _transferType = TransferType_Binary;

	std::thread _worker;
};
