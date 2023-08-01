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
  auto it = std::find_if(components_.begin(), components_.end(),
                         [component](std::shared_ptr<Component> const& i) {
                           return i.get() == component;
                         });

  if (it != components_.end()) {
    components_.erase(it);
  }
}

Scene* GameObject::GetScene() { return scene_; }