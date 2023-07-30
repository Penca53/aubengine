#pragma once

#include <glm/glm.hpp>

#include "aubengine/components/component.h"

class Transform : public Component {
 public:
  glm::vec3 position{};
  glm::vec3 size{};
  glm::vec3 euler_rotation{};

 public:
  virtual void Update() override;
};