#include <csignal>

#include <iostream>
#include <stdexcept>
#include <vector>

#include "dns_server.h"
#include "dns_query.h"
#include "dns_response.h"
#include "dns_resolver.h"


DNSServer::DNSServer(const std::string& addr, std::uint16_t port, bool reuseAddr /*= false*/)
	: _addr(addr)
	, _port(port)
	, _ioContext(1)
	, _signal(_ioContext, SIGINT, SIGTERM)
	, _socket(_ioContext)
{
	// TO DO: reuseAddr
}

DNSServer::~DNSServer()
{

}


void DNSServer::start()
{
    std::error_code ec;

    _socket.open(udp::v4(), ec);
    if (ec)
    {
        std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
        throw std::runtime_error("Could not start server (socket open failed)");
    }

    _socket.bind({asio::ip::make_address(_addr), _port}, ec);
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

void DNSServer::stop()
{
	try
	{
		_ioContext.stop();
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Exception: " << ex.what() << std::endl;
	}
}

void DNSServer::receive()
{
	/* Article about reading unspecified amount of bytes,
	 and how to allocate read buffer "on the fly" 
	 http://breese.github.io/2016/12/03/receiving-datagrams.html
	 */

	_socket.async_receive_from(asio::null_buffers(), _clientEndpoint,
		[this](std::error_code ec, std::size_t sz)
		{
			if (!ec)
			{
				std::cout << "Received " << sz << " bytes.\n";
				udp::socket::bytes_readable cmdReadable(true);

				_socket.io_control(cmdReadable, ec);
				if (!ec)
				{
					std::size_t readableAmount = cmdReadable.get();
					std::cout << "Available to read: " << readableAmount << " bytes.\n";

					std::vector<std::uint8_t> buffer(readableAmount);
					sz = _socket.receive_from(asio::buffer(buffer.data(), buffer.size()), _clientEndpoint, 0, ec);
					if (!ec)
					{
						std::cout << "Received " << sz << " bytes.\n";
						// TO DO: process received data and send back response
						// TO DO: implement asynchronous processing
						try
						{
							DNSQuery dnsQuery;
							std::size_t n = dnsQuery.decode(buffer);
							std::cout << "Decoded " << n << " bytes.\n";

							if (dnsQuery.getFlagQR())
							{
								throw std::logic_error("Received message is not query.");
							}

							// DNSResponse dnsResponse(dnsQuery.getId());
							DNSResponse dnsResponse;

							assert(_resolver != NULL);
							_resolver->process(dnsQuery, dnsResponse);

							// std::cout << "Response is:\n";
							// dnsResponse.dump(std::cout);
							// std::cout << std::endl;
							sendResponse(dnsResponse.encode());
						}
						catch (const std::exception& ex)
						{
							std::cerr << "Exception when processing DNS message: "
								<< ex.what() << std::endl;
						}
					}
					else
					{
						std::cerr << "Read failed. ";
						std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
					}
				}
				else
				{
					std::cerr << "IOControl failed. ";
					std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
				}
			}
			else
			{
				std::cerr << "AsyncReceive failed. ";
				std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}

			//receive();
		});
}

void DNSServer::sendResponse(std::vector<std::uint8_t>&& buffer)
{
	_socket.async_send_to(asio::buffer(buffer.data(), buffer.size()), _clientEndpoint,
		[this](std::error_code ec, std::size_t sz)
		{
			if (ec)
			{
				std::cerr << "AsyncSend failed. ";
				std::cerr << "Error: " << ec.message() << '(' << ec.value() << ')' << std::endl;
			}
			else
			{
				std::cout << "Amount of bytes sent: " << sz << std::endl;
				receive();
			}
		});
}

void DNSServer::waitSignal()
{
	_signal.async_wait([this](std::error_code ec, int signo)
		{
			if (!ec)
			{
				std::cout << " signal #" << signal << std::endl;
				stop();
			}
			else
			{
				std::cerr << "Error when waiting signal: "
					<< ec.message() << '(' << ec.value() << ')' << std::endl;
			}
		});
}
