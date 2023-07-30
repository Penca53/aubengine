#include "aubengine/game_object.h"

#include "aubengine/components/component.h"

GameObject::GameObject(const std::string& name, Scene* scene)
    : name(name), scene_(scene) {}

void GameObject::Update() {
  for (auto it : components_) {
    if (it->is_enabled) {
      it->Update();
    }
  }
}
void GameObject::RemoveComponent(Component* component) {
  if (components_.count(component)) {
    components_.erase(component);
  }
}

Scene* GameObject::GetScene() { return scene_; }