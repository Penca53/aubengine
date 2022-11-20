#include "SpriteRenderer.h"
#include "Components/SpriteRenderer2D.h"
#include "Components/Transform.h"

void SpriteRenderer::DrawSprite(GameObject* go)
{
	Transform* transform = go->GetComponent<Transform>();
	SpriteRenderer2D* sprite = go->GetComponent<SpriteRenderer2D>();

	if (!transform || !sprite)
	{
		return;
	}

	// prepare transformations
	sprite->_shader->Use();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, transform->Position);  // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)

	model = glm::translate(model, glm::vec3(0.5f * transform->Size.x, 0.5f * transform->Size.y, 0.0f)); // move origin of rotation to center of quad
	model = glm::rotate(model, glm::radians(transform->EulerRotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // then rotate
	model = glm::translate(model, glm::vec3(-0.5f * transform->Size.x, -0.5f * transform->Size.y, 0.0f)); // move origin back

	model = glm::scale(model, transform->Size); // last scale
	auto projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

	sprite->_shader->SetInteger("image", 0);
	sprite->_shader->SetMatrix4("model", model);
	sprite->_shader->SetMatrix4("projection", projection);
	sprite->_shader->SetVector3f("spriteColor", sprite->Color);

	// render textured quad

	sprite->_context->ActiveTexture(GL_TEXTURE0);
	sprite->_texture2D->Bind();

	sprite->_context->BindVertexArray(sprite->_quadVAO);
	sprite->_context->DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	sprite->_context->BindVertexArray(0);
}