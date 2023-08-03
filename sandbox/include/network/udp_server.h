#pragma once

#include "network/delegate.h"
#include "network/message.h"
#include "network/queue_thread_safe.h"

template <typename T>
class UDPServer {
 public:
  UDPServer(uint16_t port)
      : _udpSocket(_asioContext,
                   asio::ip::udp::endpoint(asio::ip::udp::v4(), port)) {}

  virtual ~UDPServer() { Stop(); }

  bool StartServer() {
    try {
      DoReceive();
      _ThreadContext = std::thread([this]() { _asioContext.run(); });
    } catch (std::exception& e) {
      std::cerr << "[SERVER] Exception: " << e.what() << "\n";
      return false;
    }

    std::cout << "[UDP SERVER] Started!\n";
    return true;
  }

  void Stop() {
    _asioContext.stop();

    if (_ThreadContext.joinable()) {
      _ThreadContext.join();
    }

    std::cout << "[UDP SERVER] Stopped!\n";
  }

  void Send(const Message<T>& msg) {
    asio::post(_asioContext, [this, msg]() {
      bool isWritingMessage = !messages_out_.Empty();
      messages_out_.PushBack(msg);
      if (!isWritingMessage) {
        DoSend();
      }
    });
  }

  void DoReceive() {
    _udpSocket.async_receive_from(
        asio::buffer(_inUdpBuffer, UDP_DATAGRAM_SIZE), _senderEndpoint,
        [this](std::error_code ec, std::size_t bytesReceived) {
          if (!ec) {
            _msgTemporaryIn.FromBuffer(_inUdpBuffer, bytesReceived);
            AddToIncomingMessageQueue();
          }

          DoReceive();
        });
  }
  void DoSend() {
    auto message = messages_out_.Front();
    size_t length = message.ToBuffer(_outUdpBuffer);
    _udpSocket.async_send_to(asio::buffer(_outUdpBuffer, length),
                             message.UDPRemote,
                             [this](std::error_code ec, std::size_t) {
                               messages_out_.PopFront();
                               if (!ec) {
                                 if (!messages_out_.Empty()) {
                                   DoSend();
                                 }
                               } else {
                                 // Error
                               }
                             });
  }

  bool CanConnect(std::shared_ptr<ClientSession<T>> connection) { return true; }

  void AddToIncomingMessageQueue() {
    _msgTemporaryIn.UDPRemote = _senderEndpoint;
    messages_in_.PushBack(_msgTemporaryIn);
  }

  // Force server to respond to incoming messages
  void UpdateNetwork(size_t maxMessages = -1, bool wait = true) {
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
  QueueThreadSafe<Message<T>> messages_in_;
  QueueThreadSafe<Message<T>> messages_out_;

  // Order of declaration is important - it is also the order of initialisation
  asio::io_context _asioContext;
  std::thread _ThreadContext;

  // These things need an asio context
  asio::ip::udp::socket
      _udpSocket;  // Handles new incoming connection attempts...
  asio::ip::udp::endpoint _senderEndpoint;

  Message<T> _msgTemporaryIn;

  // Clients will be identified in the "wider system" via an ID
  uint32_t _IDCounter = 1;
};