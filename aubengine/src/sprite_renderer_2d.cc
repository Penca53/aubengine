#include "aubengine/components/sprite_renderer_2d.h"

#include "aubengine/game_object.h"
#include "aubengine/scene.h"

void SpriteRenderer2D::Start() {
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