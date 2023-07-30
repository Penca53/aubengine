#pragma once

#include <memory>
#include <unordered_set>

#include "aubengine/utils/derived.h"

class GameObject;
class Window;
class Prefab;
class SpriteRenderer;

class Scene {
 public:
  Scene(std::shared_ptr<Window> window, SpriteRenderer* renderer);
  virtual ~Scene() = default;

  void Update();
  void Render();

  template <Derived<GameObject> T>
  T* Instantiate() {
    T* go = new T(this);
    game_objects_.insert(go);
    return go;
  }
  void Destroy(GameObject* gameObject);
  std::shared_ptr<Window> GetWindow();

 private:
  std::shared_ptr<Window> window_ = nullptr;
  SpriteRenderer* renderer_ = nullptr;

  std::unordered_set<GameObject*> game_objects_;
};