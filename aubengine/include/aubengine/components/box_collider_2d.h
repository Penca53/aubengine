#pragma once

#include <glm/glm.hpp>

#include "aubengine/application.h"
#include "aubengine/components/component.h"

class BoxCollider2D : public Component {
 public:
  glm::vec3 Position{};
  glm::vec3 Size{};
  glm::vec3 EulerRotation{};

 public:
  virtual void Start() override {}
  virtual void Update() override {}

 private:
};