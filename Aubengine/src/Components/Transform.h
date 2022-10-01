#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class Transform : public Component
{
public:
	glm::vec3 Position{};
	glm::vec3 Size{};
	glm::vec3 EulerRotation{};
public:
	virtual void Update() override;
};