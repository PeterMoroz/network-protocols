#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <asio/ip/udp.hpp>


#include <csignal>
#include <cstdint>

#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

using asio::ip::udp;


namespace TFTP
{

	static const std::uint16_t PORT = 10069;
	static const std::size_t BUFFER_SIZE = 516;	

    enum class TransferMode
    {
        Binary,
        ASCII
    };


    enum class MessageType : std::uint16_t
    {
        RRQ = 1,
        WRQ,
        DATA,
        ACK,
        ERROR,
        OACK,
    };

    enum class ErrorCode : std::uint16_t
    {
        NoError = 0,
        FileNotFound,
        AccessViolation,
        DiskFull,
        IllegalOperation,
        UnknownTransferId,
        FileAlreadyExists,
        NoSuchUser,
        TerminateTransfer,
    };

	using Buffer = std::array<char, BUFFER_SIZE>;

	Buffer makeErrorMessage(ErrorCode code, const std::string& text);
}


class TFTPSession
{
public:
	enum class State : std::uint8_t
	{
		Undefined = 0,
		Active,
		Failed,
		Finished,
	};

public:
	TFTPSession(udp::socket& socket,
				const udp::endpoint& ep);
	virtual ~TFTPSession();

	virtual void onDataReceived(const TFTP::Buffer& data, std::size_t n) = 0;

	State getState() const { return _state; }

protected:
	TFTP::TransferMode getTrasferMode() const { return _transferMode; }
	void setTransferMode(TFTP::TransferMode transferMode)
	{
		_transferMode = transferMode;
	}

	std::uint16_t getBlockCounter() const { return _blockCounter; }
	void incrementBlockCounter() { _blockCounter += 1; }

	void setState(State state) { _state = state; }

	bool send(const TFTP::Buffer& dataBuffer, std::size_t bytesCount);

private:
	udp::socket& _socket;
	udp::endpoint _ep;
	TFTP::TransferMode _transferMode = TFTP::TransferMode::ASCII;
	std::uint16_t _blockCounter = 0;
	State _state = State::Undefined;
};


class TFTPSessionRead final : public TFTPSession
{
public:
	TFTPSessionRead(udp::socket& socket,
				const udp::endpoint& ep);

	void onDataReceived(const TFTP::Buffer& data, std::size_t n) override;

private:
	bool sendBlockOfData();

private:
	std::ifstream _inFile;
	bool _readComplete = false;
	bool _lineFeed = false;
	TFTP::Buffer _sendBuffer;
	std::size_t _sendBytesCount = 0;
};

class TFTPSessionWrite final : public TFTPSession
{
public:
	TFTPSessionWrite(udp::socket& socket,
				const udp::endpoint& ep);

	void onDataReceived(const TFTP::Buffer& data, std::size_t n) override;

private:
	bool sendAcknowledge();

private:
	std::ofstream _outFile;
	TFTP::Buffer _sendBuffer;
	std::size_t _sendBytesCount = 0;
};



class TFTPServer final
{

public:
	TFTPServer(const TFTPServer&) = delete;
	TFTPServer& operator=(const TFTPServer&) = delete;

	TFTPServer();
	~TFTPServer();

	void start(const std::string& addr, std::uint16_t port, bool reuse = false);
	void stop();

private:
	void receive();
    void waitSignal();

private:
	asio::io_context _ioContext;
    asio::signal_set _signal;
	udp::socket _socket;
	udp::endpoint _remoteEndpoint;

	struct UDP_Endpoint_Hash
	{
	public:
		std::size_t operator()(const udp::endpoint& ep) const
		{
			return std::hash<decltype(ep.port())>()(ep.port())
				+ std::hash<std::string>()(ep.address().to_string());
		}
	};

	// struct UDP_Endpoint_Equal 
	// 	: std::binary_function<const udp::endpoint&, const udp::endpoint&, bool>
	// {
	// public:
	// 	result_type operator()(first_argument_type& lhs, second_argument_type& rhs) const
	// 	{
	// 		return true;
	// 	}
	// };

	using Sessions = std::unordered_map<udp::endpoint, std::unique_ptr<TFTPSession>, UDP_Endpoint_Hash>;
	Sessions _sessions;

	TFTP::Buffer _recvBuffer;
	TFTP::Buffer _sendBuffer;
	std::size_t _sendBytesCount = 0;
};

