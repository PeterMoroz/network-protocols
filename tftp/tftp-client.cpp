#include <cassert>

#include <algorithm>

#include <array>
#include <exception>
#include <experimental/filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>

#include <asio/io_context.hpp>
#include <asio/ip/udp.hpp>


using asio::ip::udp;


class TFTPClient final
{
public:
    enum class TransferMode
    {
        Binary,
        ASCII
    };


private:
    static const std::uint16_t TFTP_PORT = 10069;
    static const std::size_t BUFFER_SIZE = 516;

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

public:
    TFTPClient(const TFTPClient&) = delete;
    TFTPClient& operator=(const TFTPClient&) = delete;


    TFTPClient();
    ~TFTPClient();

    void setTransferMode(TransferMode transferMode)
    {
        _transferMode = transferMode;
    }

    bool getFile(const std::string& remoteHost, 
                const std::string& srcFilename,
                const std::string& dstFilename);

    bool putFile(const std::string& remoteHost, 
                const std::string& srcFilename,
                const std::string& dstFilename);

private:
    void worker();

    bool send();
    bool recv();

    void writeErrorMessage(std::ostream& os) const;

private:
    TransferMode _transferMode = TransferMode::ASCII;
    asio::io_context _ioContext;
    udp::socket _socket;    
    std::thread _worker;
    std::string _remoteHostAddr;

    std::array<char, BUFFER_SIZE> _sendBuffer;
    std::array<char, BUFFER_SIZE> _recvBuffer;
    std::size_t _sendBytesCount = 0;
    std::size_t _recvBytesCount = 0;
};



TFTPClient::TFTPClient()
    : _ioContext()
    , _socket(_ioContext, udp::v4())
    , _worker()
{
    try
    {
        _worker = std::thread(&TFTPClient::worker, this);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception in TFTPClient c-tor: " << ex.what() << std::endl;
        throw std::runtime_error("TFTPClient c-tor failed.");
    }
}

TFTPClient::~TFTPClient()
{
    try
    {
        _ioContext.stop();
        _worker.join();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception in TFTPClient d-tor: " << ex.what() << std::endl;
    }
}

