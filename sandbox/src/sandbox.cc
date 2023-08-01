#include <deque>
#include <filesystem>
#include <iostream>
#include <limits>

#include "aubengine/application.h"
#include "aubengine/components/box_collider_2d.h"
#include "aubengine/components/sprite_renderer_2d.h"
#include "aubengine/components/transform.h"
#include "aubengine/game_object.h"
#include "aubengine/input.h"
#include "aubengine/prefab.h"
#include "aubengine/resource_manager.h"
#include "aubengine/shader.h"
#include "aubengine/sprite_renderer.h"
#include "network/net.h"

bool isServer = true;

class FPS {
 protected:
  unsigned int _fps;
  unsigned int _fpsCount;
  std::chrono::steady_clock::time_point last;

 public:
  // Constructor
  FPS() : _fps(0), _fpsCount(0) { last = std::chrono::steady_clock::now(); }

  // Update
  void Update() {
    // increase the counter by one
    _fpsCount++;

    auto now = std::chrono::steady_clock::now();

    // one second elapsed? (= 1000 milliseconds)
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last)
            .count() > 1000) {
      // save the current counter value to m_fps
      _fps = _fpsCount;

      // reset the counter and the interval
      _fpsCount = 0;
      last = now;
    }
  }

  // Get fps
  unsigned int Get() const { return _fps; }
};

struct InputData {
  double PressTime = 0;
  unsigned int InputSequenceNumber = 0;
  unsigned int EntityId = 0;
};

struct Position {
  std::chrono::steady_clock::time_point timestamp;
  double position = 0;
};

class Entity {
 public:
  Entity() {}

  double X = 400;
  double Y = 300;
  double Speed = 100;
  std::deque<Position> PositionBuffer;
  unsigned int EntityId = 0;

 public:
  void ApplyInput(InputData input) { X += input.PressTime * Speed; }
};
struct NetState {
  uint32_t EntityId = 0;
  double Position = 0;
  uint32_t LastProcessedInput = 0;
};

class MovementManager : public Component {
 public:
  virtual void Start() override {
    transform_ = game_object->GetComponent<Transform>();
  }

  virtual void Update() override {
    if (entity_) {
      transform_->position.x = entity_->X;
    }
  }

  void SetEntity(Entity* entity) { entity_ = entity; }

 private:
  Transform* transform_ = nullptr;
  Entity* entity_ = nullptr;
};

class PlayerPaddlePrefab : public GameObject {
 public:
  PlayerPaddlePrefab(Scene* scene) : GameObject("PlayerPaddle", scene) {
    Transform* transform = AddComponent<Transform>();
    auto shader = ResourceManager::GetShader("default");
    auto texture = ResourceManager::GetTexture("paddle");

    AddComponent<SpriteRenderer2D>(shader, texture);
    AddComponent<BoxCollider2D>();
    AddComponent<MovementManager>();

    transform->position = {400, 300, 0};
    transform->size = {512 / 5.0, 128 / 5.0, 1};
  }
};

class BlockPrefab : public GameObject {
 public:
  BlockPrefab(Scene* scene) : GameObject("Block", scene) {
    Transform* transform = AddComponent<Transform>();
    auto shader = ResourceManager::GetShader("default");
    auto texture = ResourceManager::GetTexture("block");

    AddComponent<SpriteRenderer2D>(shader, texture);
    AddComponent<BoxCollider2D>();

    transform->position = {400, 300, 0};
    transform->size = {128 / 4.0, 128 / 4.0, 1};
  }
};

enum class CustomMsgTypes : uint32_t {
  ServerAccept,
  ServerDeny,
  ServerPing,
  ClientUDPPort,
  Movement,
  WorldState,
};

class NetworkClient : public Component {
 public:
  NetworkClient() {}

  int ticks = 0;
  void Start() override {
    _tcpClient.Connected += [this]() { OnConnect(); };
    _tcpClient.MessageReceived +=
        [this](Message<CustomMsgTypes> msg) { OnMessage(msg); };
    _tcpClient.Disconnected += [this]() { OnDisconnect(); };

    _tcpClient.Connect("127.0.0.1", 60000);

    _udpClient.Connected += [this]() { OnConnect(); };
    _udpClient.MessageReceived +=
        [this](Message<CustomMsgTypes> msg) { OnMessage(msg); };
    _udpClient.Disconnected += [this]() { OnDisconnect(); };

    _udpClient.Connect("127.0.0.1", 60000);
  }

