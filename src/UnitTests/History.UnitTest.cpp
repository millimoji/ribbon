#include "pch.h"
#include "stdafx.h"
#include "CppUnitTest.h"
#include <dictionary/Transliterator.h>
#include <dictionary/DictionaryReader.h>
#include <history/HistoryStore.h>
#include <history/HistoryStruct.h>
#include <history/HistoryReuser.h>
#include <history/HistoryManager.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	struct FlexStreamMock : public IFlexibleBinStream
	{
		std::deque<uint8_t> m_data;
		std::vector<uint8_t> m_view;

		uint32_t Write(const uint8_t* ptr, uint32_t size) override
		{
			for (uint32_t i = 0; i < size; ++i) {
				m_data.push_back(ptr[i]);
			}
			return size;
		}
		void Clear() { m_data.clear(); }
		uint32_t TotalSize() { return static_cast<uint32_t>(m_data.size()); }
		std::pair<const uint8_t*, const uint8_t*> CreateMemoryStream() {
			m_view = std::vector<uint8_t>(m_data.begin(), m_data.end());
			return std::make_pair(&m_view.front(), &m_view.back() + 1);
		}

	};

	TEST_CLASS(HistoryTest)
	{
	private:
		std::shared_ptr<Dictionary::IDictionaryReader> OpenSystemDictionary()
		{
			std::shared_ptr<ISetting> setting = Platform->GetSettings();
			const char* dictionariesSection = "Dictionaries";
			std::string  dictionaryFile = setting->GetExpandedString(dictionariesSection, "SystemDictionary");
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = Dictionary::FACTORYCREATE(DictionaryReader);
			dictReader->Open(dictionaryFile.c_str());
			return dictReader;
		}

	public:
		TEST_METHOD(VerifyFlexBinContainer)
		{
			for (uint32_t val = 1; val != 0; val <<= 1)
			{	// int
				FLEXBIN_ELEMENT src((val & ~3) + FLEXBIN_TYPE_INT32, val);

				FlexStreamMock stream;
				uint32_t writeSize = src.WriteData(&stream);

				const uint8_t *dataPtr, *endPtr;
				std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

				FLEXBIN_ELEMENT copy;
				const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);

				Assert::IsTrue(endPtr == finishPtr);
				Assert::IsTrue(src == copy);
			}

			for (uint32_t val = 1; val != 0; val <<= 1)
			{	// int
				uint32_t testVal = val * 2 - 1;
				FLEXBIN_ELEMENT src((val & ~3) + FLEXBIN_TYPE_INT32, testVal);

				FlexStreamMock stream;
				uint32_t writeSize = src.WriteData(&stream);

				const uint8_t *dataPtr, *endPtr;
				std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

				FLEXBIN_ELEMENT copy;
				const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);

				Assert::IsTrue(endPtr == finishPtr);
				Assert::IsTrue(src == copy);
			}

			{	// pair
				FLEXBIN_ELEMENT sub1((0x12 << 2) + FLEXBIN_TYPE_INT32, 0x12345678);
				FLEXBIN_ELEMENT sub2((0x999 << 2) + FLEXBIN_TYPE_INT32, 0xABCDEF);
				FLEXBIN_ELEMENT src((0x123 << 2) + FLEXBIN_TYPE_PAIR, std::move(sub1), std::move(sub2));;

				FlexStreamMock stream;
				uint32_t writeSize = src.WriteData(&stream);

				const uint8_t *dataPtr, *endPtr;
				std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

				FLEXBIN_ELEMENT copy;
				const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);

				Assert::IsTrue(endPtr == finishPtr);
				Assert::IsTrue(src == copy);
			}

			{	// array
				const size_t elementCount = 10;
				FLEXBIN_ELEMENT* arrayElem = new FLEXBIN_ELEMENT[elementCount];
				arrayElem[0] = FLEXBIN_ELEMENT((0x1F << 2) + 0, 0x7F);
				arrayElem[1] = FLEXBIN_ELEMENT((0x20 << 2) + 0, 0x80);
				arrayElem[2] = FLEXBIN_ELEMENT((0x7FF << 2) + 0, 0x1FFF);
				arrayElem[3] = FLEXBIN_ELEMENT((0x800 << 2) + 0, 0x2000);
				arrayElem[4] = FLEXBIN_ELEMENT((0x1FFFF << 2) + 0, 0x7FFFF);
				arrayElem[5] = FLEXBIN_ELEMENT((0x20000 << 2) + 0, 0x80000);
				arrayElem[6] = FLEXBIN_ELEMENT((0x7FFFFF << 2) + 0, 0x1FFFFFF);
				arrayElem[7] = FLEXBIN_ELEMENT((0x800000 << 2) + 0, 0x2000000);
				arrayElem[8] = FLEXBIN_ELEMENT((0x3FFFFFFF << 2) + 0, 0xFFFFFFFF);
				arrayElem[9] = FLEXBIN_ELEMENT((0x9ABCDEF0 << 2) + 0, 0x12345678);
				FLEXBIN_ELEMENT src((0xFED << 2) + FLEXBIN_TYPE_ARRAY, 10, std::move(arrayElem));

				FlexStreamMock stream;
				uint32_t writeSize = src.WriteData(&stream);

				const uint8_t *dataPtr, *endPtr;
				std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

				FLEXBIN_ELEMENT copy;
				const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);

				Assert::IsTrue(endPtr == finishPtr);
				Assert::IsTrue(src == copy);
			}

			{	// blob
				FLEXBIN_ELEMENT src((0x3DEF123 << 2) + FLEXBIN_TYPE_BLOB, u"AbCdEfG");

				FlexStreamMock stream;
				uint32_t writeSize = src.WriteData(&stream);

				const uint8_t *dataPtr, *endPtr;
				std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

				FLEXBIN_ELEMENT copy;
				const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);

				Assert::IsTrue(endPtr == finishPtr);
				Assert::IsTrue(src == copy);
			}
		}

		TEST_METHOD(VerifyHistoryClassList)
		{
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = OpenSystemDictionary();
			std::shared_ptr<History::IHistoryClassList> histClassList = FACTORYCREATENS(History, HistoryClassList);

			histClassList->Initialize(dictReader.get());

			const uint32_t usingMark = 0x123;
			dictReader->CreatePosNameReader().EnumeratePosName([&](const char16_t *posName, uint16_t classId) {
				histClassList->GetFromClassId(classId)->usingMark = usingMark;
			});
			histClassList->UpdateUsingMark(usingMark);

			FlexStreamMock stream;
			uint32_t writeSize = histClassList->Write(&stream);

			const uint8_t *dataPtr, *endPtr;
			std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

			FLEXBIN_ELEMENT copy;
			const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);

			FlexStreamMock streamCopy;
			uint32_t writeSizeCopy = copy.WriteData(&streamCopy);
			Assert::IsTrue(writeSize == writeSizeCopy);
			std::tie(dataPtr, endPtr) = streamCopy.CreateMemoryStream();

			dataPtr = histClassList->Read(dataPtr, endPtr);
		}

		TEST_METHOD(VerifyHistoryPrimList)
		{
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = OpenSystemDictionary();
			std::vector<uint16_t> classIdList;

			dictReader->CreatePosNameReader().EnumeratePosName([&](const char16_t *posName, uint16_t classId) {
				classIdList.emplace_back(classId);
			});

			std::shared_ptr<History::IHistoryClassList> histClassList = FACTORYCREATENS(History, HistoryClassList);
			histClassList->Initialize(dictReader.get());

			std::shared_ptr<History::IHistoryPrimList> histPrimList = FACTORYCREATENS(History, HistoryPrimList);
			histPrimList->Initialize(histClassList.get());
			{
				RefString displayText(u"abc"); RefString readingText(u"def");
				History::HistoryClass* classPtr = histClassList->GetFromClassId(classIdList[classIdList.size() / 2 + 0]);
				histPrimList->AddOrFindWord(displayText, readingText, classPtr, 3, true);
			}
			{
				RefString displayText(u"ade"); RefString readingText(u"deg");
				History::HistoryClass* classPtr = histClassList->GetFromClassId(classIdList[classIdList.size() / 2 + 1]);
				histPrimList->AddOrFindWord(displayText, readingText, classPtr, 3, true);
			}
			{
				RefString displayText(u"a"); RefString readingText(u"g");
				History::HistoryClass* classPtr = histClassList->GetFromClassId(classIdList[classIdList.size() / 2 + 2]);
				histPrimList->AddOrFindWord(displayText, readingText, classPtr, 3, true);
			}

			const uint32_t usingMark = 0x123;
			histPrimList->PrepareBeforeSaving(usingMark);
			histClassList->UpdateUsingMark(usingMark);

			uint32_t writeSize = 0;
			FlexStreamMock stream;
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_TYPE_ARRAY, &stream);
			writeSize += FlexBinUtil::WriteUint32(2, &stream);
			writeSize += histClassList->Write(&stream);
			writeSize += histPrimList->Write(&stream);

			const uint8_t *dataPtr, *endPtr;
			std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

			FLEXBIN_ELEMENT copy;
			const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);
			Assert::IsTrue(finishPtr == endPtr);

			FlexStreamMock streamCopy;
			uint32_t writeSizeCopy = copy.WriteData(&streamCopy);
			Assert::IsTrue(writeSize == writeSizeCopy);

			const uint8_t *copyTopPtr, *copyEndPtr;
			std::tie(copyTopPtr, copyEndPtr) = streamCopy.CreateMemoryStream();
			Assert::IsTrue(memcmp(copyTopPtr, dataPtr, copyEndPtr - copyTopPtr) == 0);

			std::shared_ptr<History::IHistoryClassList> histClassListCopy = FACTORYCREATENS(History, HistoryClassList);
			histClassListCopy->Initialize(dictReader.get());
			std::shared_ptr<History::IHistoryPrimList> histPrimListCopy = FACTORYCREATENS(History, HistoryPrimList);
			histPrimListCopy->PrepareBeforeLoading(histClassListCopy.get());

			const uint8_t* readingPtr = copyTopPtr;
			uint32_t tmp;
			readingPtr = FlexBinUtil::ReadUint32(readingPtr, copyEndPtr, &tmp);
			Assert::IsTrue(tmp == FLEXBIN_TYPE_ARRAY);
			readingPtr = FlexBinUtil::ReadUint32(readingPtr, copyEndPtr, &tmp);
			Assert::IsTrue(tmp == 2);

			readingPtr = histClassListCopy->Read(readingPtr, copyEndPtr);
			readingPtr = histPrimListCopy->Read(readingPtr, copyEndPtr);
			Assert::IsTrue(readingPtr == copyEndPtr);

			const auto& orgList = histPrimList->AllPrimitives();
			const auto& newList = histPrimListCopy->AllPrimitives();

			for (const auto& orgItem : orgList) {
				bool isFound = false;
				for (const auto& newItem : newList) {
					if (*orgItem == *newItem) {
						isFound = true;
						break;
					}
				}
				Assert::IsTrue(isFound);
			}
		}

		TEST_METHOD(VerifyHistoryNGramTree)
		{
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = OpenSystemDictionary();
			std::vector<uint16_t> classIdList;

			dictReader->CreatePosNameReader().EnumeratePosName([&](const char16_t *posName, uint16_t classId) {
				classIdList.emplace_back(classId);
			});
			std::shared_ptr<History::IHistoryClassList> histClassList = FACTORYCREATENS(History, HistoryClassList);
			histClassList->Initialize(dictReader.get());
			std::shared_ptr<History::IHistoryPrimList> histPrimList = FACTORYCREATENS(History, HistoryPrimList);
			histPrimList->Initialize(histClassList.get());

			std::vector<History::HistoryPrimitive*> primList;

			{	RefString displayText(u"abc"); RefString readingText(u"def");
				History::HistoryClass* classPtr = histClassList->GetFromClassId(classIdList[classIdList.size() / 2 + 0]);
				auto prim = histPrimList->AddOrFindWord(displayText, readingText, classPtr, 3, true);
				primList.emplace_back(prim);
			}
			{	RefString displayText(u"ade"); RefString readingText(u"deg");
				History::HistoryClass* classPtr = histClassList->GetFromClassId(classIdList[classIdList.size() / 2 + 1]);
				auto prim = histPrimList->AddOrFindWord(displayText, readingText, classPtr, 3, true);
				primList.emplace_back(prim);
			}
			{	RefString displayText(u"a"); RefString readingText(u"g");
				History::HistoryClass* classPtr = histClassList->GetFromClassId(classIdList[classIdList.size() / 2 + 2]);
				auto prim = histPrimList->AddOrFindWord(displayText, readingText, classPtr, 3, true);
				primList.emplace_back(prim);
			}

			std::shared_ptr<History::IHistoryNGramTree> histNGramTree = FACTORYCREATENS(History, HistoryNGramTree);
			histNGramTree->InsertSequence(&primList[0], 3, false);
			histNGramTree->InsertSequence(&primList[1], 2, false);
			histNGramTree->InsertSequence(&primList[2], 1, false);

			std::vector<History::HistoryPrimitive*> primList2 = { primList[2], primList[1], primList[0] };
			histNGramTree->InsertSequence(&primList2[0], 3, false);
			histNGramTree->InsertSequence(&primList2[1], 2, false);
			histNGramTree->InsertSequence(&primList2[2], 1, false);

			const uint32_t usingMark = 0x123;
			histPrimList->PrepareBeforeSaving(usingMark);
			histClassList->UpdateUsingMark(usingMark);

			uint32_t writeSize = 0;
			FlexStreamMock stream;
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_TYPE_ARRAY, &stream);
			writeSize += FlexBinUtil::WriteUint32(3, &stream);
			writeSize += histClassList->Write(&stream);
			writeSize += histPrimList->Write(&stream);
			writeSize += histNGramTree->Write(&stream);

			const uint8_t *dataPtr, *endPtr;
			std::tie(dataPtr, endPtr) = stream.CreateMemoryStream();

			FLEXBIN_ELEMENT copy;
			const uint8_t* finishPtr = copy.ReadData(dataPtr, endPtr);
			Assert::IsTrue(finishPtr == endPtr);

			FlexStreamMock streamCopy;
			uint32_t writeCopySize = 0;
			writeCopySize += copy.WriteData(&streamCopy);

			const uint8_t *copyDataPtr, *copyEndPtr;
			std::tie(copyDataPtr, copyEndPtr) = streamCopy.CreateMemoryStream();
			Assert::IsTrue((copyEndPtr - copyDataPtr) == (endPtr - dataPtr));
			Assert::IsTrue(memcmp(copyDataPtr, dataPtr, (endPtr - dataPtr)) == 0);

			const uint8_t* readingPtr = copyDataPtr;
			uint32_t tmp;
			readingPtr = FlexBinUtil::ReadUint32(readingPtr, copyEndPtr, &tmp);
			Assert::IsTrue(tmp == FLEXBIN_TYPE_ARRAY);
			readingPtr = FlexBinUtil::ReadUint32(readingPtr, copyEndPtr, &tmp);
			Assert::IsTrue(tmp == 3);

			std::shared_ptr<History::IHistoryClassList> histClassListCopy = FACTORYCREATENS(History, HistoryClassList);
			histClassListCopy->Initialize(dictReader.get());
			std::shared_ptr<History::IHistoryPrimList> histPrimListCopy = FACTORYCREATENS(History, HistoryPrimList);
			histPrimListCopy->PrepareBeforeLoading(histClassListCopy.get());
			std::shared_ptr<History::IHistoryNGramTree> histNGramTreeCopy = FACTORYCREATENS(History, HistoryNGramTree);
			histNGramTreeCopy->PrepareBeforeLoading(histPrimListCopy.get());

			readingPtr = histClassListCopy->Read(readingPtr, copyEndPtr);
			readingPtr = histPrimListCopy->Read(readingPtr, copyEndPtr);
			readingPtr = histNGramTreeCopy->Read(readingPtr, copyEndPtr);
			Assert::IsTrue(readingPtr == copyEndPtr);

			Assert::IsTrue(*histNGramTree->GetNGramNode() == *histNGramTreeCopy->GetNGramNode());
		}

		TEST_METHOD(VerifyHistorySaveLoad)
		{
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = OpenSystemDictionary();
			std::shared_ptr<History::IHistoryManager> historyMgr = FACTORYCREATENS(History, HistoryManager);
			historyMgr->Initialize(dictReader.get(), "", 1);
			std::shared_ptr<History::IHistoryReuser> reuser = historyMgr->GetReuser();

			auto langModel = FACTORYCREATENS(Transliteration, Transliterator);
			{
				auto inputLattice = langModel->StringToInputLattice(UT_TEXT_READING_KYOUHAIITENKIDESUNE);
				auto phraseList = langModel->Query(inputLattice.get());
				Assert::IsTrue(phraseList->PhraseCount() > 0);
				reuser->RegisterPhrase(phraseList->Phrase(0));
			}

			historyMgr->Save();
			historyMgr->Load();
		}

		TEST_METHOD(VerifyHistoryPrediction)
		{
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = OpenSystemDictionary();
			std::shared_ptr<History::IHistoryManager> historyMgr = FACTORYCREATENS(History, HistoryManager);
			historyMgr->Initialize(dictReader.get(), "", 1);
			std::shared_ptr<History::IHistoryReuser> reuser = historyMgr->GetReuser();

			auto langModel = FACTORYCREATENS(Transliteration, Transliterator);
			{
				auto inputLattice = langModel->StringToInputLattice(UT_TEXT_READING_KYOUHAIITENKIDESUNE);
				auto phraseList = langModel->Query(inputLattice.get());
				Assert::IsTrue(phraseList->PhraseCount() > 0);
				reuser->RegisterPhrase(phraseList->Phrase(0));
			}

			std::pair<const char16_t*, const char16_t*> readDispPair[] = {
				{ UT_TEXT_DISPLAY_KYOU, UT_TEXT_READING_KI },
				{ UT_TEXT_DISPLAY_KYOU, UT_TEXT_READING_KYO },
				{ UT_TEXT_DISPLAY_KYOU, UT_TEXT_READING_KYOU },
				{ UT_TEXT_DISPLAY_KYOUHA, UT_TEXT_READING_KYOUHA },
				{ UT_TEXT_DISPLAY_KYOUHAII, UT_TEXT_READING_KYOUHAI },
				{ UT_TEXT_DISPLAY_KYOUHAII, UT_TEXT_READING_KYOUHAII },
				{ UT_TEXT_DISPLAY_KYOUHAIITENKI, UT_TEXT_READING_KYOUHAIITE },
				{ UT_TEXT_DISPLAY_KYOUHAIITENKI, UT_TEXT_READING_KYOUHAIITEN },
				{ UT_TEXT_DISPLAY_KYOUHAIITENKI, UT_TEXT_READING_KYOUHAIITENKI },
				{ UT_TEXT_DISPLAY_KYOUHAIITENKIDESU, UT_TEXT_READING_KYOUHAIITENKIDE },
				{ UT_TEXT_DISPLAY_KYOUHAIITENKIDESU, UT_TEXT_READING_KYOUHAIITENKIDESU },
				{ UT_TEXT_DISPLAY_KYOUHAIITENKIDESUNE, UT_TEXT_READING_KYOUHAIITENKIDESUNE },
				{ UT_TEXT_DISPLAY_II, UT_TEXT_READING_I },
				{ UT_TEXT_DISPLAY_II, UT_TEXT_READING_II },
				{ UT_TEXT_DISPLAY_IITENKI, UT_TEXT_READING_IITE },
				{ UT_TEXT_DISPLAY_IITENKI, UT_TEXT_READING_IITEN },
				{ UT_TEXT_DISPLAY_IITENKI, UT_TEXT_READING_IITENKI },
				{ UT_TEXT_DISPLAY_IITENKIDESU, UT_TEXT_READING_IITENKIDE },
				{ UT_TEXT_DISPLAY_IITENKIDESU, UT_TEXT_READING_IITENKIDESU },
				{ UT_TEXT_DISPLAY_IITENKIDESUNE, UT_TEXT_READING_IITENKIDESUNE },
				{ UT_TEXT_DISPLAY_TENKI, UT_TEXT_READING_TE },
				{ UT_TEXT_DISPLAY_TENKI, UT_TEXT_READING_TEN },
				{ UT_TEXT_DISPLAY_TENKI, UT_TEXT_READING_TENKI },
				{ UT_TEXT_DISPLAY_TENKIDESU, UT_TEXT_READING_TENKIDE },
				{ UT_TEXT_DISPLAY_TENKIDESU, UT_TEXT_READING_TENKIDESU },
				{ UT_TEXT_DISPLAY_TENKIDESUNE, UT_TEXT_READING_TENKIDESUNE },
				{ UT_TEXT_DISPLAY_DESU, UT_TEXT_READING_DE },
				{ UT_TEXT_DISPLAY_DESU, UT_TEXT_READING_DESU },
				{ UT_TEXT_DISPLAY_DESUNE, UT_TEXT_READING_DESUNE },
				{ nullptr, nullptr }
			};

			for (int i = 0; readDispPair[i].first != nullptr; ++i)
			{
				const char16_t* inputHira = readDispPair[i].second;
				const char16_t* expectedKanji = readDispPair[i].first;

				auto inputLattice = langModel->StringToInputLattice(inputHira);
				reuser->SetContext(nullptr);
				auto phraseList = reuser->Query(inputLattice);
				Assert::IsTrue(phraseList->PhraseCount() > 0);
				Assert::IsTrue(phraseList->Phrase(0)->Display() == expectedKanji);
			}
		}

		TEST_METHOD(VerifyPredictionWithContext)
		{
			std::shared_ptr<Dictionary::IDictionaryReader> dictReader = OpenSystemDictionary();
			std::shared_ptr<History::IHistoryManager> historyMgr = FACTORYCREATENS(History, HistoryManager);
			historyMgr->Initialize(dictReader.get(), "", -1);
			std::shared_ptr<History::IHistoryReuser> reuser = historyMgr->GetReuser();

			std::shared_ptr<IPhrase> registPhrase = FACTORYCREATE(Phrase);
			registPhrase->Push(IPrimitive::CreatePrimitive(u"ABC", u"abc", 10, 3));
			reuser->RegisterLastWordWithOpenPhrases(registPhrase);
			registPhrase->Push(IPrimitive::CreatePrimitive(u"DEF", u"def", 10, 3));
			reuser->RegisterLastWordWithOpenPhrases(registPhrase);
			registPhrase->Push(IPrimitive::CreatePrimitive(u"GHI", u"ghi", 10, 3));
			reuser->RegisterLastWordWithOpenPhrases(registPhrase);
			registPhrase->Push(IPrimitive::CreatePrimitive(u"JKL", u"jkl", 10, 3));
			reuser->RegisterLastWordWithOpenPhrases(registPhrase);

			std::shared_ptr<IPhrase> contextPhrase = FACTORYCREATE(Phrase);
			contextPhrase->Push(IPrimitive::CreatePrimitive(u"ABC", u"abc", 10, 3));
			contextPhrase->Push(IPrimitive::CreatePrimitive(u"DEF", u"def", 10, 3));
			reuser->SetContext(contextPhrase);

			auto langModel = FACTORYCREATENS(Transliteration, Transliterator);
			auto inputLattice = langModel->StringToInputLattice(u"g");
			auto phraseList = reuser->Query(inputLattice);

			Assert::IsTrue(phraseList->PhraseCount() > 0);
			Assert::IsTrue(phraseList->Phrase(0)->Primitive(0)->Display() == u"GHI");
		}

	};
}