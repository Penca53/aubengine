#pragma once

// Client
template <typename T>
class UDPClient {
 public:
  UDPClient() : _socket(_asioContext) {}

  virtual ~UDPClient() {
    // If the client is destroyed, always try and disconnect from server
    std::cout << "Destroy Client\n";
    Disconnect();
  }

 public:
  // Connect to server with hostname/ip-address and port
  bool Connect(const std::string& host, const uint16_t port) {
    try {
      // Resolve hostname/ip-address into tangiable physical address
      asio::ip::udp::resolver resolver(_asioContext);
      asio::ip::udp::resolver::results_type endpoints =
          resolver.resolve(host, std::to_string(port));

      DoConnect(endpoints);

      // Start Context Thread
      _threadContext = std::thread([this]() { _asioContext.run(); });
    } catch (std::exception& e) {
      std::cerr << "UDP Client Exception: " << e.what() << "\n";
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
        DoSend();
      }
    });
  }

  // Retrieve queue of messages from server
  QueueThreadSafe<Message<T>>& Incoming() { return messages_in_; }

  uint16_t GetSocketPort() { return _socket.local_endpoint().port(); }

 private:
  void DoConnect(const asio::ip::udp::resolver::results_type& endpoints) {
    asio::async_connect(_socket, endpoints,
                        [this](asio::error_code ec, asio::ip::udp::endpoint) {
                          if (!ec) {
                            is_connected_ = true;
                            DoReceive();
                          }
                        });
  }

  void DoReceive() {
    _socket.async_receive(
        asio::buffer(_inUDPBuffer, UDP_DATAGRAM_SIZE),
        [this](std::error_code ec, std::size_t bytesReceived) {
          if (!ec) {
            _msgTemporaryIn.FromBuffer(_inUDPBuffer, bytesReceived);
            AddToIncomingMessageQueue();
          }

          DoReceive();
        });
  }

  void DoSend() {
    auto message = messages_out_.Front();
    size_t length = message.ToBuffer(_outUDPBuffer);
    _socket.async_send(asio::buffer(_outUDPBuffer, length),
                       [this](std::error_code ec, std::size_t) {
                         if (!ec) {
                           messages_out_.PopFront();
                           if (!messages_out_.Empty()) {
                             DoSend();
                           }
                         } else {
                           Disconnect();
                         }
                       });
  }

  void AddToIncomingMessageQueue() {
    _msgTemporaryIn.UDPRemote = {};
    messages_in_.PushBack(_msgTemporaryIn);
  }

 public:
  Delegate<> Connected;
  Delegate<Message<T>> MessageReceived;
  Delegate<> Disconnected;

 private:
  static const int UDP_DATAGRAM_SIZE = 64 * 1024;

  uint8_t _inUDPBuffer[UDP_DATAGRAM_SIZE] = {};
  uint8_t _outUDPBuffer[UDP_DATAGRAM_SIZE] = {};

  asio::io_context _asioContext;
  asio::ip::udp::socket _socket;
  std::thread _threadContext;

  QueueThreadSafe<Message<T>> messages_in_;
  QueueThreadSafe<Message<T>> messages_out_;

  Message<T> _msgTemporaryIn;

  bool is_connected_ = false;
};