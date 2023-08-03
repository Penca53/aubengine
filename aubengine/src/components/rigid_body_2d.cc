#include "aubengine/components/rigid_body_2d.h"

#include <iostream>

#include "aubengine/components/transform.h"
#include "aubengine/game_object.h"

b2World RigidBody2D::world({0.0f, -10.0f});

RigidBody2D::RigidBody2D() : body_type(RigidBody2D::BodyType::kStatic) {}

RigidBody2D::RigidBody2D(RigidBody2D::BodyType body_type)
    : body_type(body_type) {}

void RigidBody2D::ChangeGravity(const b2Vec2& gravity) {
  world.SetGravity(gravity);
}

void RigidBody2D::Start() {
  b2BodyDef bodyDef;
  bodyDef.type = (b2BodyType)body_type;
  auto pos = game_object->transform->position;
  bodyDef.position.Set(pos.x, pos.y);
  body = world.CreateBody(&bodyDef);
}

void RigidBody2D::PhysicsUpdate() {
  body->SetTransform(
      {game_object->transform->position.x, game_object->transform->position.y},
      game_object->transform->euler_rotation.x);
}

void RigidBody2D::Update() {}

void RigidBody2D::HandleNewCollider(const b2FixtureDef& fixtureDef) {
  body->CreateFixture(&fixtureDef);
}