bool TFTPClient::getFile(const std::string& remoteHost, 
                        const std::string& srcFilename,
                        const std::string& dstFilename)
{
    std::ofstream outFile;

    std::function<void ()> removeOutFile = 
        [&outFile, &dstFilename]()
        {
            outFile.close();
            namespace fs = std::experimental::filesystem;
            const fs::path outFilePath(dstFilename);
            std::error_code ec;
            if (!fs::remove(outFilePath, ec) || ec)
            {
                std::cerr << "Could not remove file " << outFilePath << std::endl;
                if (ec)
                {
                    std::cerr << "Filesystem error " << ec.message() << '(' << ec.value() << ')' << std::endl;
                }
            }
        };

    if (_transferMode == TransferMode::Binary)
    {
        outFile.open(dstFilename, std::ios_base::binary);
    }
    else
    {
        outFile.open(dstFilename);
    }

    if (!outFile)
    {
        std::cerr << "Could not open output file: '" << dstFilename << "'.\n";
        return false;
    }

    std::size_t bytesCount = 0;

    static const std::uint16_t RRQ_OPCODE = static_cast<std::uint16_t>(MessageType::RRQ);
    std::memcpy(_sendBuffer.data(), &RRQ_OPCODE, sizeof(RRQ_OPCODE));
    bytesCount += sizeof(RRQ_OPCODE);

    static const std::size_t MAX_FILENAME_LENGTH = 78;
    const std::size_t fnameLength = std::min(srcFilename.length(), static_cast<std::size_t>(MAX_FILENAME_LENGTH));
    std::copy_n(srcFilename.cbegin(), fnameLength, _sendBuffer.begin() + bytesCount);
    bytesCount += fnameLength;
    *(_sendBuffer.begin() + bytesCount) = '\0';
    bytesCount += 1;

    if (_transferMode == TransferMode::Binary)
    {
        static const char MODE[] = "octet";
        std::copy_n(MODE, sizeof(MODE), _sendBuffer.begin() + bytesCount);
        bytesCount += sizeof(MODE);
    }
    else
    {
        static const char MODE[] = "netascii";
        std::copy_n(MODE, sizeof(MODE), _sendBuffer.begin() + bytesCount);
        bytesCount += sizeof(MODE);        
    }


    assert((_sendBytesCount = bytesCount) <= BUFFER_SIZE);
    _remoteHostAddr = remoteHost;
    if (!send())
    {
        std::cerr << "Send command RRQ failed!\n";
        return false;
    }


    std::uint16_t expectedBlockNumber = 0;
    bool complete = false;
    bytesCount = 0;
    while (!complete)
    {
        if (!recv())
        {
            std::cerr << "Could not receive data from server.\n";
            return false;
        }

        std::uint16_t msgType = 0;
        std::memcpy(&msgType, _recvBuffer.data(), sizeof(msgType));
        bytesCount = sizeof(msgType);

        if (msgType == static_cast<std::uint16_t>(MessageType::ERROR))
        {
            writeErrorMessage(std::cerr);
            removeOutFile();
            return false;
        }

        expectedBlockNumber += 1;
        if (msgType == static_cast<std::uint16_t>(MessageType::DATA))
        {
            std::uint16_t blockNum = 0;
            std::memcpy(&blockNum, _recvBuffer.data() + bytesCount, sizeof(blockNum));
            bytesCount += sizeof(blockNum);            
            if (blockNum != expectedBlockNumber)
            {
                std::cerr << "Blocks sequence is broken. (expected "
                         << expectedBlockNumber << " actual " << blockNum << ")\n";
                removeOutFile();
                return false;
            }

            const std::size_t blockSize = _recvBytesCount - bytesCount;
            assert(blockSize <= 512);

            if (_transferMode == TransferMode::Binary)
            {
                outFile.write(_recvBuffer.data() + bytesCount, blockSize);
            }
            else
            {
                const char* p = _recvBuffer.data() + bytesCount;
                for (std::size_t i = 0; i < blockSize; i++, p++)
                {
                    if (*p == '\r')
                    {
                        continue;
                    }
                    outFile.put(*p);
                }
            }

            if (blockSize < 512)
            {
                complete = true;
            }

            bytesCount = 0;
            static const std::uint16_t ACK_OPCODE = static_cast<std::uint16_t>(MessageType::ACK);
            std::memcpy(_sendBuffer.data(), &ACK_OPCODE, sizeof(ACK_OPCODE));
            bytesCount += sizeof(ACK_OPCODE);
            std::memcpy(_sendBuffer.data() + bytesCount, &blockNum, sizeof(blockNum));
            bytesCount += sizeof(blockNum);
            assert((_sendBytesCount = bytesCount) <= BUFFER_SIZE);

            if (!send())
            {
                std::cerr << "Send ACKnowlede to block " << blockNum << " failed.\n";
                removeOutFile();
                return false;
            }
        }
        else
        {
            std::cerr << "Type of the received message is unknown.\n";
        }
    }

    return true;
}

