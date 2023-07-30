#include "aubengine/scene.h"

#include "aubengine/game_object.h"
#include "aubengine/sprite_renderer.h"

Scene::Scene(std::shared_ptr<Window> window, SpriteRenderer* renderer) :window_(window), renderer_(renderer) {
}

void Scene::Update() {
  for (auto& go : game_objects_) {
    go->Update();
  }
}

void Scene::Render() {
  for (auto& go : game_objects_) {
    renderer_->DrawSprite(go);
  }
}

void Scene::Destroy(GameObject* game_object) {
  if (game_objects_.count(game_object)) {
    game_objects_.erase(game_object);
  }
}

std::shared_ptr<Window> Scene::GetWindow() { return window_; }