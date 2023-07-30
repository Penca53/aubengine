#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

// General purpsoe shader object. Compiles from file, generates
// compile/link-time error messages and hosts several utility
// functions for easy management.
class Shader {
 public:
  // state
  uint32_t id = 0;
  // constructor
  Shader(GladGLContext* context);
  // sets the current shader as active
  void Use();
  // compiles the shader from given source code
  void Compile(
      const char* vertexSource,
      const char* fragmentSource);  // note: geometry source code is optional
  // utility functions
  void SetFloat(const char* name, float value, bool useShader = false);
  void SetInteger(const char* name, int value, bool useShader = false);
  void SetVector2f(const char* name, float x, float y, bool useShader = false);
  void SetVector2f(const char* name, const glm::vec2& value,
                   bool useShader = false);
  void SetVector3f(const char* name, float x, float y, float z,
                   bool useShader = false);
  void SetVector3f(const char* name, const glm::vec3& value,
                   bool useShader = false);
  void SetVector4f(const char* name, float x, float y, float z, float w,
                   bool useShader = false);
  void SetVector4f(const char* name, const glm::vec4& value,
                   bool useShader = false);
  void SetMatrix4(const char* name, const glm::mat4& matrix,
                  bool useShader = false);

 private:
  // checks if compilation or linking failed and if so, print the error logs
  void CheckCompileErrors(unsigned int object, std::string type);
  GladGLContext* context_ = nullptr;
};
