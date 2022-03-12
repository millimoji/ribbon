#include "pch.h"
#include "../compiler/LexiconReader.h"
#include "PhraseCounter.h"
namespace Ribbon { namespace Dictionary {
#ifdef _WINDOWS

struct PhraseCounter :
	public std::enable_shared_from_this<PhraseCounter>,
	public IPhraseCounter,
	public IObject
{
	int m_predictionMaxPerInput = 5;
	int m_cutOffLine;
	std::vector<WORDLINK*> m_fullPhraseList;

	PhraseCounter() {}
	virtual ~PhraseCounter() {}

	// IPhraseCounter
	void DoCount(WordContainer& wordContainer, int cutOffLine) override
	{
		m_cutOffLine = cutOffLine;

		for (auto& wordPrim : wordContainer)
		{
			TraceAllWordTree(wordPrim.second.get(), false, 1);

			TraceAllWordTree(&wordPrim.second->reverseInk, true, 1);
		}

		ApplyMinPhraseScoreByBackward();

		// sort full list
		std::sort(m_fullPhraseList.begin(),
			m_fullPhraseList.end(),
			[](const WORDLINK* lhs, const WORDLINK* rhs) {
			return lhs->discountedCount > rhs->discountedCount;
		});

		//m_fullPhraseList.resize(static_cast<size_t>(m_cutOffLine));

		OutputPredictionFile();
	}

	// private methods
	void TraceAllWordTree(WORDLINK *wordLink, bool isReversed, int nGram)
	{
		wordLink->predictionList.clear();

		for (auto& wordInfoPair : wordLink->rawNext)
		{
			TraceAllWordTree(&wordInfoPair.second, isReversed, nGram + 1);

			std::copy(wordInfoPair.second.predictionList.begin(),
				wordInfoPair.second.predictionList.end(),
				std::back_inserter(wordLink->predictionList));
		}

		std::sort(wordLink->predictionList.begin(),
			wordLink->predictionList.end(),
			[](const WORDLINK* lhs, const WORDLINK* rhs) {
				return lhs->discountedCount > rhs->discountedCount;
			});
		// cut off to max predictions per input
		if (wordLink->predictionList.size() > static_cast<size_t>(m_predictionMaxPerInput))
		{
			wordLink->predictionList.resize(static_cast<size_t>(m_predictionMaxPerInput));
		}

		// calculate sum of longer ngram.
		uint64_t countLongerUsage = 0;
		for (auto& countTailPair : wordLink->predictionList)
		{
			countLongerUsage += countTailPair->discountedCount;
		}

		// check if the last longer item is used more than shorter one.
		if (wordLink->rawCount > countLongerUsage)
		{
			if (wordLink->predictionList.size() < static_cast<size_t>(m_predictionMaxPerInput))
			{
				uint64_t adjustedThisCount = (uint64_t)std::max(0LL, (int64_t)wordLink->rawCount - (int64_t)countLongerUsage);

				wordLink->discountedCount = adjustedThisCount;
				wordLink->predictionList.push_back(wordLink);
				if (!isReversed)
				{
					m_fullPhraseList.push_back(wordLink);
				}
			}
			else if (wordLink->predictionList.back()->discountedCount /* used count of the lowest ranked item */
				<
				(wordLink->rawCount - countLongerUsage)) /* used count of the shorter item(this ngram) */
			{
				uint64_t adjustedThisCount = (uint64_t)std::max(0LL,
					(int64_t)wordLink->rawCount
					- (int64_t)countLongerUsage
					+ (int64_t)wordLink->predictionList.back()->discountedCount);

				// remove the last longer item, and add this self.
				wordLink->predictionList.resize(static_cast<size_t>(m_predictionMaxPerInput - 1));

				wordLink->discountedCount = adjustedThisCount;
				wordLink->predictionList.push_back(wordLink);
				if (!isReversed)
				{
					m_fullPhraseList.push_back(wordLink);
				}
			}
		}
	}

	void ApplyMinPhraseScoreByBackward()
	{
		for (auto& phraseItem : m_fullPhraseList)
		{
			std::vector<WORDLINK*> wordList;
			for (WORDLINK* cur = phraseItem; cur != nullptr; cur = cur->rawParentNode)
			{
				wordList.push_back(cur);
			}
			// wordList is already reversed order, look up reverse item
			WORDLINK *reverseEnd = &wordList.front()->rawWordPrim->reverseInk;
			for (int i = 1; i < (int)wordList.size(); ++i)
			{
				auto findRes = reverseEnd->rawNext.find(wordList[(size_t)i]->rawWordPrim);
				if (findRes == reverseEnd->rawNext.end())
				{
					reverseEnd = nullptr;
					phraseItem->discountedCount = 0ULL;
					break;
				}
				reverseEnd = &findRes->second;
			}
			if (reverseEnd != nullptr)
			{
				// apply small count as discountedCount for from both top end end
				phraseItem->discountedCount = std::min(phraseItem->discountedCount, reverseEnd->discountedCount);
			}
		}
	}

	void OutputPredictionFile()
	{
		std::shared_ptr<ILexiconReader> lexiconReader = FACTORYCREATE(LexiconReader);

		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		const auto& predictionFileName = setting->GetString("Dictionaries", "PhraseList");

		FILE *pf = nullptr;
		THROW_IF_FALSE(fopen_s(&pf, predictionFileName.c_str(), "wt, ccs=UNICODE") == 0);
		auto scopeExit = ScopeExit([&]() { fclose(pf); });
		
		int outputLine = 0;

		for (auto& phraseItem : m_fullPhraseList)
		{
			std::deque<WORDLINK*> wordList;
			for (WORDLINK* cur = phraseItem; cur != nullptr; cur = cur->rawParentNode)
			{
				wordList.push_front(cur);
			}
			if (phraseItem->discountedCount < 1)
			{
				continue;
			}
			if (wordList.size() == 7)
			{
				continue; // ignore 7 gram
			}

			if (wordList.size() > 1)
			{
				// count only multiple words
				if (outputLine > m_cutOffLine)
				{
					continue;
				}
				++outputLine;
			}

#if 0
if (wordList.size() == 1)
{
	continue;
}
#endif

			std::u16string textLine;
			std::u16string phraseText;
			for (WORDLINK* wordLink : wordList)
			{
				textLine += wordLink->rawWordPrim->rawDisplay.c_str();
				textLine += u'/';

				WORDINFO wordInfo;
				wordInfo.disp = wordLink->rawWordPrim->rawDisplay.c_str();
				wordInfo.read = wordLink->rawWordPrim->rawReading.c_str();
				wordInfo.pos = wordLink->rawWordPrim->rawPos->rawPosName.c_str();

				const auto& wordText = lexiconReader->WordInfoToString(&wordInfo);
				(phraseText += wordText) += u'\t';
			}

			textLine.pop_back();
			textLine += u'\t';
			textLine += phraseText;
			textLine += to_utf16(to_text(phraseItem->discountedCount));
			textLine += u'\n';

			fputws(reinterpret_cast<const wchar_t*>(textLine.c_str()), pf);
		}
	}

	IOBJECT_COMMON_METHODS
};
#else
struct PhraseCounter :
	public std::enable_shared_from_this<PhraseCounter>,
	public IPhraseCounter,
	public IObject
{
	IOBJECT_COMMON_METHODS
};
#endif // _WINDOWS
FACTORYDEFINE(PhraseCounter);
} /* namespace Dictionary */ } /* namespace Ribbon */
