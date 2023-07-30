#include "aubengine/window_opengl.h"

#include <iostream>

static std::unordered_map<GLFWwindow*, GladGLContext*> _windowToContext;
uint8_t WindowOpenGL::window_opengl_instances_count_ = 0;

WindowOpenGL::~WindowOpenGL() { Close(); }

bool WindowOpenGL::Initialize(std::string title, uint32_t width,
                              uint32_t height) {
  if (window_opengl_instances_count_ == 0) {
    glfwInit();
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window_ = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
  if (window_ == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    return false;
  }

  glfwMakeContextCurrent(window_);
  context_ = new GladGLContext();
  if (!context_) {
    std::cout << "Failed to create OpenGL context" << std::endl;
    return false;
  }

  int version = gladLoadGLContext(context_, glfwGetProcAddress);
  if (version == 0) {
    std::cout << "Failed to initialize OpenGL context" << std::endl;
    return false;
  }

  _windowToContext[window_] = context_;

  ++window_opengl_instances_count_;
  glfwSetFramebufferSizeCallback(window_,
                                 [](GLFWwindow* window, int width, int height) {
                                   glfwMakeContextCurrent(window);
                                   auto ctx = _windowToContext[window];
                                   ctx->Viewport(0, 0, width, height);
                                 });
  context_->Viewport(0, 0, width, height);

  return true;
}
void WindowOpenGL::Close() {
  glfwDestroyWindow(window_);
  --window_opengl_instances_count_;

  if (window_opengl_instances_count_ == 0) {
    glfwTerminate();
  }
}
bool WindowOpenGL::WindowShouldClose() {
  return glfwWindowShouldClose(window_);
}
void* WindowOpenGL::GetWindowNative() { return window_; }
void* WindowOpenGL::GetContext() { return context_; }

void WindowOpenGL::Begin() { glfwPollEvents(); }

void WindowOpenGL::End() { glfwSwapBuffers(window_); }

void WindowOpenGL::Update() {
  if (!scene_) {
    return;
  }

  Use();
  scene_->Update();
}
void WindowOpenGL::Render() {
  Use();

  context_->ClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  context_->Clear(GL_COLOR_BUFFER_BIT);
  context_->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  context_->Enable(GL_BLEND);

  if (!scene_) {
    return;
  }

  scene_->Render();
}

void WindowOpenGL::Use() { glfwMakeContextCurrent(window_); }
void WindowOpenGL::SetVSync(bool isEnabled) {
  v_sync_ = isEnabled;
  glfwSwapInterval(isEnabled);
}
bool WindowOpenGL::GetVSync() { return v_sync_; }

void WindowOpenGL::SetScene(std::shared_ptr<Scene> scene) { scene_ = scene; }