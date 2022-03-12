#include "pch.h"
#include <dictionary/Transliterator.h>
#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;
using namespace Ribbon::Transliteration;

namespace UnitTests
{
	TEST_CLASS(LangModelTest)
	{
	public:

		TEST_METHOD(VerifyStaticSimpleConversion)
		{
			std::shared_ptr<ITransliterator> langModel = FACTORYCREATENS(Transliteration, Transliterator);

			RefString result = langModel->SimpleStringConversion(UT_TEXT_READING_KYOUHAIITENKIDESUNE);
			Assert::IsTrue(result == UT_TEXT_DISPLAY_KYOUHAIITENKIDESUNE);
			Platform->Printf("[%s]\n", result.u8str().c_str());

			result = langModel->SimpleStringConversion(UT_TEXT_READING_SUMOMOMOMOMOMOMOMONOUCHI);
			Platform->Printf("[%s]\n", result.u8str().c_str());

			result = langModel->SimpleStringConversion(UT_TEXT_READING_TONARUNOKYAKUHAYOKUKAKIKUUKYAKUDA);
			Platform->Printf("[%s]\n", result.u8str().c_str());
		}

		TEST_METHOD(VerifyStaticSimplePrediction)
		{
			std::shared_ptr<ITransliterator> langModel = FACTORYCREATENS(Transliteration, Transliterator);

			RefString result = langModel->SimpleStringPrediction(UT_TEXT_READING_ARIGATOUGOZAIMA);
			Assert::IsTrue(result == UT_TEXT_READING_ARIGATOUGOZAIMASU || result == UT_TEXT_READING_ARIGATOUGOZAIMASHITA);
			Platform->Printf("[%s]\n", result.u8str().c_str());

			result = langModel->SimpleStringPrediction(UT_TEXT_READING_KA);
			Platform->Printf("[%s]\n", result.u8str().c_str());

			result = langModel->SimpleStringPrediction(UT_TEXT_READING_SUSHI);
			Platform->Printf("[%s]\n", result.u8str().c_str());

			result = langModel->SimpleStringPrediction(UT_TEXT_READING_DOUGA);
			Platform->Printf("[%s]\n", result.u8str().c_str());
		}

	};
}