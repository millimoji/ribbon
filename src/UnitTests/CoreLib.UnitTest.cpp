#include "pch.h"
#include "stdafx.h"
#include "CppUnitTest.h"
#include "mocks.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	TEST_CLASS(CoreLib)
	{
	public:
		TEST_METHOD(VerifyFactory)
		{
			{
				auto obj = FACTORYCREATE(MockIntFloat);
				auto intPtr = std::dynamic_pointer_cast<IMockInteger>(obj);

				Assert::IsTrue(!!intPtr);
				Assert::AreEqual(intPtr->GetInteger(), 10);

				// Platform->Printf("%s\n", obj->myself().name());
			}
			// Factory Overwrite
			{
				auto savedFactory = FACTORYFUNCTION(MockIntFloat);
				auto scopeExit = ScopeExit([&]() { FACTORYFUNCTION(MockIntFloat) = savedFactory; });

				RegisterDifferentGenerator();

				auto obj = FACTORYCREATE(MockIntFloat);
				auto intPtr = std::dynamic_pointer_cast<IMockInteger>(obj);

				Assert::IsTrue(!!intPtr);
				Assert::AreEqual(intPtr->GetInteger(), 20);

				// automatically recovered
			}
			// generator back to original
			{
				auto obj = FACTORYCREATE(MockIntFloat);
				auto intPtr = std::dynamic_pointer_cast<IMockInteger>(obj);

				Assert::IsTrue(!!intPtr);
				Assert::AreEqual(intPtr->GetInteger(), 10);
			}
		}

		void SubTestRefString(const char16_t* srcStr)
		{
			RefString rstr(srcStr);
			Assert::IsTrue(textcmp(rstr.u16ptr(), srcStr) == 0);
			Assert::AreEqual(rstr.length(), textlen(srcStr));

			RefString rstrCopy1(rstr);
			Assert::IsTrue(rstr == rstrCopy1);
			Assert::IsTrue(textcmp(rstr.u16ptr(), rstrCopy1.u16ptr()) == 0);

			RefString rstrCopy2; rstrCopy2 = rstr;
			Assert::IsTrue(rstr == rstrCopy2);
			Assert::IsTrue(textcmp(rstr.u16ptr(), rstr.u16ptr()) == 0);

			RefString rstrNull;
			Assert::IsTrue(rstrNull.u16ptr() != nullptr);
			Assert::IsTrue(rstrNull.length() == 0); // not null, length 0
			Assert::IsTrue(rstr > rstrNull);
			Assert::IsTrue(rstrNull < rstr);
		}

		TEST_METHOD(TestRefString)
		{
			Assert::AreEqual(sizeof(RefString), sizeof(void*));

			std::atomic_ulong refCount = 1;
			Assert::AreEqual(sizeof(refCount), static_cast<size_t>(4));

			SubTestRefString(u"a");

			SubTestRefString(u"123456789012");

			SubTestRefString(u"12345678901234567890");

			SubTestRefString(u"123456789012345678901");

			// constructor, assignemnt operator do not cause memory leak
			uint32_t beforeCount = RefString::GetActiveInstanceCount();
			{
				RefString refSrc(u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 1);
				RefString refTar(refSrc); // copy constructor
				Assert::IsTrue(refSrc == u"ABC");
				Assert::IsTrue(refTar == u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 2 && refTar.GetReferenceCount() == 2);
			}
			Assert::IsTrue(beforeCount == RefString::GetActiveInstanceCount());
			{
				RefString refSrc(u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 1);
				RefString refTar(u"DEF");
				Assert::IsTrue(refTar.GetReferenceCount() == 1);
				refTar = refSrc; // operator = in copy
				Assert::IsTrue(refSrc == u"ABC");
				Assert::IsTrue(refTar == u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 2 && refTar.GetReferenceCount() == 2);
			}
			{
				RefString refSrc(u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 1);
				RefString refTar(std::move(refSrc)); // move constructor
				Assert::IsTrue(!refSrc);
				Assert::IsTrue(refTar == u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 0 && refTar.GetReferenceCount() == 1);
			}
			Assert::IsTrue(beforeCount == RefString::GetActiveInstanceCount());
			{
				RefString refSrc(u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 1);
				RefString refTar(u"DEF");
				Assert::IsTrue(refTar.GetReferenceCount() == 1);
				refTar = std::move(refSrc); // operator = in move context
				Assert::IsTrue(!refSrc);
				Assert::IsTrue(refTar == u"ABC");
				Assert::IsTrue(refSrc.GetReferenceCount() == 0 && refTar.GetReferenceCount() == 1);
			}
			Assert::IsTrue(beforeCount == RefString::GetActiveInstanceCount());
		}

		TEST_METHOD(VerifySingleSemaphore)
		{
			SingleSemaphore prepareEvent;
			SingleSemaphore resultEvent;
			std::atomic_int value = 0;

			auto task1 = std::thread([&] {
				prepareEvent.wait();

				value += 1;

				resultEvent.signal();
			});

			auto task2 = std::thread([&] {
				prepareEvent.wait();

				value += 10;

				resultEvent.signal();
			});

			auto task3 = std::thread([&] {
				prepareEvent.wait();

				value += 100;

				resultEvent.signal();
			});

			Assert::IsTrue(value == 0);

			prepareEvent.signal();
			resultEvent.wait();

			Assert::IsTrue(value == 1 || value == 10 || value == 100);

			prepareEvent.signal();
			resultEvent.wait();

			Assert::IsTrue(value == 11 || value == 110 || value == 101);

			prepareEvent.signal();
			resultEvent.wait();

			Assert::IsTrue(value == 111);

			task1.join();
			task2.join();
			task3.join();
		}

		TEST_METHOD(VerifyAsyncTask)
		{
			int marker = 0;

			std::shared_ptr<IAsyncTask> asyncTask;

			asyncTask = std::make_shared<AsyncTask<bool>>([&]() -> bool {

				Assert::IsTrue(asyncTask->GetState() == IAsyncTask::State::running);

				++marker;
				return true;
			});

			Assert::IsTrue(asyncTask->GetState() == IAsyncTask::State::waiting);

			asyncTask->Run();

			Assert::IsTrue(asyncTask->GetState() == IAsyncTask::State::finished);
			Assert::AreEqual(marker, 1);
		}

		TEST_METHOD(VerifyThreadPool)
		{
			ThreadPool threadPool;
			std::atomic_int maxValue = 0;
			std::atomic_int value = 0;

			auto testFunctor = [&]() -> bool {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				++value;
				maxValue = static_cast<int>(std::max(maxValue, value));
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				--value;
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				return true;
			};

			std::vector<std::shared_ptr<AsyncTask<bool>>> taskList;
			taskList.push_back(threadPool.Request<bool>(testFunctor));
			taskList.push_back(threadPool.Request<bool>(testFunctor));
			taskList.push_back(threadPool.Request<bool>(testFunctor));
			taskList.push_back(threadPool.Request<bool>(testFunctor));
			taskList.push_back(threadPool.Request<bool>(testFunctor));
			taskList.push_back(threadPool.Request<bool>(testFunctor));

			for (auto& it : taskList)
			{
				Assert::IsTrue(it->GetResult());
			}

			Assert::IsTrue(maxValue > 1);
		}

		TEST_METHOD(VerifyContainers)
		{
			auto supplement  = FACTORYCREATE(Supplement);
			auto primitive = FACTORYCREATE(Primitive);
			auto phrase = FACTORYCREATE(Phrase);
			auto phraseList = FACTORYCREATE(PhraseList);
			auto lattice = FACTORYCREATE(Lattice);

			Assert::IsTrue(!!supplement);
			Assert::IsTrue(!!primitive);
			Assert::IsTrue(!!phrase);
			Assert::IsTrue(!!phraseList);
			Assert::IsTrue(!!lattice);
		}

		TEST_METHOD(VerifySupplement)
		{
			const UUID uuidText(0x44883069, 0xa1ed, 0x4fcc, 0xba, 0xc8, 0xfb, 0x45, 0x40, 0x7, 0xe2, 0xa9);
			const UUID uuidInt(0x76d0d642, 0x8449, 0x4fce, 0xab, 0x1c, 0x1c, 0x98, 0xb7, 0x49, 0xfb, 0xe7);
			const UUID uuidObj(0x538779c9, 0x62ab, 0x4baf, 0xa3, 0xd0, 0x81, 0x89, 0x6b, 0x15, 0xa9, 0x8f);

			auto supplement = FACTORYCREATE(Supplement);
			supplement->SetString(uuidText, u"ABC");
			auto refStr = supplement->GetString(uuidText);
			Assert::IsTrue(refStr == u"ABC");

			supplement->SetData(uuidInt, (int)123);
			int intData = supplement->GetData<int>(uuidInt);
			Assert::IsTrue(intData == 123);

			auto primitive = FACTORYCREATE(Primitive);
			primitive->Reading(RefString(u"TestReading"));
			supplement->SetObject(uuidObj, primitive);
			std::shared_ptr<IPrimitive> objData = supplement->GetObject<IPrimitive>(uuidObj);
			Assert::IsTrue(objData->Reading() == u"TestReading");
		}

		TEST_METHOD(VerifySizeDataPair)
		{
			const char16_t testDataSmall[] = u"ABC";
			SizeDataPair smallData(testDataSmall, sizeof(testDataSmall));
			Assert::IsTrue(smallData.GetDataSize() == sizeof(testDataSmall));
			Assert::IsTrue(memcmp(smallData.GetData(), testDataSmall, sizeof(testDataSmall)) == 0);

			const char16_t testDataLarge[] = u"DEFGHIJKLMN";
			SizeDataPair largeData(testDataLarge, sizeof(testDataLarge));
			Assert::IsTrue(largeData.GetDataSize() == sizeof(testDataLarge));
			Assert::IsTrue(memcmp(largeData.GetData(), testDataLarge, sizeof(testDataLarge)) == 0);

			// move context
			SizeDataPair moveConst(std::move(smallData));
			Assert::IsTrue(moveConst.GetDataSize() == sizeof(testDataSmall));
			Assert::IsTrue(memcmp(moveConst.GetData(), testDataSmall, sizeof(testDataSmall)) == 0);
			Assert::IsTrue(smallData.GetDataSize() == 0);
			Assert::IsTrue(smallData.GetData() == nullptr);

			// move assignment
			SizeDataPair moveAssign(std::move(largeData));
			Assert::IsTrue(moveAssign.GetDataSize() == sizeof(testDataLarge));
			Assert::IsTrue(memcmp(moveAssign.GetData(), testDataLarge, sizeof(testDataLarge)) == 0);
			Assert::IsTrue(largeData.GetDataSize() == 0);
			Assert::IsTrue(largeData.GetData() == nullptr);
		}
	};
}