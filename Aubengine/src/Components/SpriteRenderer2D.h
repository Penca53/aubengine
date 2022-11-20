#pragma once

#include <memory>
#include <glad/gl.h>

#include "Component.h"
#include "../Shader.h"
#include "../ResourceManager.h"
#include "../Window.h"

class SpriteRenderer2D : public Component
{
public:
	SpriteRenderer2D(std::shared_ptr<Shader> shader, std::shared_ptr<Texture2D> texture2D)
	{
		_shader = shader;
		_texture2D = texture2D;
	}

	virtual void Start() override
	{
		_context = (GladGLContext*)GameObject->GetScene()->GetWindow()->GetContext();

		float vertices[] = {
			// pos			    // tex
			0.5f, 0.5f,	        1.0f, 1.0f,
			0.5f, -0.5f,		1.0f, 0.0f,
			-0.5f, -0.5f,		0.0f, 0.0f,
			-0.5f,  0.5f,		0.0f, 1.0f,
		};

		unsigned int indices[] =
		{
			0, 1, 3,
			1, 2, 3
		};

		_context->GenVertexArrays(1, &_quadVAO);
		_context->GenBuffers(1, &_quadEBO);
		_context->GenBuffers(1, &_quadVBO);


		_context->BindVertexArray(_quadVAO);

		_context->BindBuffer(GL_ARRAY_BUFFER, _quadVBO);
		_context->BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		_context->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
		_context->BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// Position
		_context->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		_context->EnableVertexAttribArray(0);
		// TexCoord
		_context->VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		_context->EnableVertexAttribArray(1);

		_context->BindVertexArray(0);
	}
public:
	std::shared_ptr<Shader> _shader = nullptr;
	std::shared_ptr<Texture2D> _texture2D = nullptr;
	glm::vec3 Color = { 1, 1, 1 };
	GladGLContext* _context = nullptr;
	unsigned int _quadVAO = 0;
	unsigned int _quadEBO = 0;
	unsigned int _quadVBO = 0;
};