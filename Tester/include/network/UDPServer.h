#pragma once

#include "QueueThreadSafe.h"
#include "Delegate.h"
#include "Message.h"

template<typename T>
class UDPServer
{
public:
	UDPServer(uint16_t port)
		: _udpSocket(_asioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
	{

	}

	virtual ~UDPServer()
	{
		Stop();
	}

	bool StartServer()
	{
		try
		{
			DoReceive();
			_ThreadContext = std::thread([this]() { _asioContext.run(); });
		}
		catch (std::exception& e)
		{
			std::cerr << "[SERVER] Exception: " << e.what() << "\n";
			return false;
		}

		std::cout << "[UDP SERVER] Started!\n";
		return true;
	}

	void Stop()
	{
		_asioContext.stop();

		if (_ThreadContext.joinable())
		{
			_ThreadContext.join();
		}

		std::cout << "[UDP SERVER] Stopped!\n";
	}

	void Send(const Message<T>& msg)
	{
		asio::post(_asioContext,
			[this, msg]()
			{
				bool isWritingMessage = !_messagesOut.Empty();
				_messagesOut.PushBack(msg);
				if (!isWritingMessage)
				{
					DoSend();
				}
			});
	}


	void DoReceive()
	{
		_udpSocket.async_receive_from(
			asio::buffer(_inUdpBuffer, UDP_DATAGRAM_SIZE),
			_senderEndpoint,
			[this](std::error_code ec, std::size_t bytesReceived)
			{
				if (!ec)
				{
					_msgTemporaryIn.FromBuffer(_inUdpBuffer, bytesReceived);
					AddToIncomingMessageQueue();
				}

				DoReceive();
			});
	}
	void DoSend()
	{
		auto message = _messagesOut.Front();
		size_t length = message.ToBuffer(_outUdpBuffer);
		_udpSocket.async_send_to(
			asio::buffer(_outUdpBuffer, length),
			message.UDPRemote,
			[this](std::error_code ec, std::size_t bytesSent)
			{
				if (!ec)
				{
					_messagesOut.PopFront();
					if (!_messagesOut.Empty())
					{
						DoSend();
					}
				}
				else
				{
					// Error
				}
			});
	}


	bool CanConnect(std::shared_ptr<ClientSession<T>> connection)
	{
		return true;
	}

	void AddToIncomingMessageQueue()
	{
		_msgTemporaryIn.UDPRemote = _senderEndpoint;
		_messagesIn.PushBack(_msgTemporaryIn);
	}

	// Force server to respond to incoming messages
	void UpdateNetwork(size_t maxMessages = -1, bool wait = true)
	{
		if (wait)
		{
			_messagesIn.Wait();
		}

		// Process as many messages as you can up to the value
		// specified
		size_t messageCount = 0;
		while (messageCount < maxMessages && !_messagesIn.Empty())
		{
			// Grab the front message
			auto msg = _messagesIn.PopFront();

			// Pass to message handler
			MessageReceived(msg);

			++messageCount;
		}
	}

public:
	Delegate<Message<T>> MessageReceived;
private:
	static const int UDP_DATAGRAM_SIZE = 64 * 1024;
	uint8_t _inUdpBuffer[UDP_DATAGRAM_SIZE]{};
	uint8_t _outUdpBuffer[UDP_DATAGRAM_SIZE]{};

	// Thread Safe Queue for incoming message packets
	QueueThreadSafe<Message<T>> _messagesIn;
	QueueThreadSafe<Message<T>> _messagesOut;

	// Order of declaration is important - it is also the order of initialisation
	asio::io_context _asioContext;
	std::thread _ThreadContext;

	// These things need an asio context
	asio::ip::udp::socket _udpSocket; // Handles new incoming connection attempts...
	asio::ip::udp::endpoint _senderEndpoint;

	Message<T> _msgTemporaryIn;

	// Clients will be identified in the "wider system" via an ID
	uint32_t _IDCounter = 1;
};