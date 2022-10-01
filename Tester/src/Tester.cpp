#include <iostream>
#include <Application.h>
#include <Shader.h>
#include <ResourceManager.h>
#include <SpriteRenderer.h>
#include <Input.h>
#include <GameObject.h>
#include <Components/Transform.h>
#include <Components/SpriteRenderer2D.h>
#include "network/Net.h"
#include "Prefab.h"
#include <deque>
#include <limits>
#include "BufferedLinearInterpolator.h"

bool isServer = false;

class BufferedLinearInterpolatorDouble : public BufferedLinearInterpolator<double>
{
public:
	BufferedLinearInterpolatorDouble(uint32_t bufferCountLimit) : BufferedLinearInterpolator<double>(bufferCountLimit)
	{

	}

	virtual double InterpolateUnclamped(double start, double end, double time) override
	{
		return start + (start - start) * time;
	}

	virtual double Interpolate(double start, double end, double time) override
	{
		return std::lerp(start, end, time);
	}
};

class FPS
{
protected:
	unsigned int _fps;
	unsigned int _fpsCount;
	std::chrono::steady_clock::time_point last;

public:
	// Constructor
	FPS() : _fps(0), _fpsCount(0)
	{
		last = std::chrono::steady_clock::now();
	}

	// Update
	void Update()
	{
		// increase the counter by one
		_fpsCount++;

		auto now = std::chrono::steady_clock::now();

		// one second elapsed? (= 1000 milliseconds)
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count() > 1000)
		{
			// save the current counter value to m_fps
			_fps = _fpsCount;

			// reset the counter and the interval
			_fpsCount = 0;
			last = now;
		}
	}

	// Get fps
	unsigned int Get() const
	{
		return _fps;
	}
};

struct InputData
{
	double PressTime = 0;
	unsigned int InputSequenceNumber = 0;
	unsigned int EntityId = 0;
};

struct Position
{
	std::chrono::steady_clock::time_point timestamp;
	double position = 0;
};

class Entity
{
public:
	Entity() : Interpolator(150)
	{

	}

	double X = 400;
	double Y = 300;
	double Speed = 100;
	std::deque<Position> PositionBuffer;
	BufferedLinearInterpolatorDouble Interpolator;
	unsigned int EntityId = 0;

public:
	void ApplyInput(InputData input)
	{
		X += input.PressTime * Speed;
	}
};
struct NetState
{
	uint32_t EntityId = 0;
	double Position = 0;
	uint32_t LastProcessedInput = 0;
};


class MovementManager : public Component
{
public:
	virtual void Start() override
	{
		_transform = GameObject->GetComponent<Transform>();
	}

	virtual void Update() override
	{
		if (_entity)
		{
			_transform->Position.x = _entity->X;
		}
	}

	void SetEntity(Entity* entity)
	{
		_entity = entity;
	}
private:
	Transform* _transform = nullptr;
	Entity* _entity = nullptr;
};

class PlayerTrianglePrefab : public GameObject
{
public:
	PlayerTrianglePrefab(Scene* scene) : GameObject(scene, "Triangle")
	{
		Transform* transform = AddComponent<Transform>();
		SpriteRenderer2D* sr2D = AddComponent<SpriteRenderer2D>();
		AddComponent<MovementManager>();

		transform->Position = { 400, 300, 0 };
		transform->Size = { 400, 300, 1 };
	}
};


enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	Movement,
	WorldState,
};

class NetworkClient : public Component
{
public:
	NetworkClient()
	{

	}

	int ticks = 0;
	void Start()
	{
		_client.Connected += [this]() { OnConnect(); };
		_client.MessageReceived += [this](Message<CustomMsgTypes> msg) { OnMessage(msg); };
		_client.Disconnected += [this]() { OnDisconnect(); };

		_client.Connect("127.0.0.1", 60000);
	}