  void Update() override {
    _tcpClient.UpdateNetwork(-1, false);
    _udpClient.UpdateNetwork(-1, false);
    if (!_tcpClient.IsConnected()) {
      return;
    }

    if (ticks == 0) {
      PingServer();
    }
    ticks = (ticks + 1) % 60;

    if (_entityId == 0) {
      return;
    }

    ProcessInputs();

    if (_entityInterpolation) {
      InterpolateEntities();
    }
  }

 public:
  void PingServer() {
    Message<CustomMsgTypes> msg;
    msg.Header.ID = CustomMsgTypes::ServerPing;

    // Caution with this...
    auto timeNow = std::chrono::steady_clock::now();

    msg.Write(timeNow);
    _udpClient.Send(msg);
  }

 protected:
  void OnConnect() { std::cout << "Connected to the server\n"; }

  // Called when a client appears to have disconnected
  void OnDisconnect() { std::cout << "Disconnected from the server\n"; }

  double Ping = 0;
  // Called when a message arrives
  void OnMessage(Message<CustomMsgTypes>& msg) {
    switch (msg.Header.ID) {
      case CustomMsgTypes::ServerAccept: {
        _entityId = msg.Read<unsigned int>();
        std::cout << "Server Accepted ClientSession - Client ID: " << _entityId
                  << "\n";

        Message<CustomMsgTypes> newMsg;
        newMsg.Header.ID = CustomMsgTypes::ClientUDPPort;
        uint16_t port = _udpClient.GetSocketPort();
        newMsg.Write(port);

        _tcpClient.Send(newMsg);
        break;
      }
      case CustomMsgTypes::ServerPing: {
        // Server has responded to a ping request
        std::chrono::steady_clock::time_point timeNow =
            std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point timeThen;
        timeThen = msg.Read<std::chrono::steady_clock::time_point>();
        Ping = std::chrono::duration<double>(timeNow - timeThen).count();
        std::cout << "Ping: " << Ping << "\n";
        break;
      }
      case CustomMsgTypes::WorldState: {
        std::vector<NetState> states = msg.ReadVector<NetState>();
        for (const auto& state : states) {
          if (!_entities.count(state.EntityId)) {
            Entity* entity = new Entity();
            entity->EntityId = state.EntityId;
            _entities[state.EntityId] = entity;

            PlayerPaddlePrefab* player =
                game_object->GetScene()->Instantiate<PlayerPaddlePrefab>();
            player->GetComponent<MovementManager>()->SetEntity(entity);
          }

          Entity* entity = _entities[state.EntityId];
          if (state.EntityId == _entityId) {
            entity->X = state.Position;

            if (_serverReconciliation) {
              size_t j = 0;
              while (j < _pendingInputs.size()) {
                InputData input = _pendingInputs[j];
                if (input.InputSequenceNumber <= state.LastProcessedInput) {
                  _pendingInputs.erase(_pendingInputs.begin() + j);
                } else {
                  entity->ApplyInput(input);
                  ++j;
                }
              }
            } else {
              _pendingInputs.clear();
            }
          } else {
            if (!_entityInterpolation) {
              entity->X = state.Position;
            } else {
              auto now = std::chrono::steady_clock::now();
              entity->PositionBuffer.push_back({now, state.Position});
            }
          }
        }
        break;
      }
    }
  }

  float currentVelocity = 0;
  void ProcessInputs() {
    auto nowTs = std::chrono::steady_clock::now();
    if (_isFirstTs) {
      _lastTs = nowTs;
      _isFirstTs = false;
    }

    auto dtSec =
        std::chrono::duration_cast<std::chrono::milliseconds>(nowTs - _lastTs)
            .count() /
        1000.0;
    _lastTs = nowTs;

    InputData input;
    if (Input::GetKey('A')) {
      input.PressTime = -dtSec;
    } else if (Input::GetKey('D')) {
      input.PressTime = dtSec;
    } else {
      return;
    }

    input.InputSequenceNumber = _inputSequenceNumber++;
    input.EntityId = _entityId;

    // Send to server InputData
    Message<CustomMsgTypes> msg;
    msg.Header.ID = CustomMsgTypes::Movement;

    msg.Write(input);
    _tcpClient.Send(msg);

    if (_clientSidePrediction) {
      _entities[_entityId]->ApplyInput(input);
    }

    _pendingInputs.push_back(input);
  }

