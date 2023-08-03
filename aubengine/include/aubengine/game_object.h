#pragma once

#include <string>
#include <unordered_set>

#include "aubengine/components/component.h"
#include "aubengine/components/rigid_body_2d.h"
#include "aubengine/components/transform.h"
#include "aubengine/utils/derived.h"

class Scene;

class GameObject {
 public:
  GameObject(const std::string& name, Scene* scene);

  void PhysicsUpdate();
  void Update();

  template <Derived<Component> T, typename... Args>
  T* AddComponent(Args... args) {
    if constexpr (std::is_same_v<T, Transform>) {
      if (transform == nullptr) {
        transform = std::make_shared<T>(args...);
        transform->SetOwner(this);
        transform->Start();
        return transform.get();
      } else {
        return nullptr;
      }
    }
    if constexpr (std::is_same_v<T, RigidBody2D>) {
      if (rigid_body_2d == nullptr) {
        rigid_body_2d = std::make_shared<T>(args...);
        rigid_body_2d->SetOwner(this);
        rigid_body_2d->Start();
        return rigid_body_2d.get();
      } else {
        return nullptr;
      }
    }

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
  std::shared_ptr<Transform> transform = nullptr;
  std::shared_ptr<RigidBody2D> rigid_body_2d = nullptr;

 private:
  std::unordered_set<std::shared_ptr<Component>> components_;
  Scene* scene_ = nullptr;
};