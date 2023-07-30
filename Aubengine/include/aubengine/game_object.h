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
    T* component = new T(args...);
    component->SetOwner(this);
    components_.insert(component);
    component->Start();
    return component;
  }
  template <Derived<Component> T>
  T* GetComponent() {
    for (auto c : components_) {
      T* a = dynamic_cast<T*>(c);
      if (a != nullptr) {
        return (T*)c;
      }
    }
    return nullptr;
  }
  void RemoveComponent(Component* component);
  Scene* GetScene();

 public:
  std::string name;

 private:
  std::unordered_set<Component*> components_;
  Scene* scene_ = nullptr;
};