#include "pch.h"
#include "stdafx.h"
#include "CppUnitTest.h"
#include "mocks.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	TEST_CLASS(CppFeature)
	{
	public:

		TEST_METHOD(VerifyAtomicUlongSize)
		{
			std::atomic_ulong refCount;
			Assert::AreEqual(sizeof(refCount), static_cast<size_t>(4));

			Assert::AreEqual(sizeof(int), static_cast<size_t>(4));
			Assert::AreEqual(sizeof(short), static_cast<size_t>(2));
			Assert::AreEqual(sizeof(char), static_cast<size_t>(1));
			Assert::AreEqual(sizeof(char16_t), static_cast<size_t>(2));
			Assert::AreEqual(sizeof(char32_t), static_cast<size_t>(4));
			// sizeof(long) is platform depends.
			// sizeof(wchar_t) is platform depends.
		}

		TEST_METHOD(VerifyThread)
		{
			std::atomic_int value = 0;

			auto task = std::thread([&] {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				++value;
			});

			Assert::IsTrue(value == 0);

			task.join();

			Assert::IsTrue(value == 1);
		}


	};
}