	void Update() override
	{
		_client.UpdateNetwork(-1, false);
		if (!_client.IsConnected())
		{
			return;
		}

		if (ticks == 0)
		{
			PingServer();
		}
		ticks = (ticks + 1) % 60;

		if (_entityId == 0)
		{
			return;
		}

		ProcessInputs();

		if (_entityInterpolation)
		{
			InterpolateEntities();
		}
	}
public:
	void PingServer()
	{
		Message<CustomMsgTypes> msg;
		msg.Header.ID = CustomMsgTypes::ServerPing;

		// Caution with this...
		auto timeNow = std::chrono::steady_clock::now();

		msg.Write(timeNow);
		_client.Send(msg);
	}
protected:
	void OnConnect()
	{
		std::cout << "Connected to the server\n";
	}

	// Called when a client appears to have disconnected
	void OnDisconnect()
	{
		std::cout << "Disconnected from the server\n";
	}

	double Ping = 0;
	// Called when a message arrives
	void OnMessage(Message<CustomMsgTypes>& msg)
	{
		switch (msg.Header.ID)
		{
		case CustomMsgTypes::ServerAccept:
		{
			_entityId = msg.Read<unsigned int>();
			std::cout << "Server Accepted ClientSession - Client ID: " << _entityId << "\n";
			break;
		}
		case CustomMsgTypes::ServerPing:
		{
			// Server has responded to a ping request
			std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
			std::chrono::steady_clock::time_point timeThen;
			timeThen = msg.Read<std::chrono::steady_clock::time_point>();
			Ping = std::chrono::duration<double>(timeNow - timeThen).count();
			std::cout << "Ping: " << Ping << "\n";
			break;
		}
		case CustomMsgTypes::WorldState:
		{
			std::vector<NetState> states = msg.ReadVector<NetState>();
			for (auto& state : states)
			{
				if (!_entities.count(state.EntityId))
				{
					Entity* entity = new Entity();
					entity->EntityId = state.EntityId;
					_entities[state.EntityId] = entity;

					PlayerTrianglePrefab* player = GameObject->GetScene()->Instantiate<PlayerTrianglePrefab>();
					player->GetComponent<MovementManager>()->SetEntity(entity);
				}

				Entity* entity = _entities[state.EntityId];
				if (state.EntityId == _entityId)
				{
					entity->X = state.Position;

					if (_serverReconciliation)
					{
						int j = 0;
						while (j < _pendingInputs.size())
						{
							InputData input = _pendingInputs[j];
							if (input.InputSequenceNumber <= state.LastProcessedInput)
							{
								_pendingInputs.erase(_pendingInputs.begin() + j);
							}
							else
							{
								entity->ApplyInput(input);
								++j;
							}
						}
					}
					else
					{
						_pendingInputs.clear();
					}
				}
				else
				{
					if (!_entityInterpolation)
					{
						entity->X = state.Position;
					}
					else
					{
						//auto now = prev + std::chrono::milliseconds(20);
						//prev = now;
						auto now = std::chrono::steady_clock::now();
						//auto serverTime = now.time_since_epoch().count() / 1000000000.0 - Ping;
						//entity->Interpolator.AddMeasurement(state.Position, serverTime);
						entity->PositionBuffer.push_back({ now, state.Position });
					}
				}
			}
			break;
		}
		}
	}

	std::chrono::steady_clock::time_point prev = std::chrono::steady_clock::now();

	float currentVelocity = 0;
	void ProcessInputs()
	{
		auto nowTs = std::chrono::steady_clock::now();
		if (_isFirstTs)
		{
			_lastTs = nowTs;
			_isFirstTs = false;
		}

		auto dtSec = std::chrono::duration_cast<std::chrono::milliseconds>(nowTs - _lastTs).count() / 1000.0;
		_lastTs = nowTs;

		InputData input;
		if (Input::GetKey('A'))
		{
			input.PressTime = -dtSec;
		}
		else if (Input::GetKey('D'))
		{
			input.PressTime = dtSec;
		}
		else
		{
			return;
		}

		input.InputSequenceNumber = _inputSequenceNumber++;
		input.EntityId = _entityId;


		// Send to server InputData
		Message<CustomMsgTypes> msg;
		msg.Header.ID = CustomMsgTypes::Movement;

		msg.Write(input);
		_client.Send(msg);

		if (_clientSidePrediction)
		{
			_entities[_entityId]->ApplyInput(input);
		}

		_pendingInputs.push_back(input);
	}

