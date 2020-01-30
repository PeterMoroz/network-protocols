#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <asio.hpp>

using asio::ip::tcp;

class TelnetServer;

class TelnetSession final
{
public:
	enum ControlCodes
	{
		//EOF   = 0xEC, // 236
		SUSP  = 0xED, // 237
		ABORT = 0xEE, // 238
		EOR   = 0xEF, // 239
		SE    = 0xF0, // 240
		NOP   = 0xF1, // 241
		DM    = 0xF2, // 242
		BRK   = 0xF3, // 243
		IP    = 0xF4, // 244
		AO    = 0xF5, // 245
		AYC   = 0xF6, // 246
		EC    = 0xF7, // 247
		EL    = 0xF8, // 248
		GA    = 0xF9, // 249
		SB    = 0xFA, // 250
		WILL  = 0xFB, // 251
		WONT  = 0xFC, // 252
		DO    = 0xFD, // 253
		DONT  = 0xFE, // 254
		IAC   = 0xFF, // 255
	};

public:
	using ConnectedCB = std::function<void (TelnetSession*)>;
	using LineCompleteCB = std::function<void (TelnetSession*, const std::string&)>;
	using ShutdownCB = std::function<void (TelnetSession*)>;

public:
	TelnetSession(const TelnetSession&) = delete;
	TelnetSession& operator=(const TelnetSession&) = delete;

public:
	explicit TelnetSession(asio::ip::tcp::socket&& socket);
	~TelnetSession();

	void setPrompt(const std::string& prompt)
	{
		// _prompt = prompt;
	}

	void setConnectedCB(const ConnectedCB& connectedCB)
	{
		_connectedCB = connectedCB;
	}

	void setLineCompleteCB(const LineCompleteCB& lineCompleteCB)
	{
		_lineCompleteCB = lineCompleteCB;
	}

	void setShutdownCB(const ShutdownCB& shutdownCB)
	{
		_shutdownCB = shutdownCB;
	}	

	void start();

	bool sendLine(const std::string& line);

private:
	static void stripNVT(std::string& s);
	static std::vector<std::string> getCompleteLines(std::string& s);

	void receive();
	bool sendEcho(std::size_t numOfBytes);
	void addToHistory(const std::string& line);

private:
	asio::ip::tcp::socket _socket;

	std::string _prompt;
	ConnectedCB _connectedCB;
	LineCompleteCB _lineCompleteCB;
	ShutdownCB _shutdownCB;

	std::string _buffer;
	static const std::size_t RECV_BUFFER_SIZE = 512;
	std::array <char, RECV_BUFFER_SIZE> _recvBuffer;
};
