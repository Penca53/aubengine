#include "GameObject.h"

GameObject::GameObject(Scene* scene, std::string name)
{
	_scene = scene;
	Name = name;
}

void GameObject::Update()
{
	for (auto it : _components)
	{
		if (it->IsEnabled)
		{
			it->Update();
		}
	}
}
void GameObject::RemoveComponent(Component* component)
{
	if (_components.count(component))
	{
		_components.erase(component);
	}
}

Scene* GameObject::GetScene()
{
	return _scene;
}