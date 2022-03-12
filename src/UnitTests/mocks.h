#pragma once
#ifndef _RIBBON_MOCK_H_
#define _RIBBON_MOCK_H_

namespace UnitTests {

struct IMockInteger
{
	virtual int GetInteger() = 0;
};
struct IMockFloat
{
	virtual float GetFloat() = 0;
};

extern void RegisterDifferentGenerator();

FACTORYEXTERN2(Ribbon::IObject, MockIntFloat);
}
#endif // _RIBBON_MOCK_H_
