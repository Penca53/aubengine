#pragma once

#include <asio.hpp>

template <typename T>
class ClientSession;

enum class SendMode {
  UDP,
  TCP,
};

// Message _header is sent at start of all messages. The template allows us
// to use "enum class" to ensure that the messages are valid at compile time
template <typename T>
class MessageHeader {
 public:
  T ID{};
  uint32_t Size = 0;
};

// Message _body contains a header and a std::vector, containing raw bytes
// of infomation. This way the message can be variable length, but the size
// in the header must be updated.
template <typename T>
class Message {
 public:
  void FromBuffer(uint8_t* buffer, size_t length) {
    std::memcpy(&Header, buffer, sizeof(MessageHeader<T>));
    Body.resize(Header.Size);
    std::memcpy(Body.data(), buffer + sizeof(MessageHeader<T>),
                length - sizeof(MessageHeader<T>));
  }

  size_t ToBuffer(uint8_t* buffer) {
    std::memcpy(buffer, &Header, sizeof(MessageHeader<T>));
    std::memcpy(buffer + sizeof(MessageHeader<T>), Body.data(), Body.size());

    size_t length = (sizeof(MessageHeader<T>) + Body.size());
    return length;
  }

  // returns size of entire message packet in bytes
  size_t Size() const { return Body.size() - _begin; }

  // Override for std::cout compatibility - produces friendly description of
  // message
  friend std::ostream& operator<<(std::ostream& os, const Message<T>& msg) {
    os << "ID:" << int(msg.Header.ID) << " Size:" << msg.Header.Size;
    return os;
  }

  template <typename D>
  void Write(const D& data) {
    static_assert(std::is_standard_layout<D>::value,
                  "Data is too complex to be pushed into vector");

    size_t old = Body.size();
    size_t s = sizeof(data);
    Body.resize(Body.size() + sizeof(data));
    std::memcpy(Body.data() + old, &data, sizeof(data));
    Header.Size = Size();
  }

  template <typename D>
  void WriteVector(const std::vector<D>& data) {
    size_t size = data.size();
    Write(size);
    for (size_t i = 0; i < size; ++i) {
      Write(data[i]);
    }
  }

  template <typename D>
  D Read() {
    static_assert(std::is_standard_layout<D>::value,
                  "Data is too complex to be pushed into vector");
    D data{};

    std::memcpy(&data, Body.data() + _begin, sizeof(D));
    _begin += sizeof(D);
    Header.Size = Size();

    return data;
  }

  template <typename D>
  std::vector<D> ReadVector() {
    size_t size = Read<size_t>();
    std::vector<D> data(size);
    for (size_t i = 0; i < size; ++i) {
      data[i] = Read<D>();
    }

    return data;
  }

 public:
  MessageHeader<T> Header{};
  std::vector<uint8_t> Body;

  asio::ip::udp::endpoint UDPRemote;
  std::shared_ptr<ClientSession<T>> TCPRemote = nullptr;

 private:
  size_t _begin = 0;
};