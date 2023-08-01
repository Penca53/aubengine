#include "aubengine/application.h"

#include <chrono>
#include <iostream>

#include "aubengine/input.h"
#include "aubengine/resource_manager.h"
#include "aubengine/window_opengl.h"

namespace Aubengine {
Application& Application::GetInstance() {
  static Application s;
  return s;
}

Application& Application::SetUpdateRate(uint32_t hz) {
  hz_ = hz;
  return *this;
}

uint64_t Application::MillisecondsPerUpdate() { return 1000LL / hz_; }
uint64_t Application::NanosecondsPerUpdate() { return 1000000000LL / hz_; }

void Application::Run(uint32_t hz) {
  SetUpdateRate(hz);
  Run();
}
void Application::Run() {
  auto previous = std::chrono::steady_clock::now();
  uint64_t lag = 0;
  while (true) {
    bool shouldClose = false;
    for (const auto& window : windows) {
      shouldClose |= window->WindowShouldClose();
    }

    if (shouldClose) {
      break;
    }

    auto current = std::chrono::steady_clock::now();

    uint64_t elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(current - previous)
            .count();

    previous = current;
    lag += elapsed;

    while (lag >= NanosecondsPerUpdate()) {
      PollInput();
      Update();
      lag -= NanosecondsPerUpdate();
    }

    Render();
  }

  for (const auto& window : windows) {
    ResourceManager::Clear((GladGLContext*)window->GetContext());
  }
}

void Application::PollInput() {
  for (const auto& window : windows) {
    window->Begin();
  }
  Input::PollInput();
}

void Application::Update() {
  for (const auto& window : windows) {
    window->Update();
  }
}

void Application::Render() {
  for (const auto& window : windows) {
    window->Render();
    window->End();
  }
}

Window* Application::CreateWindowOpenGL() {
  std::shared_ptr<Window> window = std::make_shared<WindowOpenGL>();
  windows.push_back(window);
  return window.get();
}
}  // namespace Aubengine