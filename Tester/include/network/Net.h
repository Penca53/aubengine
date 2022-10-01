#pragma once 

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <set>

template<typename ...Params>
class Delegate
{
public:
	Delegate& operator+=(std::function<void(Params... params)> func)
	{
		_functions.push_back(func);
		return *this;
	}

	template<typename ...Args>
	void operator()(Args... args)
	{
		for (auto& f : _functions)
		{
			f(args...);
		}
	}
private:
	std::vector<std::function<void(Params... params)>> _functions;
};

template<typename T>
class ClientSession;

// Message

// Message _header is sent at start of all messages. The template allows us
// to use "enum class" to ensure that the messages are valid at compile time
template <typename T>
class MessageHeader
{
public:
	T ID{};
	uint32_t Size = 0;
};

// Message _body contains a header and a std::vector, containing raw bytes
// of infomation. This way the message can be variable length, but the size
// in the header must be updated.
template <typename T>
class Message
{
public:
	// returns size of entire message packet in bytes
	size_t Size() const
	{
		return Body.size() - _begin;
	}

	// Override for std::cout compatibility - produces friendly description of message
	friend std::ostream& operator << (std::ostream& os, const Message<T>& msg)
	{
		os << "ID:" << int(msg.Header.ID) << " Size:" << msg.Header.Size;
		return os;
	}

	template<typename T>
	void Write(const T& data)
	{
		static_assert(std::is_standard_layout<T>::value, "Data is too complex to be pushed into vector");

		size_t old = Body.size();
		size_t s = sizeof(data);
		Body.resize(Body.size() + sizeof(data));
		std::memcpy(Body.data() + old, &data, sizeof(data));
		Header.Size = Size();
	}

	template<typename T>
	void WriteVector(const std::vector<T>& data)
	{
		size_t size = data.size();
		Write(size);
		for (size_t i = 0; i < size; ++i)
		{
			Write(data[i]);
		}
	}

	template<typename T>
	T Read()
	{
		static_assert(std::is_standard_layout<T>::value, "Data is too complex to be pushed into vector");
		T data{};

		std::memcpy(&data, Body.data() + _begin, sizeof(T));
		_begin += sizeof(T);
		Header.Size = Size();

		return data;
	}

	template<typename T>
	std::vector<T> ReadVector()
	{
		size_t size = Read<size_t>();
		std::vector<T> data(size);
		for (size_t i = 0; i < size; ++i)
		{
			data[i] = Read<T>();
		}

		return data;
	}
public:
	MessageHeader<T> Header{};
	std::vector<uint8_t> Body;

	std::shared_ptr<ClientSession<T>> Remote = nullptr;
private:
	size_t _begin = 0;
};

// Queue
template<typename T>
class QueueThreadSafe
{
public:
	QueueThreadSafe() = default;
	QueueThreadSafe(const QueueThreadSafe<T>&) = delete;
	~QueueThreadSafe() { Clear(); }

public:
	// Returns and maintains item at front of Queue
	const T& Front()
	{
		std::scoped_lock lock(_muxQueue);
		return _deqQueue.front();
	}

	// Returns and maintains item at back of Queue
	const T& Back()
	{
		std::scoped_lock lock(_muxQueue);
		return _deqQueue.back();
	}

	// Removes and returns item from front of Queue
	T PopFront()
	{
		std::scoped_lock lock(_muxQueue);
		auto t = std::move(_deqQueue.front());
		_deqQueue.pop_front();
		return t;
	}

	// Removes and returns item from back of Queue
	T PopBack()
	{
		std::scoped_lock lock(_muxQueue);
		auto t = std::move(_deqQueue.back());
		_deqQueue.pop_back();
		return t;
	}

	// Adds an item to back of Queue
	void PushBack(const T& item)
	{
		std::scoped_lock lock(_muxQueue);
		_deqQueue.emplace_back(std::move(item));
		_cvBlocking.notify_one();
	}

	// Adds an item to front of Queue
	void PushFront(const T& item)
	{
		std::scoped_lock lock(_muxQueue);
		_deqQueue.emplace_front(std::move(item));
		_cvBlocking.notify_one();
	}

	// Returns true if Queue has no items
	bool Empty()
	{
		std::scoped_lock lock(_muxQueue);
		return _deqQueue.empty();
	}