bool TFTPClient::putFile(const std::string& remoteHost, 
                        const std::string& srcFilename,
                        const std::string& dstFilename)
{
    std::ifstream inFile;

    if (_transferMode == TransferMode::Binary)
    {
        inFile.open(srcFilename, std::ios_base::binary);
    }
    else
    {
        inFile.open(srcFilename);
    }

    if (!inFile)
    {
        std::cerr << "Could not open file: '" << srcFilename << "'.\n";
        return false;
    }

    std::size_t bytesCount = 0;

    const std::uint16_t WRQ_OPCODE = static_cast<std::uint16_t>(MessageType::WRQ);
    std::memcpy(_sendBuffer.data(), &WRQ_OPCODE, sizeof(WRQ_OPCODE));
    bytesCount += sizeof(WRQ_OPCODE);

    static const std::size_t MAX_FILENAME_LENGTH = 78;
    const std::size_t fnameLength = std::min(dstFilename.length(), static_cast<std::size_t>(MAX_FILENAME_LENGTH));
    std::copy_n(dstFilename.cbegin(), fnameLength, _sendBuffer.begin() + bytesCount);
    bytesCount += fnameLength;
    *(_sendBuffer.begin() + bytesCount) = '\0';
    bytesCount += 1;

    if (_transferMode == TransferMode::Binary)
    {
        static const char MODE[] = "octet";
        std::copy_n(MODE, sizeof(MODE), _sendBuffer.begin() + bytesCount);
        bytesCount += sizeof(MODE);
    }
    else
    {
        static const char MODE[] = "netascii";
        std::copy_n(MODE, sizeof(MODE), _sendBuffer.begin() + bytesCount);
        bytesCount += sizeof(MODE);        
    }


    assert((_sendBytesCount = bytesCount) <= BUFFER_SIZE);
    _remoteHostAddr = remoteHost;
    if (!send())
    {
        std::cerr << "Send command WRQ failed!\n";
        return false;
    }



    std::uint16_t currBlockNumber = 0;
    bool complete = false;
    bool lineFeed = false;

    while (true)
    {
        if (!recv())
        {
            std::cerr << "Could not receive data from server.\n";
            return false;
        }

        std::uint16_t msgType = 0;
        std::memcpy(&msgType, _recvBuffer.data(), sizeof(msgType));
        bytesCount = sizeof(msgType);

        if (msgType == static_cast<std::uint16_t>(MessageType::ERROR))
        {
            writeErrorMessage(std::cerr);
            return false;
        }        

        if (msgType == static_cast<std::uint16_t>(MessageType::ACK))
        {
            std::uint16_t blockNum = 0;
            std::memcpy(&blockNum, _recvBuffer.data() + bytesCount, sizeof(blockNum));
            bytesCount += sizeof(blockNum);
            if (blockNum != currBlockNumber)
            {
                std::cerr << "Blocks sequence is broken. (expected "
                         << currBlockNumber << " actual " << blockNum << ")\n";
                return false;
            }

            if (complete)
            {
                break;
            }

            currBlockNumber += 1;

            bytesCount = 0;
            static const std::uint16_t DATA_OPCODE = static_cast<std::uint16_t>(MessageType::DATA);
            std::memcpy(_sendBuffer.data(), &DATA_OPCODE, sizeof(DATA_OPCODE));
            bytesCount += sizeof(DATA_OPCODE);
            std::memcpy(_sendBuffer.data() + bytesCount, &currBlockNumber, sizeof(currBlockNumber));
            bytesCount += sizeof(currBlockNumber);


            std::size_t n = 0;
            if (_transferMode == TransferMode::ASCII)
            {
                while (n < 512)
                {
                    if (lineFeed)
                    {
                        *(_sendBuffer.data() + bytesCount) = '\n';
                        bytesCount += 1;
                        n += 1;
                        lineFeed = false;
                    }

                    int x = inFile.peek();
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
                            inFile.get();
                            lineFeed = true;
                            break;
                        }
                    }

                    x = inFile.get();
                    *(_sendBuffer.data() + bytesCount) = x;
                    bytesCount += 1;
                    n += 1;
                }
            }
            else
            {
                // n = inFile.readsome(_sendBuffer.data() + bytesCount, 512);
                // std::cout << " n = " << n << ", gcount() = " << inFile.gcount() << std::endl;
                // bytesCount += n;

                inFile.read(_sendBuffer.data() + bytesCount, 512);
                n = inFile.gcount();
                // std::cout << " n = " << n << ", gcount() = " << inFile.gcount() << std::endl;
                bytesCount += n;
            }

            if (n < 512)
            {
                complete = true;
            }

            assert((_sendBytesCount = bytesCount) <= BUFFER_SIZE);

            if (!send())
            {
                std::cerr << "Could not send block of data to server.\n";
                return false;
            }
        }
        else
        {
            std::cerr << "Type of the received message is unknown.\n";            
        }
    }

    return true;
}

void TFTPClient::worker()
{
    asio::error_code ec;
    _ioContext.run(ec);

    if (ec)
    {
        std::cerr << "Error when run IO-context: "
            << ec.message() << '(' << ec.value() << ')' << std::endl;
    }
}

bool TFTPClient::send()
{
    asio::ip::udp::endpoint ep(asio::ip::address::from_string(_remoteHostAddr), TFTP_PORT);
    asio::error_code ec;
    // std::size_t n = _socket.send_to(asio::buffer(buffer), ep, 0, ec);
    std::size_t n = _socket.send_to(asio::const_buffer(_sendBuffer.data(), _sendBytesCount), ep, 0, ec);

    if (ec)
    {
        std::cerr << "Error when sending data (" << _remoteHostAddr << ':' << TFTP_PORT << ") "
                 << ec.message() << '(' << ec.value() << ')' << std::endl;
        return false;
    }

    std::cout << "Sent " << n << " bytes.\n";
    return true;
}

