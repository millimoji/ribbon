#include "pch.h"
#include "stdafx.h"
#include "mocks.h"

using namespace Ribbon;

namespace UnitTests {

class MockIntFloat :
	public std::enable_shared_from_this<MockIntFloat>,
	public IMockInteger,
	public IMockFloat,
	public IObject
{
public:
	MockIntFloat(int n = 10, float f = 20.0f) :
		m_n(n), m_f(f) {}

	int GetInteger() override { return m_n; }
	float GetFloat() override { return m_f; }

	int m_n;
	float m_f;

	IOBJECT_COMMON_METHODS
};

void RegisterDifferentGenerator() {
	FACTORYFUNCTION(MockIntFloat) = []() -> std::shared_ptr<IObject> { return std::make_shared<MockIntFloat>(20, 30.0f); };
}

FACTORYDEFINE2(IObject, MockIntFloat);
}
