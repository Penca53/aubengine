#include "InputGLFW.h"
#include "Application.h"

#include <GLFW/glfw3.h>

Input* Input::_instance = new InputGLFW();

bool InputGLFW::GetKeyDownImpl(int key) const
{
	return (_KeyStates[key] && !_OldKeyStates[key]);
}
bool InputGLFW::GetKeyImpl(int key) const
{
	return _KeyStates[key];
}
bool InputGLFW::GetKeyUpImpl(int key) const
{
	return (!_KeyStates[key] && _OldKeyStates[key]);
}
void InputGLFW::PollInputImpl()
{
	for (int i = 0; i <= GLFW_KEY_LAST; ++i)
	{
		_OldKeyStates[i] = _KeyStates[i];
	}

	GLFWwindow* _window = static_cast<GLFWwindow*>(Aubengine::Application::GetInstance().Windows[0]->GetWindowNative());
	for (int i = 0; i <= GLFW_KEY_LAST; ++i)
	{
		_KeyStates[i] = (glfwGetKey(_window, i));
	}
}