	// Returns number of items in Queue
	size_t Size()
	{
		std::scoped_lock lock(_muxQueue);
		return _deqQueue.size();
	}

	// Clears Queue
	void Clear()
	{
		std::scoped_lock lock(_muxQueue);
		_deqQueue.clear();
	}

	void Wait()
	{
		std::unique_lock<std::mutex> ul(_muxQueue);
		while (_deqQueue.empty())
		{
			_cvBlocking.wait(ul);
		}
	}

private:
	std::mutex _muxQueue;
	std::deque<T> _deqQueue;
	std::condition_variable _cvBlocking;
};


template<typename T>
class ClientSession : public std::enable_shared_from_this<ClientSession<T>>
{
public:
	ClientSession(asio::ip::tcp::socket socket, asio::io_context& asioContext, QueueThreadSafe<Message<T>>& qIn)
		: _socket(std::move(socket)), _asioContext(asioContext), _qMessagesIn(qIn)
	{

	}

	virtual ~ClientSession()
	{
		std::cout << "Destroy Connection\n";
	}
public:
	void ConnectToClient(uint32_t id)
	{
		if (_socket.is_open())
		{
			_id = id;
			_isConnected = true;
			DoReadHeader();
		}
	}

	void Send(const Message<T>& msg)
	{
		asio::post(_asioContext,
			[this, msg]()
			{
				bool bWritingMessage = !_qMessagesOut.Empty();
				_qMessagesOut.PushBack(msg);
				if (!bWritingMessage)
				{
					DoWriteHeader();
				}
			});
	}

