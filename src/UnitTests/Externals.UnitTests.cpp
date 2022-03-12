#include "pch.h"
#include "stdafx.h"
#include "CppUnitTest.h"
#include "mocks.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	TEST_CLASS(Externals)
	{
	public:

		TEST_METHOD(VerifyJson11)
		{
			const char* srcJson = R"({"label":"s","keyId":"qw-s","keyName":"s","shiftState":false,"capsState":false,"command":"KeyPress"})";

			std::string err;
			json11::Json res = json11::Json::parse(srcJson, err);

			Assert::IsTrue(res["label"].string_value() == "s");
			Assert::IsFalse(res["shiftState"].bool_value());
		}
	};
}