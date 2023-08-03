#pragma once

#include <glm/glm.hpp>

#include "aubengine/application.h"
#include "aubengine/components/component.h"
#include "box2d/box2d.h"

class RigidBody2D : public Component {
 public:
  enum class BodyType {
    kStatic = 0,
    kKinematic,
    kDynamic,
  };

 public:
  BodyType body_type{};
  b2Body* body = nullptr;

 public:
  RigidBody2D();
  RigidBody2D(BodyType body_type);

  virtual void Start() override;
  virtual void PhysicsUpdate() override;
  virtual void Update() override;

  static void ChangeGravity(const b2Vec2& gravity);
  void HandleNewCollider(const b2FixtureDef& fixtureDef);

 public:
  static b2World world;
};