#include "aubengine/scene.h"

#include "aubengine/game_object.h"
#include "aubengine/sprite_renderer.h"

Scene::Scene(Window* window, SpriteRenderer* renderer)
    : window_(window), renderer_(renderer) {}

void Scene::Update() {
  for (const auto& go : game_objects_) {
    go->Update();
  }
}

void Scene::Render() {
  for (const auto& go : game_objects_) {
    renderer_->DrawSprite(go.get());
  }
}

void Scene::Destroy(GameObject* game_object) {
  auto it = std::find_if(game_objects_.begin(), game_objects_.end(),
                         [game_object](std::shared_ptr<GameObject> const& i) {
                           return i.get() == game_object;
                         });

  if (it != game_objects_.end()) {
    game_objects_.erase(it);
  }
}

Window* Scene::GetWindow() { return window_; }