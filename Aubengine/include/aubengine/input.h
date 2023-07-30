#pragma once

#include <bitset>

class Input {
 public:
  static void PollInput() { instance_->PollInputImpl(); }

  static bool GetKeyDown(int key) { return instance_->GetKeyDownImpl(key); }
  static bool GetKey(int key) { return instance_->GetKeyImpl(key); }
  static bool GetKeyUp(int key) { return instance_->GetKeyUpImpl(key); }

 protected:
  virtual void PollInputImpl() = 0;

  virtual bool GetKeyDownImpl(int key) const = 0;
  virtual bool GetKeyImpl(int key) const = 0;
  virtual bool GetKeyUpImpl(int key) const = 0;

 protected:
  std::bitset<512> key_states_{};
  std::bitset<512> old_key_states_{};

 private:
  static Input* instance_;
};