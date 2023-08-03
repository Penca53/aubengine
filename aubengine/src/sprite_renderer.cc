#include "aubengine/sprite_renderer.h"

#include "aubengine/components/sprite_renderer_2d.h"
#include "aubengine/components/transform.h"

void SpriteRenderer::DrawSprite(GameObject* go) {
  SpriteRenderer2D* sprite = go->GetComponent<SpriteRenderer2D>();

  if (!go->transform || !sprite) {
    return;
  }

  // prepare transformations
  sprite->shader_->Use();
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(
      model,
      go->transform->position);  // first translate (transformations are: scale
                                 // happens first, then rotation, and then final
                                 // translation happens; reversed order)

  model = glm::translate(
      model,
      glm::vec3(0.5f * go->transform->size.x, 0.5f * go->transform->size.y,
                0.0f));  // move origin of rotation to center of quad
  model = glm::rotate(model, glm::radians(go->transform->euler_rotation.z),
                      glm::vec3(0.0f, 0.0f, 1.0f));  // then rotate
  model = glm::translate(model, glm::vec3(-0.5f * go->transform->size.x,
                                          -0.5f * go->transform->size.y,
                                          0.0f));  // move origin back

  model = glm::scale(model, go->transform->size);  // last scale
  auto projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

  sprite->shader_->SetInteger("image", 0);
  sprite->shader_->SetMatrix4("model", model);
  sprite->shader_->SetMatrix4("projection", projection);
  sprite->shader_->SetVector3f("spriteColor", sprite->color_);

  // render textured quad

  sprite->context_->ActiveTexture(GL_TEXTURE0);
  sprite->texture_2d_->Bind();

  sprite->context_->BindVertexArray(sprite->quad_vao_);
  sprite->context_->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  sprite->context_->BindVertexArray(0);
}