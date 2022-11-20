#pragma once

#include "Component.h"
#include "../Application.h"
#include <glm/glm.hpp>

class BoxCollider2D : public Component
{
public:
	glm::vec3 Position{};
	glm::vec3 Size{};
	glm::vec3 EulerRotation{};
public:
	virtual void Start() override
	{

	}
	virtual void Update() override
	{

	}
private:
};