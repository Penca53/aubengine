#pragma once

// clang-format off
#include <glad/gl.h>
// clang-format on
#include <GLFW/glfw3.h>

#include <unordered_map>

#include "aubengine/window.h"

class WindowOpenGL : public Window {
 public:
  virtual ~WindowOpenGL();

  virtual bool Initialize(const std::string& name, uint32_t width,
                          uint32_t height) override;
  virtual void Close() override;
  virtual bool WindowShouldClose() override;
  virtual void* GetWindowNative() override;
  virtual void* GetContext() override;
  virtual void Begin() override;
  virtual void End() override;
  virtual void Update() override;
  virtual void Render() override;
  virtual void Use() override;
  virtual bool GetVSync() override;
  virtual void SetVSync(bool isEnabled) override;
  virtual void SetScene(std::shared_ptr<Scene> scene) override;

 private:
  GLFWwindow* window_ = nullptr;
  GladGLContext* context_ = nullptr;
  static uint8_t window_opengl_instances_count_;
};