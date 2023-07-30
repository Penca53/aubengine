#pragma once

#include "aubengine/game_object.h"

class SpriteRenderer {
 public:
  // Renders a defined quad textured with given sprite
  void DrawSprite(GameObject* go);
};