  void InterpolateEntities() {
    auto now = std::chrono::steady_clock::now();
    auto renderTimestamp = now - std::chrono::milliseconds(400);
    for (const auto& it : _entities) {
      Entity* entity = it.second;
      if (entity->EntityId == _entityId) {
        continue;
      }

      std::deque<Position>& buffer = entity->PositionBuffer;

      while (buffer.size() >= 2 && buffer[1].timestamp <= renderTimestamp) {
        buffer.pop_front();
      }

      if (buffer.size() >= 2 && buffer[0].timestamp <= renderTimestamp &&
          renderTimestamp <= buffer[1].timestamp) {
        auto x0 = buffer[0].position;
        auto x1 = buffer[1].position;
        auto t0 = buffer[0].timestamp;
        auto t1 = buffer[1].timestamp;

        entity->X = x0 + ((x1 - x0) * (renderTimestamp - t0)) / (t1 - t0);
      }
    }
  }

 private:
  TCPClient<CustomMsgTypes> _tcpClient;
  UDPClient<CustomMsgTypes> _udpClient;

  std::unordered_map<int, Entity*> _entities;
  unsigned int _entityId = 0;

  bool _clientSidePrediction = true;
  bool _serverReconciliation = true;
  unsigned int _inputSequenceNumber = 0;
  std::vector<InputData> _pendingInputs;

  bool _entityInterpolation = false;

  bool _isFirstTs = true;
  std::chrono::steady_clock::time_point _lastTs;
};

class NetworkServer : public Component {
 public:
  NetworkServer(uint16_t port) : _tcpServer(port), _udpServer(port) {}

  virtual void Start() override {
    _tcpServer.ClientConnected +=
        [this](std::shared_ptr<ClientSession<CustomMsgTypes>> client) {
          OnClientConnect(client);
        };
    _tcpServer.ClientApproved +=
        [this](std::shared_ptr<ClientSession<CustomMsgTypes>> client) {
          OnClientApprove(client);
        };
    _tcpServer.MessageReceived +=
        [this](Message<CustomMsgTypes> msg) { OnMessage(msg); };
    _tcpServer.ClientDisconnected +=
        [this](std::shared_ptr<ClientSession<CustomMsgTypes>> client) {
          OnClientDisconnect(client);
        };
    _tcpServer.StartServer();

    _udpServer.MessageReceived +=
        [this](Message<CustomMsgTypes> msg) { OnMessage(msg); };
    _udpServer.StartServer();
  }
  virtual void Update() override {
    _tcpServer.UpdateNetwork(-1, false);
    _udpServer.UpdateNetwork(-1, false);
    SendWorldState();
  }

 protected:
  void OnClientConnect(std::shared_ptr<ClientSession<CustomMsgTypes>> client) {}

  void OnClientApprove(std::shared_ptr<ClientSession<CustomMsgTypes>> client) {
    Message<CustomMsgTypes> msg;
    msg.Header.ID = CustomMsgTypes::ServerAccept;
    msg.Write(client->GetID());

    Entity e;
    e.EntityId = client->GetID();
    _entities[client->GetID()] = e;

    client->Send(msg);
  }

  // Called when a client appears to have disconnected
  void OnClientDisconnect(
      std::shared_ptr<ClientSession<CustomMsgTypes>> client) {
    std::cout << "Removing client [" << client->GetID() << "]\n";
  }

  // Called when a message arrives
  void OnMessage(Message<CustomMsgTypes>& msg) {
    switch (msg.Header.ID) {
      case CustomMsgTypes::ServerPing: {
        // std::cout << "[" << msg.Remote->GetID() << "]: Server Ping\n";
        std::cout << "UDP Server Ping\n";

        // Simply bounce message back to client
        // msg.Remote->Send(msg);
        _udpServer.Send(msg);
        break;
      }
      case CustomMsgTypes::ClientUDPPort: {
        // std::cout << "[" << msg.Remote->GetID() << "]: Server Ping\n";
        std::cout << "TCP ClientUDPPort Ping\n";

        uint16_t port = msg.Read<uint16_t>();
        std::cout << "Their port is " << port << "\n";

        msg.TCPRemote->UDPPort = port;
        break;
      }
      case CustomMsgTypes::Movement: {
        InputData input = msg.Read<InputData>();
        unsigned int id = input.EntityId;
        if (!_entities.count(id)) {
          break;
        }

        if (ValidateInput(input)) {
          _entities[id].ApplyInput(input);
          _lastProcessedInput[id] = input.InputSequenceNumber;
        }
        break;
      }
    }
  }

