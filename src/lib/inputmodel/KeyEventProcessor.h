#pragma once
#ifndef _RIBBON_KEYEVENTPROCESSOR_H_
#define _RIBBON_KEYEVENTPROCESSOR_H_
#include "IEditContext.h"
namespace Ribbon {

struct IKeyEventProcessor : public ITaskProcessor
{
	virtual uint32_t ModifierState() = 0;
	virtual void ModifierState(uint32_t newState) = 0;
};

FACTORYEXTERN(KeyEventProcessor);
} // Ribbon
#endif // _RIBBON_KEYEVENTPROCESSOR_H_
