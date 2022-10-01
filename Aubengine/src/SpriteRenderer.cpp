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

	sprite->_shader->SetMatrix4("model", model);
	sprite->_shader->SetMatrix4("projection", projection);

	// render textured quad

	//_context->ActiveTexture(GL_TEXTURE0);
	//texture.Bind();

	sprite->_context->BindVertexArray(sprite->_quadVAO);
	sprite->_context->DrawArrays(GL_TRIANGLES, 0, 3);
	sprite->_context->BindVertexArray(0);
}