  bool ValidateInput(InputData input) {
    if (abs(input.PressTime) > 1.0 / 40.0) {
      return false;
    } else {
      return true;
    }
  }

  void SendWorldState() {
    Message<CustomMsgTypes> msg;
    msg.Header.ID = CustomMsgTypes::WorldState;
    std::vector<NetState> states;
    for (const auto& it : _entities) {
      NetState state;
      state.EntityId = it.second.EntityId;
      state.Position = it.second.X;
      state.LastProcessedInput = _lastProcessedInput[it.first];
      states.push_back(state);
    }

    msg.WriteVector(states);
    for (const auto& conn : _tcpServer.Connections) {
      asio::ip::udp::endpoint endpoint;
      if (conn->TryGetUDPEndpoint(endpoint)) {
        msg.UDPRemote = endpoint;
        _udpServer.Send(msg);
      }
    }
  }

 private:
  TCPServer<CustomMsgTypes> _tcpServer;
  UDPServer<CustomMsgTypes> _udpServer;
  std::unordered_map<int, Entity> _entities;
  std::unordered_map<int, unsigned int> _lastProcessedInput;
};

class NetworkClientPrefab : public GameObject {
 public:
  NetworkClientPrefab(Scene* scene) : GameObject("NetworkClient", scene) {
    AddComponent<NetworkClient>();
  }
};

class NetworkServerPrefab : public GameObject {
 public:
  NetworkServerPrefab(Scene* scene) : GameObject("NetworkServer", scene) {
    AddComponent<NetworkServer>(60000);
  }
};

class MainScene : public Scene {
 public:
  MainScene(Window* window, SpriteRenderer* renderer)
      : Scene(window, renderer) {
    auto ctx = static_cast<GladGLContext*>(window->GetContext());
    ResourceManager::LoadShader("../../aubengine/src/shaders/default.vs.glsl",
                                "../../aubengine/src/shaders/default.fs.glsl",
                                "default", ctx);
    ResourceManager::LoadTexture("../../aubengine/src/textures/block.png",
                                 false, "block", ctx);
    ResourceManager::LoadTexture("../../aubengine/src/textures/paddle.png",
                                 true, "paddle", ctx);

    if (isServer) {
      networkServer = Instantiate<NetworkServerPrefab>();
    } else {
      networkClient = Instantiate<NetworkClientPrefab>();
    }

    for (int i = 0; i < 5; ++i) {
      auto block = Instantiate<BlockPrefab>();
      block->GetComponent<Transform>()->position.x = i * 100 + 100;
    }
  }

 public:
  GameObject* networkServer = nullptr;
  GameObject* networkClient = nullptr;
};

void TesterInitializer() {
  auto& app = Aubengine::Application::GetInstance();

  auto window1 = app.CreateWindowOpenGL();
  window1->Initialize("First", 800, 600);
  window1->SetVSync(true);

  window1->Use();
  SpriteRenderer renderer;
  std::shared_ptr<MainScene> mainScene1 =
      std::make_shared<MainScene>(window1, &renderer);
  window1->SetScene(mainScene1);

  if (isServer) {
    return;
  }
  auto window2 = app.CreateWindowOpenGL();
  window2->Initialize("Second", 800, 600);
  window2->SetVSync(true);

  window2->Use();
  std::shared_ptr<MainScene> mainScene2 =
      std::make_shared<MainScene>(window2, &renderer);
  window2->SetScene(mainScene2);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "false") == 0) {
      isServer = false;
    } else {
      isServer = true;
    }
  }

  TesterInitializer();

  if (isServer) {
    Aubengine::Application::GetInstance().SetUpdateRate(50).Run();
  } else {
    Aubengine::Application::GetInstance().SetUpdateRate(60).Run();
  }

  return 0;
}
