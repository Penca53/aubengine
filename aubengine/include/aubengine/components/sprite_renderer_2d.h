#pragma once

#include <glad/gl.h>

#include <memory>

#include "aubengine/components/component.h"
#include "aubengine/resource_manager.h"
#include "aubengine/shader.h"
#include "aubengine/window.h"

class SpriteRenderer2D : public Component {
 public:
  SpriteRenderer2D(std::shared_ptr<Shader> shader,
                   std::shared_ptr<Texture2D> texture2D)
      : shader_(shader), texture_2d_(texture2D) {}

  virtual void Start() override;

 public:
  std::shared_ptr<Shader> shader_ = nullptr;
  std::shared_ptr<Texture2D> texture_2d_ = nullptr;
  glm::vec3 color_ = {1, 1, 1};
  GladGLContext* context_ = nullptr;
  unsigned int quad_vao_ = 0;
  unsigned int quad_ebo_ = 0;
  unsigned int quad_vbo_ = 0;
};