#pragma once

class Scene;
class GameObject;

class Prefab
{
public:
	virtual GameObject* Build(Scene* scene) = 0;
};