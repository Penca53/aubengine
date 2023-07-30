#include "aubengine/shader.h"

#include <iostream>

Shader::Shader(GladGLContext* context): context_(context) { }

void Shader::Use() { context_->UseProgram(this->id); }

void Shader::Compile(const char* vertexSource, const char* fragmentSource) {
  unsigned int sVertex, sFragment;
  // vertex Shader
  sVertex = context_->CreateShader(GL_VERTEX_SHADER);
  context_->ShaderSource(sVertex, 1, &vertexSource, NULL);
  context_->CompileShader(sVertex);
  CheckCompileErrors(sVertex, "VERTEX");
  // fragment Shader
  sFragment = context_->CreateShader(GL_FRAGMENT_SHADER);
  context_->ShaderSource(sFragment, 1, &fragmentSource, NULL);
  context_->CompileShader(sFragment);
  CheckCompileErrors(sFragment, "FRAGMENT");

  // shader program
  this->id = context_->CreateProgram();
  context_->AttachShader(this->id, sVertex);
  context_->AttachShader(this->id, sFragment);
  context_->LinkProgram(this->id);
  CheckCompileErrors(this->id, "PROGRAM");

  // delete the shaders as they're linked into our program now and no longer
  // necessary
  context_->DeleteShader(sVertex);
  context_->DeleteShader(sFragment);
}

void Shader::SetFloat(const char* name, float value, bool useShader) {
  if (useShader) this->Use();

  context_->Uniform1f(context_->GetUniformLocation(this->id, name), value);
}
void Shader::SetInteger(const char* name, int value, bool useShader) {
  if (useShader) this->Use();

  context_->Uniform1i(context_->GetUniformLocation(this->id, name), value);
}
void Shader::SetVector2f(const char* name, float x, float y, bool useShader) {
  if (useShader) this->Use();

  context_->Uniform2f(context_->GetUniformLocation(this->id, name), x, y);
}
void Shader::SetVector2f(const char* name, const glm::vec2& value,
                         bool useShader) {
  if (useShader) this->Use();

  context_->Uniform2f(context_->GetUniformLocation(this->id, name), value.x,
                      value.y);
}
void Shader::SetVector3f(const char* name, float x, float y, float z,
                         bool useShader) {
  if (useShader) this->Use();

  context_->Uniform3f(context_->GetUniformLocation(this->id, name), x, y, z);
}
void Shader::SetVector3f(const char* name, const glm::vec3& value,
                         bool useShader) {
  if (useShader) this->Use();

  context_->Uniform3f(context_->GetUniformLocation(this->id, name), value.x,
                      value.y, value.z);
}
void Shader::SetVector4f(const char* name, float x, float y, float z, float w,
                         bool useShader) {
  if (useShader) this->Use();

  context_->Uniform4f(context_->GetUniformLocation(this->id, name), x, y, z, w);
}
void Shader::SetVector4f(const char* name, const glm::vec4& value,
                         bool useShader) {
  if (useShader) this->Use();

  context_->Uniform4f(context_->GetUniformLocation(this->id, name), value.x,
                      value.y, value.z, value.w);
}
void Shader::SetMatrix4(const char* name, const glm::mat4& matrix,
                        bool useShader) {
  if (useShader) this->Use();

  context_->UniformMatrix4fv(context_->GetUniformLocation(this->id, name), 1,
                             false, glm::value_ptr(matrix));
}

void Shader::CheckCompileErrors(unsigned int object, std::string type) {
  int success;
  char infoLog[1024];
  if (type != "PROGRAM") {
    context_->GetShaderiv(object, GL_COMPILE_STATUS, &success);
    if (!success) {
      context_->GetShaderInfoLog(object, 1024, NULL, infoLog);
      std::cout
          << "| ERROR::SHADER: Compile-time error: Type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  } else {
    context_->GetProgramiv(object, GL_LINK_STATUS, &success);
    if (!success) {
      context_->GetProgramInfoLog(object, 1024, NULL, infoLog);
      std::cout
          << "| ERROR::Shader: Link-time error: Type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  }
}