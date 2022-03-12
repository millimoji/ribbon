#include "pch.h"
#include "../DictionaryFormat.h"
#include "LangModelCalc.h"
namespace Ribbon { namespace Dictionary {

struct LangModelCalculater :
	public std::enable_shared_from_this<LangModelCalculater>,
	public ILangModelCalculater,
	public ILexiconRegister,
	public IObject
{
	LangModelCalculater()
	{
		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		const char* dictionariesSection = "Dictionaries";
		m_katzThreshold = setting->GetInt(dictionariesSection, "KatzThreshold");

		{
			POSPRIM* posPrim = new POSPRIM;
			posPrim->rawPosName = TEXT_BOS;
			posPrim->rawPosID = POSID_BOS;
			posPrim->rawCount = 1LL;
			posPrim->allWordCount = 1LL;
			m_posContainer[posPrim->rawPosName.c_str()] = std::unique_ptr<POSPRIM>(posPrim);

			WORDPRIM *wordPrim = new WORDPRIM;
			wordPrim->rawReading = POSID_BOS_IN_WORDNGRAM;
			wordPrim->rawDisplay = POSID_BOS_IN_WORDNGRAM;
			wordPrim->rawPos = posPrim;
			wordPrim->rawCount = 1LL;
			wordPrim->rawParentNode = nullptr;

			WORDINFO wordInfoKey;
			wordInfoKey.read = wordPrim->rawReading.c_str();
			wordInfoKey.disp = wordPrim->rawDisplay.c_str();
			wordInfoKey.pos = wordPrim->rawPos->rawPosName.c_str();
			m_wordContainer[wordInfoKey] = std::unique_ptr<WORDPRIM>(wordPrim);
		}

		{
			POSPRIM* posPrim = new POSPRIM;
			posPrim->rawPosName = TEXT_EOS;
			posPrim->rawPosID = POSID_EOS;
			posPrim->rawCount = 1LL;
			posPrim->allWordCount = 1LL;
			m_posContainer[posPrim->rawPosName.c_str()] = std::unique_ptr<POSPRIM>(posPrim);

			WORDPRIM *wordPrim = new WORDPRIM;
			wordPrim->rawReading = POSID_EOS_IN_WORDNGRAM;
			wordPrim->rawDisplay = POSID_EOS_IN_WORDNGRAM;
			wordPrim->rawPos = posPrim;
			wordPrim->rawCount = 1LL;
			wordPrim->rawParentNode = nullptr;

			WORDINFO wordInfoKey;
			wordInfoKey.read = wordPrim->rawReading.c_str();
			wordInfoKey.disp = wordPrim->rawDisplay.c_str();
			wordInfoKey.pos = wordPrim->rawPos->rawPosName.c_str();
			m_wordContainer[wordInfoKey] = std::unique_ptr<WORDPRIM>(wordPrim);
		}
	}
	virtual ~LangModelCalculater() {}

	// ILexiconRegister
	void RegisterWord(const WORDINFO* wordInfo) override
	{
		auto resFind = m_wordContainer.find(*wordInfo);
		if (resFind == m_wordContainer.end())
		{
			POSPRIM *posPrim = GetOrRegisterPos(wordInfo->pos);
			posPrim->allWordCount++;	// count up for belonging word as all

			std::unique_ptr<WORDPRIM> wordPrim(new WORDPRIM);
			wordPrim->rawReading = wordInfo->read;
			wordPrim->rawDisplay = wordInfo->disp;
			wordPrim->rawPos = posPrim;
			wordPrim->rawCount = 0LL;
			wordPrim->rawParentNode = nullptr;

			WORDINFO wordInfoKey;
			wordInfoKey.read = wordPrim->rawReading.c_str();
			wordInfoKey.disp = wordPrim->rawDisplay.c_str();
			wordInfoKey.pos = wordPrim->rawPos->rawPosName.c_str();
	
			m_wordContainer[wordInfoKey] = std::move(wordPrim);
		}
	}

	void AllowUnknownWords(bool allowUnknownWords) override
	{
		m_allowUnknownWords = allowUnknownWords;
	}
	
	void BuildReverseLink(bool useReverseLink) override
	{
		m_useReverseLink = useReverseLink;
	}

	void SetNgram(int nGram, const WORDINFO* wordList, uint64_t count) override
	{
		WORDPRIM* wordPrims[10];
		for (int i = 0; i < nGram; ++i)
		{
			auto resFound = m_wordContainer.find(wordList[i]);
			if (resFound == m_wordContainer.end())
			{
				if (!m_allowUnknownWords)
				{
					return; // ignore this entry
				}
				RegisterWord(&wordList[i]);

				resFound = m_wordContainer.find(wordList[i]);

				if (resFound == m_wordContainer.end())
				{
					return; // ignore this entry
				}
			}
			wordPrims[i] = resFound->second.get();
		}

		switch (nGram)
		{
		case 1:
			SetUnigram(wordList, count);
			break;
		case 2:
			SetBigram(&wordList[0], &wordList[1], count);
			break;
		case 3:
			SetTrigram(&wordList[0], &wordList[1], &wordList[2], count);
			break;
		default:
			// Prediction Filter
			if (nGram >= 4)
			{
				std::map<WORDPRIM*, WORDLINK>* rawNextPtr = &wordPrims[0]->rawNext;
				WORDLINK* lastWordLink = nullptr;

				for (int i = 1; i < nGram; ++i)
				{
					WORDLINK wordLink;
					wordLink.rawCount = 0;
					wordLink.rawWordPrim = wordPrims[i];
					wordLink.rawParentNode = lastWordLink;
					auto insertRes = rawNextPtr->insert(std::make_pair(wordPrims[i], std::move(wordLink)));
					lastWordLink = &insertRes.first->second;
					rawNextPtr = &lastWordLink->rawNext;
				}
				lastWordLink->rawCount += count;
			}
			break;
		}
		if (m_useReverseLink && nGram > 1)
		{
			std::map<WORDPRIM*, WORDLINK>* rawNextPtr = &wordPrims[nGram - 1]->reverseInk.rawNext;
			WORDLINK* lastWordLink = nullptr;

			for (int i = nGram - 2; i >= 0; --i)
			{
				WORDLINK wordLink;
				wordLink.rawCount = 0;
				wordLink.rawWordPrim = wordPrims[i];
				wordLink.rawParentNode = lastWordLink;
				auto insertRes = rawNextPtr->insert(std::make_pair(wordPrims[i], std::move(wordLink)));
				lastWordLink = &insertRes.first->second;
				rawNextPtr = &lastWordLink->rawNext;
			}
			lastWordLink->rawCount += count;
		}
	}

	void SetPhrase(int nGram, const WORDINFO* wordList, uint64_t count) override
	{
		std::unique_ptr<PHRASEDATA> phraseData(new PHRASEDATA);
		phraseData->rawCount = count;

		for (int i = 0; i < nGram; ++i)
		{
			auto resFound = m_wordContainer.find(wordList[i]);
			if (resFound == m_wordContainer.end())
			{
				return; // ignore this entry
			}
			phraseData->rawWordList.push_back(resFound->second.get());
			phraseData->rawReading += resFound->second->rawReading;
		}

		m_phraseContainer.push_back(std::move(phraseData));
	}

	void SetUnigram(const WORDINFO* wordInfo, uint64_t count)
	{
		auto resFind = m_wordContainer.find(*wordInfo);
		if (resFind != m_wordContainer.end())
		{
			WORDPRIM* wordPrim(resFind->second.get());
			wordPrim->rawCount += count;
			wordPrim->rawPos->rawCount += count;
			wordPrim->rawPos->unigramWordCount++;	// count up for belonging word in corpus
			wordPrim->reverseInk.rawCount = wordPrim->rawCount;
		}
		else
		{
			Platform->Error("Failed to add unigram: %s/%s/%s\n",
				to_utf8(wordInfo->disp).c_str(), to_utf8(wordInfo->read).c_str(), to_utf8(wordInfo->pos).c_str());
		}
	}
	void SetBigram(const WORDINFO* word1Info, const WORDINFO* word2Info, uint64_t count)
	{
		auto resFind1 = m_wordContainer.find(*word1Info);
		auto resFind2 = m_wordContainer.find(*word2Info);
		if (resFind1 == m_wordContainer.end() || resFind2 == m_wordContainer.end())
		{
			Platform->Error("Failed to add bigram: %s/%s/%s - %s/%s/%s\n",
				to_utf8(word1Info->disp).c_str(), to_utf8(word1Info->read).c_str(), to_utf8(word1Info->pos).c_str(),
				to_utf8(word2Info->disp).c_str(), to_utf8(word2Info->read).c_str(), to_utf8(word2Info->pos).c_str());
		}

		WORDPRIM *wordPrim1(resFind1->second.get());
		WORDPRIM *wordPrim2(resFind2->second.get());

		// set POS bigram
		SetPosBigram(wordPrim1->rawPos, wordPrim2->rawPos, count);

		if (count > static_cast<uint32_t>(m_katzThreshold))
		{
			wordPrim2->isNgramTail = true;

			WORDLINK wordLink;
			wordLink.rawCount = count;
			wordLink.rawWordPrim = wordPrim2;
			wordLink.rawParentNode = wordPrim1;

			auto insRslt = wordPrim1->rawNext.insert(std::make_pair(wordPrim2, std::move(wordLink)));
			if (!insRslt.second)
			{
				insRslt.first->second.rawCount += count;
			}
		}
	}
	void SetTrigram(const WORDINFO* word1Info, const WORDINFO* word2Info, const WORDINFO* word3Info, uint64_t count)
	{
		auto resFind1 = m_wordContainer.find(*word1Info);
		auto resFind2 = m_wordContainer.find(*word2Info);
		auto resFind3 = m_wordContainer.find(*word3Info);

		if (resFind1 == m_wordContainer.end() || resFind2 == m_wordContainer.end() || resFind3 == m_wordContainer.end())
		{
			Platform->Error("Failed to add trigram: %s/%s/%s - %s/%s/%s - %s/%s/%s\n",
				to_utf8(word1Info->disp).c_str(), to_utf8(word1Info->read).c_str(), to_utf8(word1Info->pos).c_str(),
				to_utf8(word2Info->disp).c_str(), to_utf8(word2Info->read).c_str(), to_utf8(word2Info->pos).c_str(),
				to_utf8(word3Info->disp).c_str(), to_utf8(word3Info->read).c_str(), to_utf8(word3Info->pos).c_str());
		}

		WORDPRIM *wordPrim1(resFind1->second.get());
		WORDPRIM *wordPrim2(resFind2->second.get());
		WORDPRIM *wordPrim3(resFind3->second.get());

		// set POS trigram
		SetPosTrigram(wordPrim1->rawPos, wordPrim2->rawPos, wordPrim3->rawPos, count);

		wordPrim3->isNgramTail = true;

		// 
		auto findRslt = wordPrim1->rawNext.find(wordPrim2);
		if (findRslt == wordPrim1->rawNext.end())
		{
			Platform->Error("no word bigram for pos trigram, %s, %s, %s\n",
				wordPrim1->rawDisplay.c_str(),
				wordPrim2->rawDisplay.c_str(),
				wordPrim3->rawDisplay.c_str());
			return;
		}
		WORDLINK *wordBigram = &findRslt->second;

		WORDLINK wordLink;
		wordLink.rawCount = count;
		wordLink.rawWordPrim = wordPrim3;
		wordLink.rawParentNode = wordBigram;

		auto insRslt = wordBigram->rawNext.insert(std::make_pair(wordPrim3, std::move(wordLink)));
		if (!insRslt.second)
		{
			insRslt.first->second.rawCount += count;
		}
	}

	// ILangModelCalculater
	void CalculateNgramProbability(WordContainer& allWords, PosContainer& allPos, PhraseContainer& allPhrases) override
	{
		Platform->Printf("Calc Pos Inside\n");
		CalculatePosInsideUnigram();
		Platform->Printf("Calc Pos Bigram\n");
		CalculatePosBigram();
		Platform->Printf("Calc Word Bigram\n");
		CalculateWordBigram();
		Platform->Printf("Calc Pos Trigram\n");
		CalculatePosTrigram();
		Platform->Printf("Calc Word Trigram\n");
		CalculateWordTrigram();
		Platform->Printf("Calc Phrase Unigram\n");
		CalculatePhraseUnigram();

		Platform->Printf("POS:%d, POS-Bi:%d, POS-Tri:%d, WORD:%d, WORD-Bi:%d, WORD-Tri:%d\n",
			(int)m_posContainer.size(), (int)m_posBigramCount, (int)m_posTrigramCount,
			(int)m_wordContainer.size(), (int)m_wordBigramCount, (int)m_wordTrigramCount);

		allPos.swap(m_posContainer);
		allWords.swap(m_wordContainer);
		allPhrases.swap(m_phraseContainer);
	}

	void GetWordContainer(WordContainer& allWords) override
	{
		allWords.swap(m_wordContainer);
	}

private:
	//
	int m_nextPosID = POSID_BEGIN;
	int m_katzThreshold = 10;
	long m_posBigramCount = 0;
	long m_posTrigramCount = 0;
	long m_wordBigramCount = 0;
	long m_wordTrigramCount = 0;
	bool m_useReverseLink = false;
	bool m_allowUnknownWords = false;

	PosContainer m_posContainer;
	WordContainer m_wordContainer;
	PhraseContainer m_phraseContainer;

	POSPRIM* GetOrRegisterPos(const char16_t* pos)
	{
		auto resFind = m_posContainer.find(pos);
		if (resFind != m_posContainer.end())
		{
			return resFind->second.get();
		}
		POSPRIM* posPrim = new POSPRIM;
		posPrim->rawPosName = pos;
		if ((m_nextPosID & 0xFF) == 0) m_nextPosID++;
		posPrim->rawPosID = m_nextPosID++;
		posPrim->rawCount = 0LL;

		m_posContainer[posPrim->rawPosName.c_str()] = std::unique_ptr<POSPRIM>(posPrim);
		return posPrim;
	}
	void SetPosBigram(POSPRIM *posPrim1, POSPRIM *posPrim2, uint64_t count)
	{
		posPrim2->isNgramTail = true;

		POSLINK posLink;
		posLink.rawCount = count;
		posLink.rawPosPrim = posPrim2;

		auto insRslt = posPrim1->rawNext.insert(std::make_pair(posPrim2, std::move(posLink)));

		if (!insRslt.second)
		{
			if (insRslt.first == posPrim1->rawNext.end())
			{
				THROW_MSG("Adding POS bigram failed.");
			}
			insRslt.first->second.rawCount += count;
		}
	}
	void SetPosTrigram(POSPRIM *posPrim1, POSPRIM *posPrim2, POSPRIM *posPrim3, uint64_t count)
	{
		posPrim3->isNgramTail = true;

		auto findRslt = posPrim1->rawNext.find(posPrim2);
		if (findRslt == posPrim1->rawNext.end())
		{
			Platform->Error("no pos bigram for pos trigram, %s, %s, %s\n",
				to_utf8(posPrim1->rawPosName).c_str(), to_utf8(posPrim2->rawPosName).c_str(), to_utf8(posPrim3->rawPosName).c_str());
			return;
		}
		POSLINK *posBigram = &findRslt->second;

		POSLINK posLink;
		posLink.rawCount = count;
		posLink.rawPosPrim = posPrim3;

		auto insRslt = posBigram->rawNext.insert(std::make_pair(posPrim3, std::move(posLink)));
		if (!insRslt.second)
		{
			if (insRslt.first == posBigram->rawNext.end())
			{
				THROW_MSG("Adding POS trigram failed.");
			}
			insRslt.first->second.rawCount += count;
		}
	}

	void CalculatePosInsideUnigram()
	{
		for (auto it = m_wordContainer.begin(); it != m_wordContainer.end(); ++it)
		{
			WORDPRIM *wordPrim(it->second.get());
			POSPRIM *posPrim(it->second->rawPos);

			double unigramCount = (double)(posPrim->rawCount + (posPrim->allWordCount > posPrim->unigramWordCount ? 1 : 0));
			double unseenCount = (double)(posPrim->allWordCount - posPrim->unigramWordCount);
			if (wordPrim->rawCount == 0)
			{
				wordPrim->probPosInside = 1.0 / unigramCount / unseenCount;
			}
			else
			{
				wordPrim->probPosInside = (double)wordPrim->rawCount / unigramCount;
			}
		}
	}
	void CalculatePosBigram()
	{
		for (auto itLhs = m_posContainer.begin(); itLhs != m_posContainer.end(); ++itLhs)
		{
			POSPRIM *posPrim(itLhs->second.get());

			double unigramCount = (double)std::max(posPrim->rawCount, 1ULL);
			for (auto itRhs = posPrim->rawNext.begin(); itRhs != posPrim->rawNext.end(); ++itRhs)
			{
				itRhs->second.probNgram = (double)itRhs->second.rawCount / unigramCount;
				THROW_IF_FALSE(itRhs->second.probNgram <= 1.0);
				m_posBigramCount++;
			}
		}
	}
	void CalculatePosTrigram()
	{
		for (auto itLhs = m_posContainer.begin(); itLhs != m_posContainer.end(); ++itLhs)
		{
			POSPRIM *posPrimLhs(itLhs->second.get());

			for (auto itMid = posPrimLhs->rawNext.begin(); itMid != posPrimLhs->rawNext.end(); ++itMid)
			{
				POSLINK *posLinkMid(&itMid->second);
				POSPRIM *posPrimMid(posLinkMid->rawPosPrim);

				double sumOfTrigram = 0.0;
				double sumOfBigram = 0.0;

				for (auto itRhs = posLinkMid->rawNext.begin(); itRhs != posLinkMid->rawNext.end(); ++itRhs)
				{
					POSLINK *posLinkRhs(&itRhs->second);
					POSPRIM *posPrimRhs(posLinkRhs->rawPosPrim);

					if (posLinkRhs->rawCount < static_cast<uint64_t>(m_katzThreshold))
					{
						continue;
					}

					posLinkRhs->probNgram = (double)posLinkRhs->rawCount / (double)posLinkMid->rawCount;
					sumOfTrigram += posLinkRhs->probNgram;

					auto itBigram = posPrimMid->rawNext.find(posPrimRhs);
					sumOfBigram += itBigram->second.probNgram;

					m_posTrigramCount++;
				}

				posLinkMid->probBackOff = 1.0;
				if (sumOfBigram < 1.0 && sumOfTrigram < 1.0)
				{
					posLinkMid->probBackOff = (1.0 - sumOfTrigram) / (1.0 - sumOfBigram);
					posLinkMid->probBackOff = std::min(std::max(posLinkMid->probBackOff, (double)FLT_MIN), (double)FLT_MAX);
				}
			}
		}
	}
	void CalculateWordBigram()
	{
		for (auto itLhs = m_wordContainer.begin(); itLhs != m_wordContainer.end(); ++itLhs)
		{
			WORDPRIM *wordLhs(itLhs->second.get());
			if (wordLhs->rawNext.size() == 0)
			{
				continue;
			}

			POSPRIM *posLhs(itLhs->second->rawPos);
			double sumOfBigram = 0.0;
			double sumOfPosBigram = 0.0;

			for (auto itRhs = wordLhs->rawNext.begin(); itRhs != wordLhs->rawNext.end(); ++itRhs)
			{
				WORDLINK *wordLink(&itRhs->second);
				WORDPRIM *wordRhs(wordLink->rawWordPrim);

				POSLINK *posLink = &(posLhs->rawNext.find(wordRhs->rawPos)->second);
				//POSPRIM *posRhs(posLink->rawPosPrim);

				wordLink->probNgram = (double)wordLink->rawCount / (double)wordLhs->rawCount;
				THROW_IF_FALSE(wordLink->probNgram <= 1.0);

				sumOfBigram += wordLink->probNgram;
				sumOfPosBigram += posLink->probNgram * wordRhs->probPosInside;

				m_wordBigramCount++;
			}

			wordLhs->probPosBackOff = 1.0;
			if (sumOfBigram < 1.0 && sumOfPosBigram < 1.0)
			{
				wordLhs->probPosBackOff = (1.0 - sumOfBigram) / (1.0 - sumOfPosBigram);
				wordLhs->probPosBackOff = std::min(std::max(wordLhs->probPosBackOff, (double)FLT_MIN), (double)FLT_MAX);
			}
		}
	}
	void CalculateWordTrigram()
	{
		for (auto itLhs = m_wordContainer.begin(); itLhs != m_wordContainer.end(); ++itLhs)
		{
			WORDPRIM *wordPrimLhs(itLhs->second.get());

			for (auto itMid = wordPrimLhs->rawNext.begin(); itMid != wordPrimLhs->rawNext.end(); ++itMid)
			{
				WORDLINK *wordLinkMid(&itMid->second);
				WORDPRIM *wordPrimMid(wordLinkMid->rawWordPrim);

				double sumOfTrigram = 0.0;
				double sumOfBigram = 0.0;

				for (auto itRhs = wordLinkMid->rawNext.begin(); itRhs != wordLinkMid->rawNext.end(); ++itRhs)
				{
					WORDLINK *wordLinkRhs(&itRhs->second);
					WORDPRIM *wordPrimRhs(wordLinkRhs->rawWordPrim);

					auto itBigram = wordPrimMid->rawNext.find(wordPrimRhs);
					if (itBigram == wordPrimMid->rawNext.end())
					{
						// ignore this trigram entry
						Platform->Printf("no suffix bigram for trigram %s/%s/%s\n",
							to_utf8(itLhs->second->rawDisplay).c_str(),
							to_utf8(itMid->first->rawDisplay).c_str(),
							to_utf8(itRhs->first->rawDisplay).c_str());
						continue;
					}

					wordLinkRhs->probNgram = (double)wordLinkRhs->rawCount / (double)wordLinkMid->rawCount;
					sumOfTrigram += wordLinkRhs->probNgram;
					sumOfBigram += itBigram->second.probNgram;

					m_wordTrigramCount++;
				}

				wordLinkMid->probBackOff = 1.0;
				if (sumOfBigram < 1.0 && sumOfTrigram < 1.0)
				{
					wordLinkMid->probBackOff = (1.0 - sumOfTrigram) / (1.0 - sumOfBigram);
					wordLinkMid->probBackOff = std::min(std::max(wordLinkMid->probBackOff, (double)FLT_MIN), (double)FLT_MAX);
				}
			}
		}
	}
	void CalculatePhraseUnigram()
	{
		for (auto& phraseItem : m_phraseContainer)
		{
			phraseItem->probPosInside = (double)phraseItem->rawCount / (double)phraseItem->rawWordList[0]->rawPos->rawCount;
		}
	}

	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(LangModelCalculater);
} /* namespace Dictionary */ } /* namespace Ribbon */
