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

  virtual void Start() override {
    context_ = static_cast<GladGLContext*>(
        game_object->GetScene()->GetWindow()->GetContext());

    float vertices[] = {
        // pos			    // tex
        0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, -0.5f, 0.5f,  0.0f, 1.0f,
    };

    unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    context_->GenVertexArrays(1, &quad_vao_);
    context_->GenBuffers(1, &quad_ebo_);
    context_->GenBuffers(1, &quad_vbo_);

    context_->BindVertexArray(quad_vao_);

    context_->BindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    context_->BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);

    context_->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo_);
    context_->BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                         GL_STATIC_DRAW);

    // Position
    context_->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                  (void*)0);
    context_->EnableVertexAttribArray(0);
    // TexCoord
    context_->VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                  (void*)(2 * sizeof(float)));
    context_->EnableVertexAttribArray(1);

    context_->BindVertexArray(0);
  }

 public:
  std::shared_ptr<Shader> shader_ = nullptr;
  std::shared_ptr<Texture2D> texture_2d_ = nullptr;
  glm::vec3 color_ = {1, 1, 1};
  GladGLContext* context_ = nullptr;
  unsigned int quad_vao_ = 0;
  unsigned int quad_ebo_ = 0;
  unsigned int quad_vbo_ = 0;
};