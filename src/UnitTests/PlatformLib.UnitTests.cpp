#include "pch.h"
#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	
	std::shared_ptr<ISetting> s_settingGrabbed;
	TEST_MODULE_INITIALIZE(PlatformLibInit) {
		setlocale(LC_ALL, "C");
		s_settingGrabbed = Platform->GetSettings();
	}
	TEST_MODULE_CLEANUP(PlatformLibClean) {
		s_settingGrabbed.reset();
	}

	TEST_CLASS(PlatformLib)
	{
	public:

		TEST_METHOD(VerifyUUID)
		{
			UUID uuid1 = Platform->CreateUUID();
			UUID uuid2 = Platform->CreateUUID();

			Assert::AreEqual(sizeof(uuid1), static_cast<size_t>(16));
			Assert::IsTrue(uuid1 == uuid1);
			Assert::IsFalse(uuid1 == uuid2);
		}

		TEST_METHOD(VerifyGetPathByType)
		{
			std::string currentPath = Platform->GetPathByType(IPlatform::PathType::Current);
			Platform->Printf("Current: %s\n", currentPath.c_str());

			std::string binPath = Platform->GetPathByType(IPlatform::PathType::BinPath);
			Platform->Printf("BinPath: %s\n", binPath.c_str());

			std::string userPath = Platform->GetPathByType(IPlatform::PathType::UserPath);
			Platform->Printf("UserPath: %s\n", userPath.c_str());
		}

		TEST_METHOD(VerifyFileSetting)
		{
			const char* unitTestSection = "UnitTest";
			std::shared_ptr<ISetting> setting = Platform->GetSettings();
			const auto& text = setting->GetString(unitTestSection, "TestText");
			Assert::IsTrue(text == "abcdefghi");

			int intVar = setting->GetInt(unitTestSection, "TestInt");
			Assert::AreEqual(intVar, 1);

			double floatVar = setting->GetFloat(unitTestSection, "TestFloat");
			Assert::AreEqual(floatVar, 123.45);

			const auto& failedText = setting->GetString(unitTestSection, "FailedTextKey");
			Assert::AreEqual(failedText.length(), (size_t)0);

			int dynamicVal = setting->GetInt(unitTestSection, "DynamicIntKey");
			setting->SetInt(unitTestSection, "DynamicIntKey", dynamicVal + 1);
			int newVal = setting->GetInt(unitTestSection, "DynamicIntKey");
			Assert::AreEqual(dynamicVal + 1, newVal);

			setting->SetInt(unitTestSection, "DynamicIntKey", 0);
		}

		TEST_METHOD(VerifyExpandedString)
		{
			const char* unitTestSection = "UnitTest";
			std::shared_ptr<ISetting> setting = Platform->GetSettings();

			std::string text = setting->GetExpandedString(unitTestSection, "TestExpandedString");
			Assert::IsTrue(text[0] != u'%');

			// no variable case
			text = setting->GetExpandedString(unitTestSection, "TestText");
			Assert::IsTrue(text[0] == u'a');
		}
	};
}