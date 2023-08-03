#pragma once

#include <memory>
#include <string>

class Scene;

class Window {
 public:
  virtual ~Window() = default;

  virtual bool Initialize(const std::string& name, uint32_t width,
                          uint32_t height) = 0;
  virtual void Close() = 0;
  virtual bool WindowShouldClose() = 0;
  virtual void* GetWindowNative() = 0;
  virtual void* GetContext() = 0;
  virtual void Use() = 0;
  virtual void Begin() = 0;
  virtual void End() = 0;
  virtual void PhysicsUpdate() = 0;
  virtual void Update() = 0;
  virtual void Render() = 0;
  virtual bool GetVSync() = 0;
  virtual void SetVSync(bool isEnabled) = 0;

  virtual void SetScene(std::shared_ptr<Scene> scene) = 0;

 protected:
  bool v_sync_ = false;
  std::shared_ptr<Scene> scene_ = nullptr;
};