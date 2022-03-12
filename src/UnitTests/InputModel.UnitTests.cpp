#include "pch.h"
#include <inputmodel/LiteralConvert.h>
#include <inputmodel/InputModel.h>
#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	TEST_CLASS(InputModel)
	{
	public:

		TEST_METHOD(TestJaRomajiConvert)
		{
			std::shared_ptr<ILiteralConvert> romajiConv = FACTORYCREATE(JaLiteralConvert);

			RefString converted = romajiConv->ConvertText(RefString(u"aiueo"));
			Assert::IsTrue(converted == UT_TEXT_AIUEO);

			converted = romajiConv->ConvertText(RefString(u"kamennraida-"));
			Assert::IsTrue(converted == UT_TEXT_KAMENRAIDAH);

			converted = romajiConv->ConvertText(RefString(u"kamenraida-"));
			Assert::IsTrue(converted == UT_TEXT_KAMENRAIDAH);

			converted = romajiConv->ConvertText(RefString(u"kikkake"));
			Assert::IsTrue(converted == UT_TEXT_KIKKAKE);

			converted = romajiConv->ConvertText(RefString(u"www"));
			Assert::IsTrue(converted == u"www");

			converted = romajiConv->ConvertText(RefString(u"kud"));
			Assert::IsTrue(converted == UT_TEXT_PARTIALROMAJI_KUD);
		}

		TEST_METHOD(TestInputModel)
		{
			std::shared_ptr<IInputModel> inputModel = FACTORYCREATE(InputModel);

			inputModel->EventHandler(KeyEvent::Create(u"k"));
			RefString text = inputModel->CompositionText()->Display();
			Assert::IsTrue(text == u"k");
			inputModel->EventHandler(KeyEvent::Create(SIPKEY_BACK));

			inputModel->EventHandler(KeyEvent::Create(u"a"));
			Assert::IsTrue(inputModel->CandidateList()->PhraseCount() > 2);
			inputModel->EventHandler(KeyEvent::Create(SIPKEY_ENTER));
			text = inputModel->InsertingText();
			Assert::IsTrue(text == UT_TEXT_A);
			Assert::IsTrue(inputModel->CompositionText()->Display().length() == 0);

			inputModel->EventHandler(KeyEvent::Create(u"o"));
			inputModel->EventHandler(KeyEvent::Create(u"c"));
			inputModel->EventHandler(KeyEvent::Create(u"h"));
			inputModel->EventHandler(KeyEvent::Create(u"a"));
			inputModel->EventHandler(KeyEvent::Create(u"m"));
			inputModel->EventHandler(KeyEvent::Create(u"e"));

			text = inputModel->CompositionText()->Display();
			Assert::IsTrue(text == UT_TEXT_OCHAME);

			inputModel->EventHandler(KeyEvent::Create(SIPKEY_BACK));
			inputModel->EventHandler(KeyEvent::Create(SIPKEY_BACK));
			text = inputModel->CompositionText()->Display();
			Assert::IsTrue(text == UT_TEXT_OCHI);
			Assert::IsTrue(inputModel->CandidateList()->PhraseCount() > 2);

			inputModel->EventHandler(CandidateEvent::Create(CandidateEvent::EventType::CandidateChosen, 0));
			text = inputModel->InsertingText();
			Assert::IsTrue(text.length() > 0);
			Assert::IsTrue(inputModel->CompositionText()->Display().length() == 0);
		}

		TEST_METHOD(Test_BugFixRomajiCaret)
		{
			std::shared_ptr<IInputModel> inputModel = FACTORYCREATE(InputModel);

			inputModel->EventHandler(KeyEvent::Create(u"a"));
			inputModel->EventHandler(KeyEvent::Create(u"a"));
			inputModel->EventHandler(KeyEvent::Create(u"a"));

			inputModel->EventHandler(KeyEvent::Create(SIPKEY_LEFT));
			inputModel->EventHandler(KeyEvent::Create(SIPKEY_LEFT));

			inputModel->EventHandler(KeyEvent::Create(u"c"));
			inputModel->EventHandler(KeyEvent::Create(u"h"));
			inputModel->EventHandler(KeyEvent::Create(u"a"));

			inputModel->EventHandler(KeyEvent::Create(u"i"));

			const auto& currentDisplay = inputModel->CompositionText()->Display().u16str();
			Assert::IsTrue(currentDisplay == UT_TEXT_DISPLAY_ACHAIAA);
		}

		TEST_METHOD(Test_BugFixRomajiNN)
		{
			std::shared_ptr<IInputModel> inputModel = FACTORYCREATE(InputModel);

			inputModel->EventHandler(KeyEvent::Create(u"a"));
			inputModel->EventHandler(KeyEvent::Create(u"i"));

			inputModel->EventHandler(KeyEvent::Create(SIPKEY_LEFT));

			inputModel->EventHandler(KeyEvent::Create(u"n"));
			inputModel->EventHandler(KeyEvent::Create(u"a"));

			const auto& currentDisplay = inputModel->CompositionText()->Display().u16str();
			Assert::IsTrue(currentDisplay == UT_TEXT_DISPLAY_ANAI);
		}

		/*
		TEST_METHOD(TestJsonModel)
		{
			std::shared_ptr<IJsonInputModel> jsonModel = FACTORYCREATE(JsonInputModel);

			const char* srcJson = R"({"label":"a","keyId":"qw-a","keyName":"a","shiftState":false,"capsState":false,"command":"KeyPress"})";

			jsonModel->JsonCommand(srcJson);

			std::string res = jsonModel->CompositionState();
			std::string error;
			auto jsonData = json11::Json::parse(res, error);

			const auto& compositionRes = to_utf16(jsonData[JSON_COMPOSITION][0][JSON_DISPLAY].string_value());

			Assert::IsTrue(compositionRes == UT_TEXT_A);

			std::string cand = jsonModel->CandidateState();
			auto jsonCand = json11::Json::parse(cand, error);

			const auto& candList = jsonCand[JSON_CANDIDATES];
			Assert::IsTrue(candList.array_items().size() > 0);
			Platform->Printf("[%s]\n", candList[0].string_value().c_str());
		}
*/
	};
}