	float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
	{
		// Based on Game Programming Gems 4 Chapter 1.10
		smoothTime = std::max(0.0001F, smoothTime);
		float omega = 2.0 / smoothTime;

		float x = omega * deltaTime;
		float exp = 1.0 / (1.0 + x + 0.48f * x * x + 0.235f * x * x * x);
		float change = current - target;
		float originalTo = target;

		// Clamp maximum speed
		float maxChange = maxSpeed * smoothTime;
		change = std::clamp(change, -maxChange, maxChange);
		target = current - change;

		float temp = (currentVelocity + omega * change) * deltaTime;
		currentVelocity = (currentVelocity - omega * temp) * exp;
		float output = target + (change + temp) * exp;

		// Prevent overshooting
		if (originalTo - current > 0.0F == output > originalTo)
		{
			output = originalTo;
			currentVelocity = (output - originalTo) / deltaTime;
		}

		return output;
	}

	/*
	void InterpolateEntities()
	{
		auto now = std::chrono::steady_clock::now();
		auto renderTimestamp = (now - std::chrono::milliseconds(1000 / 60)).time_since_epoch().count() / 1000000000.0;
		auto serverTime = now.time_since_epoch().count() / 1000000000.0 - Ping;
		for (auto& it : _entities)
		{
			Entity* entity = it.second;
			if (entity->EntityId == _entityId)
			{
				continue;
			}

			BufferedLinearInterpolatorDouble& buffer = entity->Interpolator;
			buffer.Update(0.01666667, renderTimestamp, serverTime);
			entity->X = buffer.GetInterpolatedValue();
		}
	}
	*/

	void InterpolateEntities()
	{
		auto now = std::chrono::steady_clock::now();
		auto renderTimestamp = now - std::chrono::milliseconds(200);
		for (auto& it : _entities)
		{
			Entity* entity = it.second;
			if (entity->EntityId == _entityId)
			{
				continue;
			}

			std::deque<Position>& buffer = entity->PositionBuffer;

			while (buffer.size() >= 2 && buffer[1].timestamp <= renderTimestamp)
			{
				buffer.pop_front();
			}

			if (buffer.size() >= 2 && buffer[0].timestamp <= renderTimestamp && renderTimestamp <= buffer[1].timestamp)
			{
				auto x0 = buffer[0].position;
				auto x1 = buffer[1].position;
				auto t0 = buffer[0].timestamp;
				auto t1 = buffer[1].timestamp;

				entity->X = x0 + ((x1 - x0) * (renderTimestamp - t0)) / (t1 - t0);
			}
		}
	}


	/*
	void InterpolateEntities()
	{
		auto now = std::chrono::steady_clock::now();
		auto renderTimestamp = now - std::chrono::nanoseconds(1000000000LL / 50LL * 3LL);

		for (auto& it : _entities)
		{
			Entity* entity = it.second;
			if (entity->EntityId == _entityId)
			{
				continue;
			}

			std::deque<Position>& buffer = entity->PositionBuffer;


			while (buffer.size() >= 3 && buffer[1].timestamp <= renderTimestamp)
			{
				buffer.pop_front();
			}

			if (buffer.size() >= 2 && buffer[0].timestamp <= renderTimestamp && renderTimestamp <= buffer[1].timestamp)
			{
				auto x0 = buffer[0].position;
				auto x1 = buffer[1].position;
				auto t0 = buffer[0].timestamp;
				auto t1 = buffer[1].timestamp;

				entity->X = x0 + ((x1 - x0) * (renderTimestamp - t0)) / (t1 - t0);

				buffer.pop_front();
			}
		}
	}
	*/


