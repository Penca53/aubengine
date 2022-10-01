#pragma once

#include <bitset>

class Input
{
public:
	static void PollInput() { _instance->PollInputImpl(); }

	static bool GetKeyDown(int key) { return _instance->GetKeyDownImpl(key); }
	static bool GetKey(int key) { return _instance->GetKeyImpl(key); }
	static bool GetKeyUp(int key) { return _instance->GetKeyUpImpl(key); }
protected:
	virtual void PollInputImpl() = 0;

	virtual bool GetKeyDownImpl(int key) const = 0;
	virtual bool GetKeyImpl(int key) const = 0;
	virtual bool GetKeyUpImpl(int key) const = 0;
protected:
	std::bitset<512> _KeyStates{};
	std::bitset<512> _OldKeyStates{};
private:
	static Input* _instance;
};