	void Disconnect()
	{
		if (IsConnected())
		{
			asio::post(_asioContext, [this]() { _socket.close(); });
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
private:
	void DoReadHeader()
	{
		asio::async_read(_socket, asio::buffer(&_msgTemporaryIn.Header, sizeof(MessageHeader<T>)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (_msgTemporaryIn.Header.Size > 0)
					{
						_msgTemporaryIn.Body.resize(_msgTemporaryIn.Header.Size);
						DoReadBody();
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

	void DoReadBody()
	{
		asio::async_read(_socket, asio::buffer(_msgTemporaryIn.Body.data(), _msgTemporaryIn.Body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					AddToIncomingMessageQueue();
				}
				else
				{
					Disconnect();
				}
			});
	}

	void DoWriteHeader()
	{
		asio::async_write(_socket, asio::buffer(&_qMessagesOut.Front().Header, sizeof(MessageHeader<T>)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (_qMessagesOut.Front().Body.size() > 0)
					{
						DoWriteBody();
					}
					// Header-only message
					else
					{
						_qMessagesOut.PopFront();
						if (!_qMessagesOut.Empty())
						{
							DoWriteHeader();
						}
					}
				}
				else
				{
					Disconnect();
				}
			});
	}

	void DoWriteBody()
	{
		asio::async_write(_socket, asio::buffer(_qMessagesOut.Front().Body.data(), _qMessagesOut.Front().Body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					_qMessagesOut.PopFront();
					if (!_qMessagesOut.Empty())
					{
						DoWriteHeader();
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
		_msgTemporaryIn.Remote = this->shared_from_this();
		_qMessagesIn.PushBack(_msgTemporaryIn);

		DoReadHeader();
	}
private:
	asio::ip::tcp::socket _socket;
	asio::io_context& _asioContext;

	QueueThreadSafe<Message<T>>& _qMessagesIn;
	QueueThreadSafe<Message<T>> _qMessagesOut;

	Message<T> _msgTemporaryIn;

	uint32_t _id = 0;
	bool _isConnected = false;
};

// Client
template <typename T>
class TCPClient
{
public:
	TCPClient() : _socket(_asioContext)
	{

	}

	virtual ~TCPClient()
	{
		// If the client is destroyed, always try and disconnect from server
		std::cout << "Destroy Client\n";
		Disconnect();
	}

public:
	// Connect to server with hostname/ip-address and port
	bool Connect(const std::string& host, const uint16_t port)
	{
		try
		{
			// Resolve hostname/ip-address into tangiable physical address
			asio::ip::tcp::resolver resolver(_asioContext);
			asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

			DoConnect(endpoints);

			// Start Context Thread
			_threadContext = std::thread([this]() { _asioContext.run(); });
		}
		catch (std::exception& e)
		{
			std::cerr << "Client Exception: " << e.what() << "\n";
			return false;
		}
		return true;
	}

	// Disconnect from server
	void Disconnect()
	{
		if (IsConnected())
		{
			_socket.close();
		}

		_asioContext.stop();
		if (_threadContext.joinable())
		{
			_threadContext.join();
		}

		Disconnected();
		_isConnected = false;
	}

	// Check if client is actually connected to a server
	bool IsConnected()
	{
		return _isConnected;
	}

	void UpdateNetwork(size_t maxMessages = -1, bool wait = false)
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

			messageCount++;
		}
	}

	void Send(const Message<T>& msg)
	{
		asio::post(_asioContext,
			[this, msg]()
			{
				bool bWritingMessage = !_messagesOut.Empty();
				_messagesOut.PushBack(msg);
				if (!bWritingMessage)
				{
					DoWriteHeader();
				}
			});
	}

	// Retrieve queue of messages from server
	QueueThreadSafe<Message<T>>& Incoming()
	{
		return _messagesIn;
	}

private:
	void DoConnect(const asio::ip::tcp::resolver::results_type& endpoints)
	{
		asio::async_connect(_socket, endpoints,
			[this](asio::error_code ec, asio::ip::tcp::endpoint)
			{
				if (!ec)
				{
					_isConnected = true;
					DoReadHeader();
				}
			});
	}

	void DoReadHeader()
	{
		asio::async_read(_socket, asio::buffer(&_msgTemporaryIn.Header, sizeof(MessageHeader<T>)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (_msgTemporaryIn.Header.Size > 0)
					{
						_msgTemporaryIn.Body.resize(_msgTemporaryIn.Header.Size);
						DoReadBody();
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

	void DoReadBody()
	{
		asio::async_read(_socket, asio::buffer(_msgTemporaryIn.Body.data(), _msgTemporaryIn.Body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					AddToIncomingMessageQueue();
				}
				else
				{
					Disconnect();
				}
			});
	}

	void DoWriteHeader()
	{
		asio::async_write(_socket, asio::buffer(&_messagesOut.Front().Header, sizeof(MessageHeader<T>)),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (_messagesOut.Front().Body.size() > 0)
					{
						DoWriteBody();
					}
					// Header-only message
					else
					{
						_messagesOut.PopFront();
						if (!_messagesOut.Empty())
						{
							DoWriteHeader();
						}
					}
				}
				else
				{
					Disconnect();
				}
			});
	}

	void DoWriteBody()
	{
		asio::async_write(_socket, asio::buffer(_messagesOut.Front().Body.data(), _messagesOut.Front().Body.size()),
			[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					_messagesOut.PopFront();
					if (!_messagesOut.Empty())
					{
						DoWriteHeader();
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
		_msgTemporaryIn.Remote = nullptr;
		_messagesIn.PushBack(_msgTemporaryIn);

		DoReadHeader();
	}

public:
	Delegate<> Connected;
	Delegate<Message<T>> MessageReceived;
	Delegate<> Disconnected;

private:
	asio::io_context _asioContext;
	asio::ip::tcp::socket _socket;
	std::thread _threadContext;

	QueueThreadSafe<Message<T>> _messagesIn;
	QueueThreadSafe<Message<T>> _messagesOut;

	Message<T> _msgTemporaryIn;

	bool _isConnected = false;
};


template<typename T>
class TCPServer
{
public:
	TCPServer(uint16_t port)
		: _AsioContext(), _AsioAcceptor(_AsioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{

	}

	virtual ~TCPServer()
	{
		Stop();
	}

	bool StartServer()
	{
		try
		{
			DoAccept();
			_ThreadContext = std::thread([this]() { _AsioContext.run(); });
		}
		catch (std::exception& e)
		{
			std::cerr << "[SERVER] Exception: " << e.what() << "\n";
			return false;
		}

		std::cout << "[SERVER] Started!\n";
		return true;
	}

	void Stop()
	{
		_AsioContext.stop();

		if (_ThreadContext.joinable())
		{
			_ThreadContext.join();
		}

		std::cout << "[SERVER] Stopped!\n";
	}

	void DoAccept()
	{
		_AsioAcceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

					auto newConnection = std::make_shared<ClientSession<T>>(std::move(socket), _AsioContext, _QMessagesIn);
					ClientConnected(newConnection);

					if (CanConnect(newConnection))
					{
						_DeqConnections.push_back(newConnection);
						newConnection->ConnectToClient(_IDCounter++);
						ClientApproved(newConnection);

						std::cout << "[" << _DeqConnections.back()->GetID() << "] Connection Approved\n";
					}
					else
					{
						std::cout << "[-----] Connection Denied\n";
					}
				}
				else
				{
					std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
				}

				DoAccept();
			});
	}

	bool CanConnect(std::shared_ptr<ClientSession<T>> connection)
	{
		return true;
	}

	// Send a message to a specific client
	void MessageClient(std::shared_ptr<ClientSession<T>> client, const Message<T>& msg)
	{
		// Check client is legitimate...
		if (client && client->IsConnected())
		{
			// ...and post the message via the connection
			client->Send(msg);
		}
		else
		{
			ClientDisconnected(client);

			client.reset();

			_DeqConnections.erase(
				std::remove(_DeqConnections.begin(), _DeqConnections.end(), client), _DeqConnections.end());
		}
	}

	// Send message to all clients
	void MessageAllClients(const Message<T>& msg, std::shared_ptr<ClientSession<T>> pIgnoreClient = nullptr)
	{
		bool bInvalidClientExists = false;

		// Iterate through all clients in container
		for (auto& client : _DeqConnections)
		{
			// Check client is connected...
			if (client && client->IsConnected())
			{
				// ..it is!
				if (client != pIgnoreClient)
				{
					client->Send(msg);
				}
			}
			else
			{
				ClientDisconnected(client);
				client.reset();

				bInvalidClientExists = true;
			}
		}

		// Remove dead clients, all in one go - this way, we dont invalidate the
		// container as we iterated through it.
		if (bInvalidClientExists)
		{
			_DeqConnections.erase(
				std::remove(_DeqConnections.begin(), _DeqConnections.end(), nullptr), _DeqConnections.end());
		}
	}

	// Force server to respond to incoming messages
	void UpdateNetwork(size_t maxMessages = -1, bool wait = true)
	{
		if (wait)
		{
			_QMessagesIn.Wait();
		}

		// Process as many messages as you can up to the value
		// specified
		size_t messageCount = 0;
		while (messageCount < maxMessages && !_QMessagesIn.Empty())
		{
			// Grab the front message
			auto msg = _QMessagesIn.PopFront();

			// Pass to message handler
			MessageReceived(msg);

			++messageCount;
		}
	}

public:
	Delegate<std::shared_ptr<ClientSession<T>>> ClientConnected;
	Delegate<std::shared_ptr<ClientSession<T>>> ClientApproved;
	Delegate<Message<T>> MessageReceived;
	Delegate<std::shared_ptr<ClientSession<T>>> ClientDisconnected;
private:
	// Thread Safe Queue for incoming message packets
	QueueThreadSafe<Message<T>> _QMessagesIn;

	// Container of active validated connections
	std::deque<std::shared_ptr<ClientSession<T>>> _DeqConnections;

	// Order of declaration is important - it is also the order of initialisation
	asio::io_context _AsioContext;
	std::thread _ThreadContext;

	// These things need an asio context
	asio::ip::tcp::acceptor _AsioAcceptor; // Handles new incoming connection attempts...

	Message<T> _msgTemporaryIn;

	// Clients will be identified in the "wider system" via an ID
	uint32_t _IDCounter = 1;
};

/*
template<typename T>
class UDPServer
{
public:
	UDPServer(uint16_t port)
		: _AsioContext(), _AsioAcceptor(_AsioContext, asio::ip::udp::endpoint(asio::ip::tcp::v4(), port))
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
			_ThreadContext = std::thread([this]() { _AsioContext.run(); });
		}
		catch (std::exception& e)
		{
			std::cerr << "[SERVER] Exception: " << e.what() << "\n";
			return false;
		}

		std::cout << "[SERVER] Started!\n";
		return true;
	}

	void Stop()
	{
		_AsioContext.stop();

		if (_ThreadContext.joinable())
		{
			_ThreadContext.join();
		}

		std::cout << "[SERVER] Stopped!\n";
	}

	void DoReceive()
	{
		_socket.async_receive_from(
			asio::buffer(data_, max_length), sender_endpoint_,
			[this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

					auto newConnection = std::make_shared<ClientSession<T>>(std::move(socket), _AsioContext, _QMessagesIn);
					ClientConnected(newConnection);

					if (CanConnect(newConnection))
					{
						_DeqConnections.push_back(newConnection);
						newConnection->ConnectToClient(_IDCounter++);
						ClientApproved(newConnection);

						std::cout << "[" << _DeqConnections.back()->GetID() << "] Connection Approved\n";
					}
					else
					{
						std::cout << "[-----] Connection Denied\n";
					}
				}
				else
				{
					std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
				}

				DoAccept();
			});
	}

	void DoReadHeader()
	{
		asio::ip:udp::endpoint _senderEndpoint;
		_socket.async_receive_from(
			asio::buffer(&_msgTemporaryIn.Header, sizeof(MessageHeader<T>)),
			_senderEndpoint,
			[this](std::error_code ec, std::size_t bytesReceived)
			{
				if (!ec)
				{
					if (_msgTemporaryIn.Header.Size > 0)
					{
						_msgTemporaryIn.Body.resize(_msgTemporaryIn.Header.Size);
						DoReadBody();
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

	void DoReadBody()
	{
		asio::ip:udp::endpoint _senderEndpoint;
		_socket.async_receive_from(asio::buffer(_msgTemporaryIn.Body.data(), _msgTemporaryIn.Body.size()),
			_senderEndpoint,
			[this](std::error_code ec, std::size_t bytesReceived)
			{
				if (!ec)
				{
					AddToIncomingMessageQueue();
				}
				else
				{
					Disconnect();
				}
			});
	}

	bool CanConnect(std::shared_ptr<ClientSession<T>> connection)
	{
		return true;
	}

	// Send a message to a specific client
	void MessageClient(std::shared_ptr<ClientSession<T>> client, const Message<T>& msg)
	{
		// Check client is legitimate...
		if (client && client->IsConnected())
		{
			// ...and post the message via the connection
			client->Send(msg);
		}
		else
		{
			ClientDisconnected(client);

			client.reset();

			_DeqConnections.erase(
				std::remove(_DeqConnections.begin(), _DeqConnections.end(), client), _DeqConnections.end());
		}
	}

	// Send message to all clients
	void MessageAllClients(const Message<T>& msg, std::shared_ptr<ClientSession<T>> pIgnoreClient = nullptr)
	{
		bool bInvalidClientExists = false;

		// Iterate through all clients in container
		for (auto& client : _DeqConnections)
		{
			// Check client is connected...
			if (client && client->IsConnected())
			{
				// ..it is!
				if (client != pIgnoreClient)
				{
					client->Send(msg);
				}
			}
			else
			{
				ClientDisconnected(client);
				client.reset();

				bInvalidClientExists = true;
			}
		}

		// Remove dead clients, all in one go - this way, we dont invalidate the
		// container as we iterated through it.
		if (bInvalidClientExists)
		{
			_DeqConnections.erase(
				std::remove(_DeqConnections.begin(), _DeqConnections.end(), nullptr), _DeqConnections.end());
		}
	}

	// Force server to respond to incoming messages
	void UpdateNetwork(size_t maxMessages = -1, bool wait = true)
	{
		if (wait)
		{
			_QMessagesIn.Wait();
		}

		// Process as many messages as you can up to the value
		// specified
		size_t messageCount = 0;
		while (messageCount < maxMessages && !_QMessagesIn.Empty())
		{
			// Grab the front message
			auto msg = _QMessagesIn.PopFront();

			// Pass to message handler
			MessageReceived(msg);

			++messageCount;
		}
	}

public:
	Delegate<std::shared_ptr<ClientSession<T>>> ClientConnected;
	Delegate<std::shared_ptr<ClientSession<T>>> ClientApproved;
	Delegate<Message<T>> MessageReceived;
	Delegate<std::shared_ptr<ClientSession<T>>> ClientDisconnected;
private:
	// Thread Safe Queue for incoming message packets
	QueueThreadSafe<Message<T>> _QMessagesIn;

	// Container of active validated connections
	std::deque<std::shared_ptr<ClientSession<T>>> _DeqConnections;

	// Order of declaration is important - it is also the order of initialisation
	asio::io_context _AsioContext;
	std::thread _ThreadContext;

	// These things need an asio context
	asio::ip::udp::socket _socket; // Handles new incoming connection attempts...

	Message<T> _msgTemporaryIn;

	// Clients will be identified in the "wider system" via an ID
	uint32_t _IDCounter = 1;
};
*/
