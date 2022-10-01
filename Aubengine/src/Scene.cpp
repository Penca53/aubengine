#include "Scene.h"
#include "GameObject.h"
#include "SpriteRenderer.h"

Scene::Scene(std::shared_ptr<Window> window, SpriteRenderer* renderer)
{
	_window = window;
	_renderer = renderer;
}

void Scene::Update()
{
	for (auto& go : _gameObjects)
	{
		go->Update();
	}
}

void Scene::Render()
{
	for (auto& go : _gameObjects)
	{
		_renderer->DrawSprite(go);
	}
}

void Scene::Destroy(GameObject* gameObject)
{
	if (_gameObjects.count(gameObject))
	{
		_gameObjects.erase(gameObject);
	}
}

std::shared_ptr<Window> Scene::GetWindow()
{
	return _window;
}