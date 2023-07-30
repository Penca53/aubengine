#include "aubengine/resource_manager.h"

#include <glad/gl.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "stb_image.h"

// Instantiate static variables
std::map<std::string, std::shared_ptr<Texture2D>> ResourceManager::Textures;
std::map<std::string, std::shared_ptr<Shader>> ResourceManager::Shaders;

std::shared_ptr<Shader> ResourceManager::LoadShader(const char* vShaderFile,
                                                    const char* fShaderFile,
                                                    std::string name,
                                                    GladGLContext* context) {
  Shaders[name] = LoadShaderFromFile(vShaderFile, fShaderFile, context);
  return Shaders[name];
}

std::shared_ptr<Shader> ResourceManager::GetShader(std::string name) {
  return Shaders[name];
}

std::shared_ptr<Texture2D> ResourceManager::LoadTexture(
    const char* file, bool alpha, std::string name, GladGLContext* context) {
  Textures[name] = LoadTextureFromFile(file, alpha, context);
  return Textures[name];
}

std::shared_ptr<Texture2D> ResourceManager::GetTexture(std::string name) {
  return Textures[name];
}

void ResourceManager::Clear(GladGLContext* context) {
  // (properly) delete all shaders
  for (auto& iter : Shaders) context->DeleteProgram(iter.second->id);
  // (properly) delete all textures
  for (auto& iter : Textures) context->DeleteTextures(1, &iter.second->ID);
}

std::shared_ptr<Shader> ResourceManager::LoadShaderFromFile(
    const char* vShaderFile, const char* fShaderFile, GladGLContext* context) {
  // 1. retrieve the vertex/fragment source code from filePath
  std::string vertexCode;
  std::string fragmentCode;
  try {
    // open files
    std::ifstream vertexShaderFile(vShaderFile);
    std::ifstream fragmentShaderFile(fShaderFile);
    std::stringstream vShaderStream, fShaderStream;
    // read file's buffer contents into streams
    vShaderStream << vertexShaderFile.rdbuf();
    fShaderStream << fragmentShaderFile.rdbuf();
    // close file handlers
    vertexShaderFile.close();
    fragmentShaderFile.close();
    // convert stream into string
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();
  } catch (std::exception e) {
    std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
  }
  const char* vShaderCode = vertexCode.c_str();
  const char* fShaderCode = fragmentCode.c_str();
  // 2. now create shader object from source code

  std::shared_ptr<Shader> shader = std::make_shared<Shader>(context);
  shader->Compile(vShaderCode, fShaderCode);
  return shader;
}

std::shared_ptr<Texture2D> ResourceManager::LoadTextureFromFile(
    const char* file, bool alpha, GladGLContext* context) {
  // create texture object
  std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(context);
  if (alpha) {
    texture->Internal_Format = GL_RGBA;
    texture->Image_Format = GL_RGBA;
  }
  // load image
  int width, height, nrChannels;
  unsigned char* data = stbi_load(file, &width, &height, &nrChannels, 0);
  // now generate texture
  texture->Generate(width, height, data);
  // and finally free image data
  stbi_image_free(data);
  return texture;
}