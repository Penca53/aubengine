#include "aubengine/components/transform.h"

#include <iostream>

#include "aubengine/components/rigid_body_2d.h"
#include "aubengine/game_object.h"

void Transform::Update() {
  if (!game_object->rigid_body_2d) {
    return;
  }

  auto pos = game_object->rigid_body_2d->body->GetPosition();
  position = {pos.x, pos.y, 0};
}