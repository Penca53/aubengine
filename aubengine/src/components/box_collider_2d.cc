#include "aubengine/components/box_collider_2d.h"

#include "aubengine/game_object.h"

BoxCollider2D::BoxCollider2D() : size(glm::vec3(1, 1, 1)) {}
BoxCollider2D::BoxCollider2D(glm::vec3 size) : size(size) {}

void BoxCollider2D::Start() {
  b2PolygonShape dynamicBox;
  auto actual_size_x = size.x * game_object->transform->size.x;
  auto actual_size_y = size.y * game_object->transform->size.y;

  dynamicBox.SetAsBox(actual_size_x / 2, actual_size_y / 2);

  b2FixtureDef fixtureDef;
  fixtureDef.shape = &dynamicBox;
  fixtureDef.density = 1.0f;
  fixtureDef.friction = 0.3f;

  if (game_object->rigid_body_2d) {
    game_object->rigid_body_2d->HandleNewCollider(fixtureDef);
  }
}

void BoxCollider2D::Update() {}