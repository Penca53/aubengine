#include "Shader.h"

#include <iostream>

Shader::Shader(GladGLContext* context)
{
	_context = context;
}

void Shader::Use()
{
	_context->UseProgram(this->ID);
}

void Shader::Compile(const char* vertexSource, const char* fragmentSource)
{
	unsigned int sVertex, sFragment;
	// vertex Shader
	sVertex = _context->CreateShader(GL_VERTEX_SHADER);
	_context->ShaderSource(sVertex, 1, &vertexSource, NULL);
	_context->CompileShader(sVertex);
	CheckCompileErrors(sVertex, "VERTEX");
	// fragment Shader
	sFragment = _context->CreateShader(GL_FRAGMENT_SHADER);
	_context->ShaderSource(sFragment, 1, &fragmentSource, NULL);
	_context->CompileShader(sFragment);
	CheckCompileErrors(sFragment, "FRAGMENT");

	// shader program
	this->ID = _context->CreateProgram();
	_context->AttachShader(this->ID, sVertex);
	_context->AttachShader(this->ID, sFragment);
	_context->LinkProgram(this->ID);
	CheckCompileErrors(this->ID, "PROGRAM");

	// delete the shaders as they're linked into our program now and no longer necessary
	_context->DeleteShader(sVertex);
	_context->DeleteShader(sFragment);
}

void Shader::SetFloat(const char* name, float value, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform1f(_context->GetUniformLocation(this->ID, name), value);
}
void Shader::SetInteger(const char* name, int value, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform1i(_context->GetUniformLocation(this->ID, name), value);
}
void Shader::SetVector2f(const char* name, float x, float y, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform2f(_context->GetUniformLocation(this->ID, name), x, y);
}
void Shader::SetVector2f(const char* name, const glm::vec2& value, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform2f(_context->GetUniformLocation(this->ID, name), value.x, value.y);
}
void Shader::SetVector3f(const char* name, float x, float y, float z, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform3f(_context->GetUniformLocation(this->ID, name), x, y, z);
}
void Shader::SetVector3f(const char* name, const glm::vec3& value, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform3f(_context->GetUniformLocation(this->ID, name), value.x, value.y, value.z);
}
void Shader::SetVector4f(const char* name, float x, float y, float z, float w, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform4f(_context->GetUniformLocation(this->ID, name), x, y, z, w);
}
void Shader::SetVector4f(const char* name, const glm::vec4& value, bool useShader)
{
	if (useShader)
		this->Use();

	_context->Uniform4f(_context->GetUniformLocation(this->ID, name), value.x, value.y, value.z, value.w);
}
void Shader::SetMatrix4(const char* name, const glm::mat4& matrix, bool useShader)
{
	if (useShader)
		this->Use();

	_context->UniformMatrix4fv(_context->GetUniformLocation(this->ID, name), 1, false, glm::value_ptr(matrix));
}


void Shader::CheckCompileErrors(unsigned int object, std::string type)
{
	int success;
	char infoLog[1024];
	if (type != "PROGRAM")
	{
		_context->GetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			_context->GetShaderInfoLog(object, 1024, NULL, infoLog);
			std::cout << "| ERROR::SHADER: Compile-time error: Type: " << type << "\n"
				<< infoLog << "\n -- --------------------------------------------------- -- "
				<< std::endl;
		}
	}
	else
	{
		_context->GetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success)
		{
			_context->GetProgramInfoLog(object, 1024, NULL, infoLog);
			std::cout << "| ERROR::Shader: Link-time error: Type: " << type << "\n"
				<< infoLog << "\n -- --------------------------------------------------- -- "
				<< std::endl;
		}
	}
}