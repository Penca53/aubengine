#pragma once

#include <memory>
#include <unordered_set>

#include "aubengine/game_object.h"
#include "aubengine/utils/derived.h"

class Window;
class Prefab;
class SpriteRenderer;

class Scene {
 public:
  Scene(Window* window, SpriteRenderer* renderer);
  virtual ~Scene() = default;

  void PhysicsUpdate();
  void Update();
  void Render();

  template <Derived<GameObject> T>
  T* Instantiate() {
    std::shared_ptr<T> go = std::make_shared<T>(this);
    game_objects_.insert(go);
    return go.get();
  }
  void Destroy(GameObject* gameObject);
  Window* GetWindow();

 private:
  Window* window_ = nullptr;
  SpriteRenderer* renderer_ = nullptr;

  std::unordered_set<std::shared_ptr<GameObject>> game_objects_;
};