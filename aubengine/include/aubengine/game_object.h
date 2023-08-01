#pragma once

#include <string>
#include <unordered_set>

#include "aubengine/scene.h"
#include "aubengine/utils/derived.h"

class Component;

class GameObject {
 public:
  GameObject(const std::string& name, Scene* scene);

  void Update();

  template <Derived<Component> T, typename... Args>
  T* AddComponent(Args... args) {
    std::shared_ptr<T> component = std::make_shared<T>(args...);
    component->SetOwner(this);
    components_.insert(component);
    component->Start();
    return component.get();
  }
  template <Derived<Component> T>
  T* GetComponent() {
    for (const auto& c : components_) {
      T* a = dynamic_cast<T*>(c.get());
      if (a != nullptr) {
        return a;
      }
    }
    return nullptr;
  }
  void RemoveComponent(Component* component);
  Scene* GetScene();

 public:
  std::string name;

 private:
  std::unordered_set<std::shared_ptr<Component>> components_;
  Scene* scene_ = nullptr;
};