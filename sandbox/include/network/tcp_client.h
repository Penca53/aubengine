#pragma once

#include "network/delegate.h"

// Client
template <typename T>
class TCPClient {
 public:
  TCPClient() : _socket(_asioContext) {}

  virtual ~TCPClient() {
    // If the client is destroyed, always try and disconnect from server
    std::cout << "Destroy Client\n";
    Disconnect();
  }

 public:
  // Connect to server with hostname/ip-address and port
  bool Connect(const std::string& host, const uint16_t port) {
    try {
      // Resolve hostname/ip-address into tangiable physical address
      asio::ip::tcp::resolver resolver(_asioContext);
      asio::ip::tcp::resolver::results_type endpoints =
          resolver.resolve(host, std::to_string(port));

      DoConnect(endpoints);

      // Start Context Thread
      _threadContext = std::thread([this]() { _asioContext.run(); });
    } catch (std::exception& e) {
      std::cerr << "Client Exception: " << e.what() << "\n";
      return false;
    }
    return true;
  }

  // Disconnect from server
  void Disconnect() {
    if (IsConnected()) {
      _socket.close();
    }

    _asioContext.stop();
    if (_threadContext.joinable()) {
      _threadContext.join();
    }

    Disconnected();
    is_connected_ = false;
  }

  // Check if client is actually connected to a server
  bool IsConnected() { return is_connected_; }

  void UpdateNetwork(size_t maxMessages = -1, bool wait = false) {
    if (wait) {
      messages_in_.Wait();
    }

    // Process as many messages as you can up to the value
    // specified
    size_t messageCount = 0;
    while (messageCount < maxMessages && !messages_in_.Empty()) {
      // Grab the front message
      auto msg = messages_in_.PopFront();

      // Pass to message handler
      MessageReceived(msg);

      messageCount++;
    }
  }

  void Send(const Message<T>& msg) {
    asio::post(_asioContext, [this, msg]() {
      bool bWritingMessage = !messages_out_.Empty();
      messages_out_.PushBack(msg);
      if (!bWritingMessage) {
        DoWriteHeader();
      }
    });
  }

  // Retrieve queue of messages from server
  QueueThreadSafe<Message<T>>& Incoming() { return messages_in_; }

 private:
  void DoConnect(const asio::ip::tcp::resolver::results_type& endpoints) {
    asio::async_connect(_socket, endpoints,
                        [this](asio::error_code ec, asio::ip::tcp::endpoint) {
                          if (!ec) {
                            is_connected_ = true;
                            DoReadHeader();
                          }
                        });
  }

  void DoReadHeader() {
    asio::async_read(
        _socket,
        asio::buffer(&_msgTemporaryIn.Header, sizeof(MessageHeader<T>)),
        [this](std::error_code ec, std::size_t) {
          if (!ec) {
            if (_msgTemporaryIn.Header.Size > 0) {
              _msgTemporaryIn.Body.resize(_msgTemporaryIn.Header.Size);
              DoReadBody();
            }
            // Header-only message
            else {
              AddToIncomingMessageQueue();
              DoReadHeader();
            }
          } else {
            Disconnect();
          }
        });
  }

  void DoReadBody() {
    asio::async_read(
        _socket,
        asio::buffer(_msgTemporaryIn.Body.data(), _msgTemporaryIn.Body.size()),
        [this](std::error_code ec, std::size_t) {
          if (!ec) {
            AddToIncomingMessageQueue();
            DoReadHeader();
          } else {
            Disconnect();
          }
        });
  }

  void DoWriteHeader() {
    asio::async_write(
        _socket,
        asio::buffer(&messages_out_.Front().Header, sizeof(MessageHeader<T>)),
        [this](std::error_code ec, std::size_t) {
          if (!ec) {
            if (messages_out_.Front().Body.size() > 0) {
              DoWriteBody();
            }
            // Header-only message
            else {
              messages_out_.PopFront();
              if (!messages_out_.Empty()) {
                DoWriteHeader();
              }
            }
          } else {
            Disconnect();
          }
        });
  }

  void DoWriteBody() {
    asio::async_write(_socket,
                      asio::buffer(messages_out_.Front().Body.data(),
                                   messages_out_.Front().Body.size()),
                      [this](std::error_code ec, std::size_t) {
                        if (!ec) {
                          messages_out_.PopFront();
                          if (!messages_out_.Empty()) {
                            DoWriteHeader();
                          }
                        } else {
                          Disconnect();
                        }
                      });
  }

  void AddToIncomingMessageQueue() {
    _msgTemporaryIn.TCPRemote = nullptr;
    messages_in_.PushBack(_msgTemporaryIn);
  }

 public:
  Delegate<> Connected;
  Delegate<Message<T>> MessageReceived;
  Delegate<> Disconnected;

 private:
  asio::io_context _asioContext;
  asio::ip::tcp::socket _socket;
  std::thread _threadContext;

  QueueThreadSafe<Message<T>> messages_in_;
  QueueThreadSafe<Message<T>> messages_out_;

  Message<T> _msgTemporaryIn;

  bool is_connected_ = false;
};