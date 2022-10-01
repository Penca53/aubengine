#pragma once

#include <unordered_set>
#include <memory>
#include "Utils/Derived.h"

class GameObject;
class Window;
class Prefab;
class SpriteRenderer;

class Scene
{
public:
	Scene(std::shared_ptr<Window> window, SpriteRenderer* renderer);
	virtual ~Scene() = default;

	void Update();
	void Render();

	template<Derived<GameObject> T>
	T* Instantiate()
	{
		T* go = new T(this);
		_gameObjects.insert(go);
		return go;
	}
	void Destroy(GameObject* gameObject);
	std::shared_ptr<Window> GetWindow();
private:
	std::shared_ptr<Window> _window = nullptr;
	SpriteRenderer* _renderer = nullptr;

	std::unordered_set<GameObject*> _gameObjects;
};