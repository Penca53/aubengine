#pragma once

class GameObject;

class Component
{
public:
	void SetOwner(GameObject* go) { GameObject = go; }
	virtual void Start() {}
	virtual void Update() {}
public:
	bool IsEnabled = true;
	GameObject* GameObject = nullptr;
};