	int Sign(float a)
	{
		if (a > 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	float MoveTowards(float current, float target, float maxDelta)
	{
		if (std::abs(target - current) <= maxDelta)
		{
			return target;
		}
		return current + Sign(target - current) * maxDelta;
	}


	/*
	void InterpolateEntities()
	{
		auto now = std::chrono::steady_clock::now();
		auto renderTimestamp = now - std::chrono::nanoseconds(100000000);

		for (auto& it : _entities)
		{
			Entity* entity = it.second;
			if (entity->EntityId == _entityId)
			{
				continue;
			}

			std::deque<Position>& buffer = entity->PositionBuffer;

			while (buffer.size() >= 2 && buffer[1].timestamp <= renderTimestamp)
			{
				buffer.pop_front();
			}

			if (buffer.size() >= 2 && buffer[0].timestamp <= renderTimestamp && renderTimestamp <= buffer[1].timestamp)
			{
				entity->X = MoveTowards(entity->X, buffer[1].position, 1000 * 0.016667);
			}
		}
	}
	*/

	/*
	void InterpolateEntities()
	{
		auto now = std::chrono::steady_clock::now();
		for (auto& it : _entities)
		{
			Entity* entity = it.second;
			if (entity->EntityId == _entityId)
			{
				continue;
			}

			std::deque<Position>& buffer = entity->PositionBuffer;
			if (buffer.size() > 0)
			{
				// TODO: remove magic numbers
				double time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - buffer[0].timestamp).count() / 1000000000.0;
				entity->X = SmoothDamp(entity->X, buffer[0].position, currentVelocity, time, 1000000, 0.0166667);

				if (std::abs(buffer[0].position - entity->X) < 0.1f)
				{
					buffer.pop_front();
				}
			}

		}
	}
	*/


	/*
	void InterpolateEntities()
	{
		for (auto& it : _entities)
		{
			Entity* entity = it.second;
			if (entity->EntityId == _entityId)
			{
				continue;
			}

			std::deque<Position>& buffer = entity->PositionBuffer;
			while (buffer.size() > 1)
			{
				buffer.pop_front();
			}

			if (!buffer.empty())
			{
				// TODO: remove magic numbers
				entity->X = MoveTowards(entity->X, buffer[0].position, 0.016 * 100);
			}
		}
	}
	*/

private:
	TCPClient<CustomMsgTypes> _client;

	std::shared_ptr<ClientSession<CustomMsgTypes>> _server = nullptr;

	std::unordered_map<int, Entity*> _entities;
	unsigned int _entityId = 0;

	bool _clientSidePrediction = true;
	bool _serverReconciliation = true;
	unsigned int _inputSequenceNumber = 0;
	std::vector<InputData> _pendingInputs;

	bool _entityInterpolation = true;

	bool _isFirstTs = true;
	std::chrono::steady_clock::time_point _lastTs;
};

class NetworkServer : public Component
{
public:
	NetworkServer(uint16_t port) : _tcpServer(port)
	{

	}

	virtual void Start() override
	{
		_tcpServer.ClientConnected += [this](std::shared_ptr<ClientSession<CustomMsgTypes>> client) { OnClientConnect(client); };
		_tcpServer.ClientApproved += [this](std::shared_ptr<ClientSession<CustomMsgTypes>> client) { OnClientApprove(client); };
		_tcpServer.MessageReceived += [this](Message<CustomMsgTypes> msg) { OnMessage(msg); };
		_tcpServer.ClientDisconnected += [this](std::shared_ptr<ClientSession<CustomMsgTypes>> client) { OnClientDisconnect(client); };

		_tcpServer.StartServer();
	}
	virtual void Update() override
	{
		_tcpServer.UpdateNetwork(-1, false);
		SendWorldState();
	}
protected:
	void OnClientConnect(std::shared_ptr<ClientSession<CustomMsgTypes>> client)
	{

	}

	void OnClientApprove(std::shared_ptr<ClientSession<CustomMsgTypes>> client)
	{
		Message<CustomMsgTypes> msg;
		msg.Header.ID = CustomMsgTypes::ServerAccept;
		msg.Write(client->GetID());

		Entity e;
		e.EntityId = client->GetID();
		_entities[client->GetID()] = e;

		client->Send(msg);
	}

	// Called when a client appears to have disconnected
	void OnClientDisconnect(std::shared_ptr<ClientSession<CustomMsgTypes>> client)
	{
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}

	// Called when a message arrives
	void OnMessage(Message<CustomMsgTypes>& msg)
	{
		switch (msg.Header.ID)
		{
		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << msg.Remote->GetID() << "]: Server Ping\n";

			// Simply bounce message back to client
			msg.Remote->Send(msg);
			break;
		}
		case CustomMsgTypes::Movement:
		{
			InputData input = msg.Read<InputData>();
			unsigned int id = input.EntityId;
			if (!_entities.count(id))
			{
				break;
			}

			if (ValidateInput(input))
			{
				_entities[id].ApplyInput(input);
				_lastProcessedInput[id] = input.InputSequenceNumber;
			}
			break;
		}
		}
	}

	bool ValidateInput(InputData input)
	{
		if (abs(input.PressTime) > 1.0 / 40.0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	void SendWorldState()
	{
		Message<CustomMsgTypes> msg;
		msg.Header.ID = CustomMsgTypes::WorldState;
		std::vector<NetState> states;
		for (auto& it : _entities)
		{
			NetState state;
			state.EntityId = it.second.EntityId;
			state.Position = it.second.X;
			state.LastProcessedInput = _lastProcessedInput[it.first];
			states.push_back(state);
		}

		msg.WriteVector(states);
		_tcpServer.MessageAllClients(msg);
	}
private:
	TCPServer<CustomMsgTypes> _tcpServer;
	std::unordered_map<int, Entity> _entities;
	std::unordered_map<int, unsigned int> _lastProcessedInput;
};




class NetworkClientPrefab : public GameObject
{
public:
	NetworkClientPrefab(Scene* scene) : GameObject(scene, "NetworkClient")
	{
		AddComponent<NetworkClient>();
	}
};

class NetworkServerPrefab : public GameObject
{
public:
	NetworkServerPrefab(Scene* scene) : GameObject(scene, "NetworkServer")
	{
		AddComponent<NetworkServer>(60000);
	}
};



class MainScene : public Scene
{
public:
	MainScene(std::shared_ptr<Window> window, SpriteRenderer* renderer) : Scene(window, renderer)
	{
		if (isServer)
		{
			networkServer = Instantiate<NetworkServerPrefab>();
		}
		else
		{
			networkClient = Instantiate<NetworkClientPrefab>();
		}

		//Instantiate<PlayerTrianglePrefab>();
	}
public:
	GameObject* networkServer = nullptr;
	GameObject* networkClient = nullptr;

};


void TesterInitializer()
{
	auto& app = Aubengine::Application::GetInstance();


	auto window1 = app.CreateWindowOpenGL();
	window1->Initialize("First", 800, 600);
	window1->SetVSync(true);


	window1->Use();
	SpriteRenderer renderer;
	std::shared_ptr<MainScene> mainScene1 = std::make_shared<MainScene>(window1, &renderer);
	window1->SetScene(mainScene1);


	/*
	auto window2 = app.CreateWindow(RenderAPI::OPEN_GL);
	window2->Initialize("Second", 800, 600);

	window2->Use();
	std::shared_ptr<MainScene> mainScene2 = std::make_shared<MainScene>();
	mainScene2->Initialize((GladGLContext*)window2->GetContext());
	window2->SetScene(mainScene2);
	*/
}

int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "true") == 0)
		{
			isServer = true;
		}
		else
		{
			isServer = false;
		}
	}

	TesterInitializer();

	if (isServer)
	{
		Aubengine::Application::GetInstance().SetUpdateRate(50).Run();
	}
	else
	{
		Aubengine::Application::GetInstance().SetUpdateRate(60).Run();
	}

	return 0;
}


