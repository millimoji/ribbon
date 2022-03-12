#include "pch.h"
#include "../compiler/DictionaryCompiler.h"
#include "../compiler/LangModelCalc.h"
#include "PhraseCounter.h"

namespace Ribbon { namespace Dictionary {

struct PhraseListBuilder :
	public std::enable_shared_from_this<PhraseListBuilder>,
	public IProgramMain,
	public ILexiconRegister,
	public IObject
{
	std::shared_ptr<ILexiconRegister> m_lexiconRegister;
	WordContainer m_wordContainer;

	PhraseListBuilder() {}
	virtual ~PhraseListBuilder() {}

	// IProgramMain
	int ProgramMain(int, char16_t const* const*) override
	{
		try
		{
			std::shared_ptr<ISetting> setting = Platform->GetSettings(true /*tryFallbackPath*/);

			std::shared_ptr<ILangModelCalculater> langModelCalculater = FACTORYCREATE(LangModelCalculater);
			langModelCalculater->AllowUnknownWords(true);
			langModelCalculater->BuildReverseLink(true);
			m_lexiconRegister = std::dynamic_pointer_cast<ILexiconRegister>(langModelCalculater);

			// read from dictionary
			std::shared_ptr<ILexiconReader> lexiconReader = FACTORYCREATE(LexiconReader);

			int lowerLimit = 1000000;
			lexiconReader->ReadSources(this, 7, lowerLimit);
			m_lexiconRegister = nullptr;

			langModelCalculater->GetWordContainer(m_wordContainer);

			std::shared_ptr<IPhraseCounter> phraseCounter = FACTORYCREATE(PhraseCounter);
			phraseCounter->DoCount(m_wordContainer, lowerLimit);

			GlobalThreadPool.Clear(); // before exiting, we need to kill sub threads.
			exit(0); // in order to skip heavy destructor, exiting program here.
		}
		CATCH_LOG();
		return 0;
	}

	// ILexiconRegister
	static bool isReadingCharacter(char16_t ch) {
		return (ch >= 0x3041 && ch <= 0x3096)
			|| (ch >= 0x30f4 && ch <= 0x30f6)
			|| (ch == 0x30fc);
	}
	void RegisterWord(const WORDINFO* wordInfo) override
	{
		// if reading contains other than hiragana, this item is ignored.
#if 0
		for (const char16_t* src = wordInfo->read; *src != 0; ++src)
		{
			if (!isReadingCharacter(*src))
			{
				return;
			}
		}
#endif
		m_lexiconRegister->RegisterWord(wordInfo);
	}
	void SetNgram(int nGram, const WORDINFO* wordList, uint64_t count) override {
		if (!TopWordLimitation(nGram, wordList))
		{
			return;
		}
#if 0
		for (int i = 0; i < nGram; ++i)
		{
			if (!isReadingCharacter(wordList[i].read[0]))
			{
				return;
			}
		}
#endif
		m_lexiconRegister->SetNgram(nGram, wordList, count);
	}
	void SetPhrase(int, const WORDINFO*, uint64_t) override {
		THROW_IF_FALSE_MSG(false, "should not be called.");
	}

	bool TopWordLimitation(int /*nGram*/, const WORDINFO* wordList)
	{
		std::u16string firstPos(wordList[0].pos);
		if (firstPos.find(CHECK_POS_JOSHI) != std::u16string::npos ||
			firstPos.find(CHECK_POS_JODOSHI) != std::u16string::npos ||
			firstPos.find(CHECK_POS_DOSHI_SETSUBI) != std::u16string::npos)
		{
			return false;
		}
		return true;
	}

	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE2(IProgramMain, PhraseListBuilder);
} /* namespace Dictionary */ } /* namespace Ribbon */
