#pragma once

class GameObject;

class Component {
 public:
  void SetOwner(GameObject* go) { game_object = go; }
  virtual void Start() {}
  virtual void PhysicsUpdate() {}
  virtual void Update() {}

  virtual ~Component() = default;

 public:
  bool is_enabled = true;
  GameObject* game_object = nullptr;
};