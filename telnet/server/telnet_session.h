#pragma once

#include <array>
#include <functional>
#include <list>
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
		NUL            = 0,
		BELL           = 7,
		BackSpace      = 8,
		HorizontalTab  = 9,
		LineFeed       = 10,
		VerticalTab    = 11,
		FormFeed       = 12,
		CarriageReturn = 13,
	};

	enum Commands
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

	enum Options
	{
		TransmitBinary       = 0,
		Echo                 = 1,
		SuppressGoAhead      = 3,
		Status               = 5,
		TimingMark           = 6,
		TerminalType         = 24,
		WindowSize           = 31,
		TerminalSpeed        = 32,
		RemoteFlowControl    = 33,
		LineMode             = 34,
		EnvironmentVariables = 36,
	};

private:
	enum State
	{
		Negotiation,
		Authentication,
		Interaction
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
	void receive();
	bool send(std::uint8_t b);
	bool send(const std::uint8_t* data, std::size_t size);
	void addToHistory(const std::string& line);

	void startAuthentication();
	void startInteraction();

	bool authorize();

	void sendEraseLine();
	void processEscapeSequence(const std::string& escSequence);

private:
	asio::ip::tcp::socket _socket;

	std::string _prompt;
	ConnectedCB _connectedCB;
	LineCompleteCB _lineCompleteCB;
	ShutdownCB _shutdownCB;

	std::string _username;
	std::string _password;

	State _state = Negotiation;
	std::list<std::uint8_t> _optionsToConfirm;

	std::string _buffer;
	static const std::size_t MAX_HISTORY_LEGTH = 80;
	std::vector<std::string> _history;
	std::size_t _historyPos = 0;
	static const std::size_t RECV_BUFFER_SIZE = 512;
	std::array <std::uint8_t, RECV_BUFFER_SIZE> _recvBuffer;
};