namespace TFTP
{

TFTP::Buffer makeErrorMessage(ErrorCode code, const std::string& text)
{
    std::ostringstream oss;

    static const std::uint16_t ERROR_OPCODE = static_cast<std::uint16_t>(TFTP::MessageType::ERROR);    
    oss.write(reinterpret_cast<const char*>(&ERROR_OPCODE), 2);

    const std::uint16_t ERROR_CODE = static_cast<std::uint16_t>(code);
    oss.write(reinterpret_cast<const char*>(&ERROR_CODE), 2);

    std::size_t n = text.length() < 512 ? text.length() : 511;
    oss.write(text.c_str(), n);
    oss.put('\0');

    TFTP::Buffer buffer;
    n = oss.str().length() + 1;
    std::copy_n(oss.str().cbegin(), n, buffer.begin());
    return buffer;
}

}




///////////////////////////////////////////////////////////////////////////////////////////////////
// TFTPSession implementation
TFTPSession::TFTPSession(udp::socket& socket, const udp::endpoint& ep)
	: _socket(socket)
	, _ep(ep)
{

}

TFTPSession::~TFTPSession()
{

}

bool TFTPSession::send(const TFTP::Buffer& dataBuffer, std::size_t bytesCount)
{
    asio::error_code ec;
    std::size_t n = _socket.send_to(asio::const_buffer(dataBuffer.data(), bytesCount), _ep, 0, ec);
    if (ec)
    {
        std::cerr << "Error when sending data (" << _ep.address().to_string()
        		 << ':' << _ep.port() << ") " << ec.message() << '(' << ec.value() << ')' << std::endl;
        return false;
    }
    return true;	
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TFTPSessionRead implementation
TFTPSessionRead::TFTPSessionRead(udp::socket& socket, const udp::endpoint& ep)
	: TFTPSession(socket, ep)
{

}

void TFTPSessionRead::onDataReceived(const TFTP::Buffer& data, std::size_t amount)
{
    std::uint16_t msgType = 0;
    std::memcpy(&msgType, data.data(), sizeof(msgType));
    std::size_t bytesCount = sizeof(msgType);

    if (msgType == static_cast<std::uint16_t>(TFTP::MessageType::RRQ))
    {
    	setState(State::Active);

    	const std::string filename(data.cbegin() + bytesCount);
    	if (filename.empty())
    	{
    		std::cerr << "Error: No filename provided (command RRQ)\n";
    		return;
    	}

    	bytesCount += filename.length() + 1;
    	const std::string mode(data.cbegin() + bytesCount);
    	if (mode == "octet")
    	{
    		setTransferMode(TFTP::TransferMode::Binary);
    	}
    	else if (mode == "netascii")
    	{
    		setTransferMode(TFTP::TransferMode::ASCII);
    	}
    	else
    	{          
            std::cerr << "Error: invalid transfer mode (command RRQ)\n";
    		return ;
    	}

    	if (getTrasferMode() == TFTP::TransferMode::Binary)
    	{
    		_inFile.open(filename, std::ios::binary);
    	}
    	else
    	{
    		_inFile.open(filename);
    	}

    	if (!_inFile)
    	{
    		const TFTP::Buffer message(TFTP::makeErrorMessage(TFTP::ErrorCode::FileNotFound, ""));
    		// TO DO: send error message
    		return;
    	}

        std::cout << " Send block of data\n";
    	if (!sendBlockOfData())
    	{
    		std::cerr << "TFTPSessionRead could not send block of data.\n";
    		return;
    	}

    }    
    else if (msgType == static_cast<std::uint16_t>(TFTP::MessageType::ACK))
    {
    	std::uint16_t blockCounter = 0;
    	std::memcpy(&blockCounter, data.cbegin() + 2, sizeof(blockCounter));    	

    	if (blockCounter != getBlockCounter())
    	{
    		const TFTP::Buffer message(TFTP::makeErrorMessage(TFTP::ErrorCode::UnknownTransferId, ""));
    		// TO DO: send error message
    		return;
    	}

    	if (_readComplete)
    	{
    		_inFile.close();
    		setState(State::Finished);
    		return;
    	}

        std::cout << "Send block of data\n";
    	if (!sendBlockOfData())
    	{
    		std::cerr << "TFTPSessionRead could not send block of data.\n";
    		return;
    	}
    }
    else
    {
    	std::cerr << "TFTPSessionRead received message of unexpected type.\n";
    }
}

bool TFTPSessionRead::sendBlockOfData()
{
	const std::uint16_t DATA_OPCODE = static_cast<std::uint16_t>(TFTP::MessageType::DATA);
	std::memcpy(_sendBuffer.data(), &DATA_OPCODE, sizeof(DATA_OPCODE));
	std::size_t bytesCount = sizeof(DATA_OPCODE);

	incrementBlockCounter();
	const std::uint16_t blockCounter = getBlockCounter();
	std::memcpy(_sendBuffer.data() + bytesCount, &blockCounter, sizeof(blockCounter));
	bytesCount += sizeof(blockCounter);

    std::size_t n = 0;
    if (getTrasferMode() == TFTP::TransferMode::ASCII)
    {
        while (n < 512)
        {
            if (_lineFeed)
            {
                *(_sendBuffer.data() + bytesCount) = '\n';
                bytesCount += 1;
                n += 1;
                _lineFeed = false;
            }

            int x = _inFile.peek();
            if (x == EOF)
            {
                break;
            }

            if (x == '\n')
            {
                *(_sendBuffer.data() + bytesCount) = '\r';
                bytesCount += 1;
                n += 1;
                if (n == 512)
                {
                    _inFile.get();
                    _lineFeed = true;
                    break;
                }
            }

            x = _inFile.get();
            *(_sendBuffer.data() + bytesCount) = x;
            bytesCount += 1;
            n += 1;
        }
    }
    else
    {
        // n = _inFile.readsome(_sendBuffer.data() + bytesCount, 512);
        // bytesCount += n;
        _inFile.read(_sendBuffer.data() + bytesCount, 512);
        n = _inFile.gcount();
        bytesCount += n;        
    }

	_sendBytesCount = bytesCount;

	// the latest block
	if (n < 512)
	{
		_readComplete = true;
	}

    std::cout << "amount of bytes to send " << _sendBytesCount << std::endl;
	if (!send(_sendBuffer, _sendBytesCount))
	{
		std::cerr << "TFTPSessionRead could not send data.\n";
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// TFTPSessionWrite implementation
TFTPSessionWrite::TFTPSessionWrite(udp::socket& socket, const udp::endpoint& ep)
	: TFTPSession(socket, ep)
{

}

void TFTPSessionWrite::onDataReceived(const TFTP::Buffer& data, std::size_t amount)
{
     std::uint16_t msgType = 0;
     std::memcpy(&msgType, data.data(), sizeof(msgType));
     std::size_t bytesCount = sizeof(msgType);

    if (msgType == static_cast<std::uint16_t>(TFTP::MessageType::WRQ))
    {
    	setState(State::Active);

    	const std::string filename(data.cbegin() + bytesCount);
    	if (filename.empty())
    	{
            std::cerr << "Error: No filename provided (command WRQ)\n";
    		return;
    	}

    	bytesCount += filename.length() + 1;
    	const std::string mode(data.cbegin() + bytesCount);
    	if (mode == "octet")
    	{
    		setTransferMode(TFTP::TransferMode::Binary);
    	}
    	else if (mode == "netascii")
    	{
    		setTransferMode(TFTP::TransferMode::ASCII);
    	}
    	else
    	{
            std::cerr << "Error: invalid transfer mode (command WRQ)\n";
    		return ;
    	}

    	if (getTrasferMode() == TFTP::TransferMode::Binary)
    	{
    		_outFile.open(filename, std::ios::binary);
    	}
    	else
    	{
    		_outFile.open(filename);
    	}

    	// TO DO: check the following situations
    	// 1. file already exists (error code )
    	// 2. not permission to write (error code 2)
    	// if (!_outFile)
    	// {
    	// 	const TFTP::Buffer message(TFTP::makeErrorMessage(TFTP::ErrorCode::FileNotFound, ""));
    	// 	// TO DO: send error message
    	// 	return;
    	// }

    	if (!sendAcknowledge())
    	{
    		std::cerr << "TFTPSessionWrite could not send acknowledge.\n";
    		return;
    	}
    }    
    else if (msgType == static_cast<std::uint16_t>(TFTP::MessageType::DATA))
    {
    	std::uint16_t blockCounter = 0;
    	std::memcpy(&blockCounter, data.cbegin() + bytesCount, sizeof(blockCounter));
    	bytesCount += sizeof(blockCounter);

    	incrementBlockCounter();
    	if (blockCounter != getBlockCounter())
    	{
    		const TFTP::Buffer message(TFTP::makeErrorMessage(TFTP::ErrorCode::UnknownTransferId, ""));
    		// TO DO: send error message
    		return;
    	}

		std::size_t n = amount - bytesCount;
    	if (getTrasferMode() == TFTP::TransferMode::ASCII)
    	{
    		const char* p = data.cbegin() + bytesCount;    		
    		for (std::size_t i = 0; i < n; i++, p++)
    		{
    			if (*p == '\r')
    			{
    				continue;
    			}
    			_outFile.put(*p);
    		}
    	}
    	else
    	{
    		_outFile.write(data.cbegin() + bytesCount, n);
    	}

    	// // the latest block
    	// if (n < 512)
    	// {
    	// 	_outFile.close();
    	// }

    	if (!sendAcknowledge())
    	{
    		std::cerr << "TFTPSessionWrite could not send acknowledge.\n";
    		return;
    	}

    	// the latest block
    	if (n < 512)
    	{
    		_outFile.close();
    		setState(State::Finished);
    	}    	
    }
    else
    {
    	std::cerr << "TFTPSessionWrite received message of unexpected type.\n";
    }	
	
}

bool TFTPSessionWrite::sendAcknowledge()
{
	const std::uint16_t blockCounter = getBlockCounter();
	const std::uint16_t ACK_OPCODE = static_cast<std::uint16_t>(TFTP::MessageType::ACK);
	std::memcpy(_sendBuffer.data(), &ACK_OPCODE, sizeof(ACK_OPCODE));
	std::size_t bytesCount = sizeof(ACK_OPCODE);
	std::memcpy(_sendBuffer.data() + bytesCount, &blockCounter, sizeof(blockCounter));
	bytesCount += sizeof(blockCounter);
	_sendBytesCount = bytesCount;

	if (!send(_sendBuffer, _sendBytesCount))
	{
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// TFTPServer implementation
TFTPServer::TFTPServer()
	: _ioContext(1)
    , _signal(_ioContext, SIGINT, SIGTERM)
	// , _socket(_ioContext, {udp::v4(), TFTP::PORT})
    , _socket(_ioContext)
{
//	receive();
}

TFTPServer::~TFTPServer()
{

}

void TFTPServer::start(const std::string& addr, std::uint16_t port, bool reuse/* = false*/)
{
    std::error_code ec;

    _socket.open(udp::v4(), ec);
    if (ec)
    {
        std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
        throw std::runtime_error("Could not start server (socket open failed)");
    }

    _socket.bind({asio::ip::make_address(addr), port}, ec);
    if (ec)
    {
        std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
        throw std::runtime_error("Could not start server (socket bind failed)");
    }

    waitSignal();
    receive();

    std::cout << " starting...\n";
    _ioContext.restart();
    _ioContext.run();
    std::cout << " finished\n";
}

void TFTPServer::stop()
{
    std::error_code ec;
    _socket.cancel(ec);
    if (ec)
    {
        std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
    }

    _socket.close(ec);
    if (ec)
    {
        std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
    }

    try
    {
        _ioContext.stop();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }

}

void TFTPServer::receive()
{
	_socket.async_receive_from(
		asio::buffer(_recvBuffer), _remoteEndpoint,
		[this](std::error_code ec, std::size_t n)
		{
			if (!ec)
			{			
		         std::uint16_t msgType = 0;
		         std::memcpy(&msgType, _recvBuffer.data(), sizeof(msgType));

	        	 Sessions::iterator it = _sessions.find(_remoteEndpoint);
	        	 if (it == _sessions.end())
	        	 {
			        if (msgType == static_cast<std::uint16_t>(TFTP::MessageType::RRQ))
			        {
			        	std::unique_ptr<TFTPSession> session = std::make_unique<TFTPSessionRead>(_socket, _remoteEndpoint);
			        	_sessions[_remoteEndpoint] = std::move(session);
			        	_sessions[_remoteEndpoint]->onDataReceived(_recvBuffer, n);
			        }
			        else if (msgType == static_cast<std::uint16_t>(TFTP::MessageType::WRQ))
			        {
			        	std::unique_ptr<TFTPSession> session = std::make_unique<TFTPSessionWrite>(_socket, _remoteEndpoint);
			        	_sessions[_remoteEndpoint] = std::move(session);
			        	_sessions[_remoteEndpoint]->onDataReceived(_recvBuffer, n);
			        }
			        else
			        {
			        	const TFTP::Buffer message(TFTP::makeErrorMessage(TFTP::ErrorCode::IllegalOperation, ""));
			        	// TO DO: send error messsage
			        }
	        	 }
	        	 else
	        	 {
	        	 	it->second->onDataReceived(_recvBuffer, n);
	        	 	if (it->second->getState() == TFTPSession::State::Finished)
	        	 	{
	        	 		_sessions.erase(_remoteEndpoint);
	        	 		std::cout << " Finished session removed from container \n";
	        	 	}
	        	 }
			}
			else
			{
				std::cerr << "Error when receive: " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}

			receive();
		});
}

void TFTPServer::waitSignal()
{
    _signal.async_wait([this](std::error_code ec, int signo)
        {
            if (!ec)
            {
                std::cout << "Signal #" << signo << std::endl;
                stop();
            }
            else
            {
                std::cerr << "Error when waiting signal: "
                    << ec.message() << '(' << ec.value() << ')' << std::endl;
            }
        });
}


int main(int argc, char* argv[])
{
	try
	{
		TFTPServer tftpServer;
		tftpServer.start("127.0.0.1", TFTP::PORT);
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
		std::exit(-1);
	}

	return 0;
}
