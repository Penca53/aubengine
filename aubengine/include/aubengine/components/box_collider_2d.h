#pragma once

#include <glm/glm.hpp>

#include "aubengine/application.h"
#include "aubengine/components/component.h"

class BoxCollider2D : public Component {
 public:
  glm::vec3 position{};
  glm::vec3 size{};
  glm::vec3 euler_rotation{};

 public:
  virtual void Start() override {}
  virtual void Update() override {}

 private:
};