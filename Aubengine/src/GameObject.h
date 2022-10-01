#pragma once

#include <string>
#include <unordered_set>

#include "Utils/Derived.h"
#include "Scene.h"
#include "Components/Component.h"

class Component;

class GameObject
{
public:
	GameObject(Scene* scene, std::string name);

	void Update();

	template<Derived<Component> T, typename... Args>
	T* AddComponent(Args... args)
	{
		T* component = new T(args...);
		component->SetOwner(this);
		_components.insert(component);
		component->Start();
		return component;
	}
	template <Derived<Component> T>
	T* GetComponent()
	{
		for (auto c : _components)
		{
			T* a = dynamic_cast<T*>(c);
			if (a != nullptr)
			{
				return (T*)c;
			}
		}
		return nullptr;
	}
	void RemoveComponent(Component* component);
	Scene* GetScene();
public:
	std::string Name;
private:
	std::unordered_set<Component*> _components;
	Scene* _scene = nullptr;
};