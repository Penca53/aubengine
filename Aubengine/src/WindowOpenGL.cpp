#include "WindowOpenGL.h"

#include <iostream>
#include <glad/gl.h>

static std::unordered_map<GLFWwindow*, GladGLContext*> _windowToContext;
uint8_t WindowOpenGL::_windowOpenGLInstancesCount = 0;

WindowOpenGL::~WindowOpenGL()
{
	Close();
}

bool WindowOpenGL::Initialize(std::string title, uint32_t width, uint32_t height)
{
	if (_windowOpenGLInstancesCount == 0)
	{
		glfwInit();
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (_window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		return false;
	}

	glfwMakeContextCurrent(_window);
	_context = new GladGLContext();
	if (!_context)
	{
		std::cout << "Failed to create OpenGL context" << std::endl;
		return false;
	}

	int version = gladLoadGLContext(_context, glfwGetProcAddress);
	if (version == 0)
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return false;
	}

	_windowToContext[_window] = _context;

	++_windowOpenGLInstancesCount;
	glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height)
		{
			glfwMakeContextCurrent(window);
			auto ctx = _windowToContext[window];
			ctx->Viewport(0, 0, width, height);
		});
	_context->Viewport(0, 0, width, height);

	return true;
}
void WindowOpenGL::Close()
{
	glfwDestroyWindow(_window);
	--_windowOpenGLInstancesCount;

	if (_windowOpenGLInstancesCount == 0)
	{
		glfwTerminate();
	}
}
bool WindowOpenGL::WindowShouldClose()
{
	return glfwWindowShouldClose(_window);
}
void* WindowOpenGL::GetWindowNative()
{
	return _window;
}
void* WindowOpenGL::GetContext()
{
	return _context;
}

void WindowOpenGL::Begin()
{
	glfwPollEvents();
}

void WindowOpenGL::End()
{
	glfwSwapBuffers(_window);
}

void WindowOpenGL::Update()
{
	if (!_Scene)
	{
		return;
	}

	Use();
	_Scene->Update();
}
void WindowOpenGL::Render()
{
	Use();

	_context->ClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	_context->Clear(GL_COLOR_BUFFER_BIT);
	_context->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	_context->Enable(GL_BLEND);

	if (!_Scene)
	{
		return;
	}

	_Scene->Render();
}

void WindowOpenGL::Use()
{
	glfwMakeContextCurrent(_window);
	gladSetGLContext(_context);
}
void WindowOpenGL::SetVSync(bool isEnabled)
{
	_VSync = isEnabled;
	glfwSwapInterval(isEnabled);
}
bool WindowOpenGL::GetVSync()
{
	return _VSync;
}

void WindowOpenGL::SetScene(std::shared_ptr<Scene> scene)
{
	_Scene = scene;
}