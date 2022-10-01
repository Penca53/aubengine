#pragma once

#include "Input.h"

class InputGLFW : public Input
{
protected:
	virtual bool GetKeyDownImpl(int key) const override;
	virtual bool GetKeyImpl(int key) const override;
	virtual bool GetKeyUpImpl(int key) const override;

	virtual void PollInputImpl() override;
};