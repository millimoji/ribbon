#include "pch.h"
#include "stdafx.h"
#include "CppUnitTest.h"
#include "inputmodel/SymEmojiList.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace Ribbon;

namespace UnitTests
{
	TEST_CLASS(SymEmojiList)
	{
	public:

		TEST_METHOD(VerifyHuman)
		{
			auto listManager = FACTORYCREATE(SymbolEmojiList);

			auto emojiList = listManager->GetList(SymbolEmojiCategory::Human, 0x10);

			Assert::IsTrue(emojiList->PhraseCount() > 10);
		}
	};
}