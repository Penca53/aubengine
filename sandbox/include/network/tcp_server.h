#pragma once

#include "network/client_session.h"
#include "network/delegate.h"

template <typename T>
class TCPServer {
 public:
  TCPServer(uint16_t port)
      : _AsioContext(),
        _AsioAcceptor(_AsioContext,
                      asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}

  virtual ~TCPServer() { Stop(); }

  bool StartServer() {
    try {
      DoAccept();
      _ThreadContext = std::thread([this]() { _AsioContext.run(); });
    } catch (std::exception& e) {
      std::cerr << "[SERVER] Exception: " << e.what() << "\n";
      return false;
    }

    std::cout << "[SERVER] Started!\n";
    return true;
  }

  void Stop() {
    _AsioContext.stop();

    if (_ThreadContext.joinable()) {
      _ThreadContext.join();
    }

    std::cout << "[SERVER] Stopped!\n";
  }

  void DoAccept() {
    _AsioAcceptor.async_accept([this](std::error_code ec,
                                      asio::ip::tcp::socket socket) {
      if (!ec) {
        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint()
                  << "\n";

        auto newConnection = std::make_shared<ClientSession<T>>(
            std::move(socket), _AsioContext, messages_in_);
        ClientConnected(newConnection);

        if (CanConnect(newConnection)) {
          Connections.push_back(newConnection);
          newConnection->ConnectToClient(_IDCounter++);
          ClientApproved(newConnection);

          std::cout << "[" << Connections.back()->GetID()
                    << "] Connection Approved\n";
        } else {
          std::cout << "[-----] Connection Denied\n";
        }
      } else {
        std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
      }

      DoAccept();
    });
  }

  bool CanConnect(std::shared_ptr<ClientSession<T>> connection) { return true; }

  // Send a message to a specific client
  void SendMessageTo(const Message<T>& msg,
                     std::shared_ptr<ClientSession<T>> client) {
    // Check client is legitimate...
    if (client && client->IsConnected()) {
      // ...and post the message via the connection
      client->Send(msg);
    } else {
      ClientDisconnected(client);

      client.reset();

      Connections.erase(
          std::remove(Connections.begin(), Connections.end(), client),
          Connections.end());
    }
  }

  // Send message to all clients
  void SendMessageToAll(
      const Message<T>& msg,
      std::shared_ptr<ClientSession<T>> ignoreClient = nullptr) {
    bool doesInvalidClientExist = false;

    // Iterate through all clients in container
    for (const auto& client : Connections) {
      // Check client is connected...
      if (client && client->IsConnected()) {
        // ..it is!
        if (client != ignoreClient) {
          client->Send(msg);
        }
      } else {
        ClientDisconnected(client);
        client.reset();

        doesInvalidClientExist = true;
      }
    }

    // Remove dead clients, all in one go - this way, we dont invalidate the
    // container as we iterated through it.
    if (doesInvalidClientExist) {
      Connections.erase(
          std::remove(Connections.begin(), Connections.end(), nullptr),
          Connections.end());
    }
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
  Delegate<std::shared_ptr<ClientSession<T>>> ClientConnected;
  Delegate<std::shared_ptr<ClientSession<T>>> ClientApproved;
  Delegate<Message<T>> MessageReceived;
  Delegate<std::shared_ptr<ClientSession<T>>> ClientDisconnected;
  // Container of active validated connections
  std::deque<std::shared_ptr<ClientSession<T>>> Connections;

 private:
  // Thread Safe Queue for incoming message packets
  QueueThreadSafe<Message<T>> messages_in_;

  // Order of declaration is important - it is also the order of initialisation
  asio::io_context _AsioContext;
  std::thread _ThreadContext;

  // These things need an asio context
  asio::ip::tcp::acceptor
      _AsioAcceptor;  // Handles new incoming connection attempts...

  Message<T> _msgTemporaryIn;

  // Clients will be identified in the "wider system" via an ID
  uint32_t _IDCounter = 1;
};