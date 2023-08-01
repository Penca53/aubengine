#include "aubengine/input_glfw.h"

#include <GLFW/glfw3.h>

#include "aubengine/application.h"

Input* Input::instance_ = new InputGLFW();

bool InputGLFW::GetKeyDownImpl(int key) const {
  return (key_states_[key] && !old_key_states_[key]);
}
bool InputGLFW::GetKeyImpl(int key) const { return key_states_[key]; }
bool InputGLFW::GetKeyUpImpl(int key) const {
  return (!key_states_[key] && old_key_states_[key]);
}
void InputGLFW::PollInputImpl() {
  for (int i = 0; i <= GLFW_KEY_LAST; ++i) {
    old_key_states_[i] = key_states_[i];
  }

  GLFWwindow* window = static_cast<GLFWwindow*>(
      Aubengine::Application::GetInstance().focused_window->GetWindowNative());
  for (int i = 0; i <= GLFW_KEY_LAST; ++i) {
    key_states_[i] = (glfwGetKey(window, i));
  }
}