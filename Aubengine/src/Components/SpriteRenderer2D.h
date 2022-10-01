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
	virtual void Start() override
	{
		_context = (GladGLContext*)GameObject->GetScene()->GetWindow()->GetContext();
		_shader = ResourceManager::LoadShader("../../OpenGL/src/shaders/default.vs.glsl", "../../OpenGL/src/shaders/default.fs.glsl", "sprite", _context);

		float vertices[] =
		{
			// positions        // colors
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom right
			-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
			 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f  // top
		};
		unsigned int indices[] =
		{
			0, 1, 2,
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
		_context->VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		_context->EnableVertexAttribArray(0);
		// Color
		_context->VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		_context->EnableVertexAttribArray(1);

		_context->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		_context->BindBuffer(GL_ARRAY_BUFFER, 0);
		_context->BindVertexArray(0);
	}
public:
	std::shared_ptr<Shader> _shader = nullptr;
	GladGLContext* _context = nullptr;
	unsigned int _quadVAO = 0;
	unsigned int _quadEBO = 0;
	unsigned int _quadVBO = 0;
};