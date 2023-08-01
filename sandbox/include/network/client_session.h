#pragma once

#include <asio.hpp>

#include "network/message.h"
#include "network/queue_thread_safe.h"

template <typename T>
class ClientSession : public std::enable_shared_from_this<ClientSession<T>> {
 public:
  ClientSession(asio::ip::tcp::socket tcpSocket, asio::io_context& asioContext,
                QueueThreadSafe<Message<T>>& qIn)
      : _asioContext(asioContext),
        tcp_socket_(std::move(tcpSocket)),
        messages_in_(qIn) {}

  virtual ~ClientSession() { std::cout << "Destroy Connection\n"; }

 public:
  void ConnectToClient(uint32_t id) {
    if (tcp_socket_.is_open()) {
      id_ = id;
      DoReadTCPHeader();
      is_connected_ = true;
    }
  }

  void Send(const Message<T>& msg) {
    asio::post(_asioContext, [this, msg]() {
      bool isWritingMessage = !messages_out_.Empty();
      messages_out_.PushBack(msg);
      if (!isWritingMessage) {
        DoWriteTCPHeader();
      }
    });
  }

  void Disconnect() {
    if (IsConnected()) {
      asio::post(_asioContext, [this]() { tcp_socket_.close(); });
    }

    is_connected_ = false;
  }

  bool IsConnected() const { return is_connected_; }

  uint32_t GetID() { return id_; }

  bool TryGetUDPEndpoint(asio::ip::udp::endpoint& endpoint) {
    if (UDPPort == 0) {
      return false;
    }

    // Resolve hostname/ip-address into tangiable physical address
    auto host = tcp_socket_.remote_endpoint().address().to_string();
    auto port = std::to_string(UDPPort);

    asio::ip::udp::resolver resolver(_asioContext);
    asio::ip::udp::resolver::results_type endpoints =
        resolver.resolve(host, port);
    endpoint = *endpoints.begin();

    return true;
  }

 private:
  void DoReadTCPHeader() {
    asio::async_read(
        tcp_socket_,
        asio::buffer(&_msgTemporaryIn.Header, sizeof(MessageHeader<T>)),
        [this](std::error_code ec, std::size_t) {
          if (!ec) {
            if (_msgTemporaryIn.Header.Size > 0) {
              _msgTemporaryIn.Body.resize(_msgTemporaryIn.Header.Size);
              DoReadTCPBody();
            }
            // Header-only message
            else {
              AddToIncomingMessageQueue();
            }
          } else {
            Disconnect();
          }
        });
  }

  void DoReadTCPBody() {
    asio::async_read(
        tcp_socket_,
        asio::buffer(_msgTemporaryIn.Body.data(), _msgTemporaryIn.Body.size()),
        [this](std::error_code ec, std::size_t) {
          if (!ec) {
            AddToIncomingMessageQueue();
            DoReadTCPHeader();
          } else {
            Disconnect();
          }
        });
  }

  void DoWriteTCPHeader() {
    asio::async_write(
        tcp_socket_,
        asio::buffer(&messages_out_.Front().Header, sizeof(MessageHeader<T>)),
        [this](std::error_code ec, std::size_t) {
          if (!ec) {
            if (messages_out_.Front().Body.size() > 0) {
              DoWriteTCPBody();
            }
            // Header-only message
            else {
              messages_out_.PopFront();
              if (!messages_out_.Empty()) {
                DoReadTCPHeader();
              }
            }
          } else {
            Disconnect();
          }
        });
  }
  void DoWriteTCPBody() {
    asio::async_write(tcp_socket_,
                      asio::buffer(messages_out_.Front().Body.data(),
                                   messages_out_.Front().Body.size()),
                      [this](std::error_code ec, std::size_t) {
                        if (!ec) {
                          messages_out_.PopFront();
                          if (!messages_out_.Empty()) {
                            DoReadTCPHeader();
                          }
                        } else {
                          Disconnect();
                        }
                      });
  }

  void AddToIncomingMessageQueue() {
    _msgTemporaryIn.TCPRemote = this->shared_from_this();
    messages_in_.PushBack(_msgTemporaryIn);
  }

 public:
  uint16_t UDPPort = 0;

 private:
  asio::io_context& _asioContext;

  asio::ip::tcp::socket tcp_socket_;

  QueueThreadSafe<Message<T>>& messages_in_;
  QueueThreadSafe<Message<T>> messages_out_;

  Message<T> _msgTemporaryIn;

  uint32_t id_ = 0;
  bool is_connected_ = false;
};