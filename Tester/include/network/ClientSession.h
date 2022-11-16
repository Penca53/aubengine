#pragma once

#include <asio/asio.hpp>
#include "QueueThreadSafe.h"
#include "Message.h"

template<typename T>
class ClientSession : public std::enable_shared_from_this<ClientSession<T>>
{
public:
	ClientSession(asio::ip::tcp::socket tcpSocket, asio::io_context& asioContext, QueueThreadSafe<Message<T>>& qIn)
		: _asioContext(asioContext), _tcpSocket(std::move(tcpSocket)), _messagesIn(qIn)
	{

	}

	virtual ~ClientSession()
	{
		std::cout << "Destroy Connection\n";
	}
public:
	void ConnectToClient(uint32_t id)
	{
		if (_tcpSocket.is_open())
		{
			_id = id;
			DoReadTCPHeader();
			_isConnected = true;
		}
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
					DoWriteTCPHeader();
				}
			});
	}

	void Disconnect()
	{
		if (IsConnected())
		{
			asio::post(_asioContext, [this]()
				{
					_tcpSocket.close();
				});
		}

		_isConnected = false;
	}

	bool IsConnected() const
	{
		return _isConnected;
	}

	uint32_t GetID()
	{
		return _id;
	}

	bool TryGetUDPEndpoint(asio::ip::udp::endpoint& endpoint)
	{
		if (UDPPort == 0)
		{
			return false;
		}

		// Resolve hostname/ip-address into tangiable physical address
		auto host = _tcpSocket.remote_endpoint().address().to_string();
		auto port = std::to_string(UDPPort);

		asio::ip::udp::resolver resolver(_asioContext);
		asio::ip::udp::resolver::results_type endpoints = resolver.resolve(host, port);
		endpoint = *endpoints.begin();

		return true;
	}
private:
	void DoReadTCPHeader()
	{
		asio::async_read(_tcpSocket, asio::buffer(&_msgTemporaryIn.Header, sizeof(MessageHeader<T>)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (_msgTemporaryIn.Header.Size > 0)
					{
						_msgTemporaryIn.Body.resize(_msgTemporaryIn.Header.Size);
						DoReadTCPBody();
					}
					// Header-only message
					else
					{
						AddToIncomingMessageQueue();
					}
				}
				else
				{
					Disconnect();
				}
			});
	}

	void DoReadTCPBody()
	{
		asio::async_read(_tcpSocket, asio::buffer(_msgTemporaryIn.Body.data(), _msgTemporaryIn.Body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					AddToIncomingMessageQueue();
					DoReadTCPHeader();
				}
				else
				{
					Disconnect();
				}
			});
	}

	void DoWriteTCPHeader()
	{
		asio::async_write(_tcpSocket, asio::buffer(&_messagesOut.Front().Header, sizeof(MessageHeader<T>)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (_messagesOut.Front().Body.size() > 0)
					{
						DoWriteTCPBody();
					}
					// Header-only message
					else
					{
						_messagesOut.PopFront();
						if (!_messagesOut.Empty())
						{
							DoReadTCPHeader();
						}
					}
				}
				else
				{
					Disconnect();
				}
			});
	}
	void DoWriteTCPBody()
	{
		asio::async_write(_tcpSocket, asio::buffer(_messagesOut.Front().Body.data(), _messagesOut.Front().Body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					_messagesOut.PopFront();
					if (!_messagesOut.Empty())
					{
						DoReadTCPHeader();
					}
				}
				else
				{
					Disconnect();
				}
			});
	}

	void AddToIncomingMessageQueue()
	{
		_msgTemporaryIn.TCPRemote = this->shared_from_this();
		_messagesIn.PushBack(_msgTemporaryIn);
	}
public:
	uint16_t UDPPort = 0;
private:
	asio::ip::tcp::socket _tcpSocket;

	asio::io_context& _asioContext;

	QueueThreadSafe<Message<T>>& _messagesIn;
	QueueThreadSafe<Message<T>> _messagesOut;

	Message<T> _msgTemporaryIn;

	uint32_t _id = 0;
	bool _isConnected = false;
};