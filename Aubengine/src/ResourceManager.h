#pragma once

#include <map>
#include <string>
#include <memory>

#include "Texture2D.h"
#include "Shader.h"


// A static singleton ResourceManager class that hosts several
// functions to load Textures and Shaders. Each loaded texture
// and/or shader is also stored for future reference by string
// handles. All functions and resources are static and no 
// public constructor is defined.
class ResourceManager
{
public:
	// resource storage
	static std::map<std::string, std::shared_ptr<Shader>> Shaders;
	static std::map<std::string, Texture2D> Textures;
	// loads (and generates) a shader program from file loading vertex, fragment (and geometry) shader's source code. If gShaderFile is not nullptr, it also loads a geometry shader
	static std::shared_ptr<Shader> LoadShader(const char* vShaderFile, const char* fShaderFile, std::string name, GladGLContext* context);
	// retrieves a stored sader
	static std::shared_ptr<Shader> GetShader(std::string name);
	// loads (and generates) a texture from file
	static Texture2D LoadTexture(const char* file, bool alpha, std::string name, GladGLContext* context);
	// retrieves a stored texture
	static Texture2D GetTexture(std::string name);
	// properly de-allocates all loaded resources
	static void Clear(GladGLContext* context);
private:
	// private constructor, that is we do not want any actual resource manager objects. Its members and functions should be publicly available (static).
	ResourceManager() { }
	// loads and generates a shader from file
	static std::shared_ptr<Shader> LoadShaderFromFile(const char* vShaderFile, const char* fShaderFile, GladGLContext* context);
	// loads a single texture from file
	static Texture2D LoadTextureFromFile(const char* file, bool alpha, GladGLContext* context);
};