bool TFTPClient::recv()
{
    asio::ip::udp::endpoint ep(asio::ip::address::from_string(_remoteHostAddr), TFTP_PORT);
    asio::error_code ec;
    // TO DO: how to avoid the buffer overflow ?
    _recvBytesCount = _socket.receive_from(asio::buffer(_recvBuffer), ep, 0, ec);

    if (ec)
    {
        std::cerr << "Error when receiving data (" << _remoteHostAddr << ':' << TFTP_PORT << ") "
                 << ec.message() << '(' << ec.value() << ')' << std::endl;
        _recvBytesCount = 0;
        return false;
    }

    std::cout << "Received " << _recvBytesCount << " bytes.\n";
    return true;
}

void TFTPClient::writeErrorMessage(std::ostream& os) const
{
    std::size_t bytesCount = 0;

    std::uint16_t msgType = 0;
    std::memcpy(&msgType, _recvBuffer.data(), sizeof(msgType));
    bytesCount = sizeof(msgType);

    assert(msgType == static_cast<std::uint16_t>(MessageType::ERROR));

    std::uint16_t errCode = 0;
    std::memcpy(&errCode, _recvBuffer.data() + bytesCount, sizeof(errCode));
    bytesCount += sizeof(errCode);

    os << "Error message was sent by server:";
    switch (errCode)
    {
    case static_cast<std::uint16_t>(ErrorCode::NoError):
        os << " No Error";
    break;
    case static_cast<std::uint16_t>(ErrorCode::FileNotFound):
        os << " File not found";
    break;
    case static_cast<std::uint16_t>(ErrorCode::AccessViolation):
        os << " Access violation";
    break;
    case static_cast<std::uint16_t>(ErrorCode::DiskFull):
        os << " Disk full or allocation exceeded";
    break;
    case static_cast<std::uint16_t>(ErrorCode::IllegalOperation):
        os << " Illegal TFTP operation";
    break;
    case static_cast<std::uint16_t>(ErrorCode::UnknownTransferId):
        os << " Unknown transfer ID";
    break;
    case static_cast<std::uint16_t>(ErrorCode::FileAlreadyExists):
        os << " File already exists";
    break;
    case static_cast<std::uint16_t>(ErrorCode::NoSuchUser):
        os << " No such user";
    break;
    case static_cast<std::uint16_t>(ErrorCode::TerminateTransfer):
        os << " Terminate transfer due to option negotiation";
    break;
    default:
        assert(false);
    }

    os << " (" << errCode << ") ";

    std::string msgText;
    msgText.reserve(BUFFER_SIZE - bytesCount);
    // don't rely that error message is null-terminated
    while (bytesCount < BUFFER_SIZE)
    {
        if (_recvBuffer[bytesCount] == '\0')
        {
            break;
        }
        msgText.push_back(_recvBuffer[bytesCount]);
        bytesCount += 1;
    }

    if (!msgText.empty())
    {
        os << " details: " << msgText;
    }
    os << std::endl;
}



int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        std::cerr << "Usage: \n"
            << argv[0] << " -i <remote host> <command: put|get> <src-file> <dst-file>\n";
        std::exit(-1);
    }

    std::function<std::string (const char*)> toUppercase = [](const char* s)
    {
        std::string str;
        while (*s)
        {
            str.push_back(static_cast<char>(std::toupper(*s++)));
        }
        return str;
    };

    bool binaryMode = (argc == 6 && !std::strcmp(argv[1], "-i"));
    const std::string remoteHost(argc == 5 ? argv[1] : argv[2]);
    const std::string command(argc == 5 ? toUppercase(argv[2]) : toUppercase(argv[3]));
    const std::string srcFilename(argc == 5 ? argv[3] : argv[4]);
    const std::string dstFilename(argc == 5 ? argv[4] : argv[5]);

    try
    {
        TFTPClient tftpClient;
        if (binaryMode)
        {
            tftpClient.setTransferMode(TFTPClient::TransferMode::Binary);
        }

        if (command == "GET")
        {
            if (!tftpClient.getFile(remoteHost, srcFilename, dstFilename))
            {
                std::cerr << "GET file failed!\n";
                std::exit(-1);
            }
            std::cerr << "GET file complete successfully.\n";
        }
        else if (command == "PUT")
        {
            if (!tftpClient.putFile(remoteHost, srcFilename, dstFilename))
            {
                std::cerr << "PUT file failed!\n";
                std::exit(-1);
            }
            std::cerr << "PUT file complete successfully.\n";
        }
        else
        {
            std::cerr << "Unknown command " << command << std::endl;
        }

    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception: " << ex.what() << std::endl;
        std::exit(-1);
    }

    return 0;
}
