#pragma once

#include <memory>
#include <vector>

#include "aubengine/render_api.h"
#include "aubengine/window.h"

namespace Aubengine {
class Application {
 public:
  virtual ~Application() = default;

  static Application& GetInstance();

  Application& SetUpdateRate(uint32_t hz);
  uint64_t MillisecondsPerUpdate();
  uint64_t NanosecondsPerUpdate();
  void Run(uint32_t hz);
  void Run();
  std::shared_ptr<Window> CreateWindowOpenGL();

  void PollInput();
  void Update();
  void Render();

 public:
  Application(Application const&) = delete;
  void operator=(Application const&) = delete;

 private:
  Application() = default;

 public:
  std::vector<std::shared_ptr<Window>> windows;

 private:
  uint32_t hz_ = 0;
  uint32_t ms_per_update_ = 0;
};
}  // namespace Aubengine