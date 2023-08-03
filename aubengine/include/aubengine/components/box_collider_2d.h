#pragma once

#include <glm/glm.hpp>

#include "aubengine/application.h"
#include "aubengine/components/component.h"
#include "box2d/box2d.h"

class BoxCollider2D : public Component {
 public:
  BoxCollider2D();
  BoxCollider2D(glm::vec3 size);

 public:
  glm::vec3 size{};

 public:
  virtual void Start() override;
  virtual void Update() override;
};