#include "pch.h"
#include "../DictionaryFormat.h"
#include "../DictionaryReader.h"
#include "DictionaryCompiler.h"
#include "DictionaryConstructor.h"
#include "DoubleArray.h"
#include "LinearKmeans.h"
namespace Ribbon { namespace Dictionary {

inline size_t RoundUp(size_t orgSize, size_t roundSize)
{
	return  (orgSize + roundSize - 1) / roundSize * roundSize;
}

namespace VocabHelper {
	class TailString {
	public:
		static size_t Size(size_t current, const uint8_t* tail)
		{
			size_t tailLen = strlen((const char*)tail);
			return current + sizeof(VOCAB_TAILSTRING) + tailLen - 1;
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, const uint8_t* tail)
		{
			VOCAB_TAILSTRING* self = (VOCAB_TAILSTRING*)base;
			int tailLen;
			for (tailLen = 0; tail[tailLen] != 0; ++tailLen)
			{
				self->tailString[tailLen] = tail[tailLen];
			}
			THROW_IF_FALSE(tailLen > 0 && tailLen <= VOCAB_TAILSTRING::maxTailLen);
			self->id = VOCAB_ID_TAILSTRING;
			self->length = (uint8_t)(tailLen - 1);
			return (VOCAB_BASE*)&self->tailString[self->length + 1];
		}
	};
	class Word {
	public:
		static size_t Size(size_t current, std::vector<WORDPRIM*> wordList, int readingNode, ILinearKmeans*, ILinearKmeans*, ILinearKmeans*)
		{
			for (auto wordPrim : wordList)
			{
				wordPrim->memoryOffset = (int)current;
				wordPrim->readingNode = readingNode;

				bool hasBigram = wordPrim->bigramOffset != 0;
				bool hasPredScore = wordPrim->isPrediciton;

				current += sizeof(VOCAB_WORD);
				current += 2; // POS
				current += (hasBigram ? (1 + 3 + 3) : 0);
				current += hasPredScore ? 1 : 0;
			}
			return current;
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, std::vector<WORDPRIM*> wordList, int readingNode,
			ILinearKmeans* logPosInside, ILinearKmeans* logBackOff, ILinearKmeans* logPosPhrase)
		{
			for (auto wordPrim : wordList)
			{
				bool hasBigram = wordPrim->bigramOffset != 0;
				bool hasPredScore = wordPrim->isPrediciton;

				VOCAB_WORD* vocabWord = reinterpret_cast<VOCAB_WORD*>(base);
				vocabWord->id = VOCAB_ID_WORD;
				vocabWord->hasPos = true;
				vocabWord->isBigramHead = hasBigram ? 1u : 0u;
				vocabWord->isNgramTail = wordPrim->isNgramTail ? 1u : 0u;
				vocabWord->hasPhraseScore = wordPrim->isPrediciton ? 1u : 0u;
				vocabWord->_free = 0;

				int displayNode = wordPrim->displayNode;
				if (wordPrim->rawDisplay == wordPrim->rawReading)
				{
					displayNode = readingNode;
				}
				Write24(vocabWord->display, displayNode);
				vocabWord->posInside = logPosInside->GetIndex(wordPrim->probPosInside);

				uint8_t* writePtr = reinterpret_cast<uint8_t*>(vocabWord + 1);
				Write16(writePtr, wordPrim->rawPos->rawPosID);
				writePtr += sizeof(uint8_t[2]);

				if (hasBigram)
				{
					*writePtr = logBackOff->GetIndex(wordPrim->probBackOff);
					++writePtr;
					Write24(writePtr, wordPrim->bigramOffset);
					writePtr += sizeof(uint8_t[3]);
					Write24(writePtr, (int)wordPrim->rawNext.size());
					writePtr += sizeof(uint8_t[3]);
				}
				if (hasPredScore)
				{
					*writePtr = logPosPhrase->GetIndex(wordPrim->probPredictionScore);
					++writePtr;
				}
				base = reinterpret_cast<VOCAB_BASE*>(writePtr);
			}
			return base;
		}
	};
	class PosAsWord {
	public:
		static size_t Size(size_t current, POSPRIM* posPrim, int /*readingNode*/)
		{
			posPrim->memoryOffset = (int)current;

			current += sizeof(VOCAB_WORD);
			current += 2; // pos

			if (posPrim->rawNext.size() > 0)
			{
				current += (1 + 3 + 3);
			}
			return current;
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, POSPRIM* posPrim, int readingNode)
		{
			VOCAB_WORD* vocabWord = reinterpret_cast<VOCAB_WORD*>(base);
			vocabWord->id = VOCAB_ID_WORD;
			vocabWord->hasPos = true;
			vocabWord->isBigramHead = (posPrim->rawNext.size() > 0) ? 1u : 0u;
			vocabWord->isNgramTail = true;
			vocabWord->hasPhraseScore = false;
			vocabWord->_free = 0;

			Write24(vocabWord->display, readingNode);
			vocabWord->posInside = 0;

			uint8_t* writePtr = reinterpret_cast<uint8_t*>(vocabWord + 1);
			Write16(writePtr, posPrim->rawPosID);
			writePtr += sizeof(uint8_t[2]);

			if (posPrim->rawNext.size() > 0)
			{
				*writePtr = 0; // posBackOff
				++writePtr;
				Write24(writePtr, posPrim->bigramOffset);
				writePtr += sizeof(uint8_t[3]);
				Write24(writePtr, (int)posPrim->rawNext.size());
				writePtr += sizeof(uint8_t[3]);
			}

			return reinterpret_cast<VOCAB_BASE*>(writePtr);
		}
	};
	class Display {
	public:
		static size_t Size(size_t current, std::vector<WORDPRIM*> words, int displayNode)
		{
			int countOfWords = (int)words.size();
			int countOfFullBlocks = countOfWords / maxWordsInVocab;
			int countOfLastBlock = countOfWords % maxWordsInVocab;

			for (auto wordPrim : words)
			{
				wordPrim->displayNode = displayNode;
			}

			return current + countOfFullBlocks * (sizeof(VOCAB_BASE) + sizeof(uint8_t[3]) * maxWordsInVocab)
				+ (countOfLastBlock > 0 ? (sizeof(VOCAB_BASE) + sizeof(uint8_t[3]) * countOfLastBlock) : 0);
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, std::vector<WORDPRIM*> words, int /*displayNode*/)
		{
			VOCAB_DISPLAY* vocabDisp = nullptr;

			for (size_t i = 0; i < words.size(); ++i)
			{
				if ((i % maxWordsInVocab) == 0)
				{
					vocabDisp = (VOCAB_DISPLAY*)(i == 0 ? base : (VOCAB_BASE*)&vocabDisp->wordIndex[vocabDisp->count + 1]);
					vocabDisp->id = VOCAB_ID_DISPLAY;
					vocabDisp->count = 0x1F; // 5 bits
				}
				Write24(vocabDisp->wordIndex[i % maxWordsInVocab], (int)words[i]->memoryOffset);
				vocabDisp->count++;
			}
			return (VOCAB_BASE*)(vocabDisp->wordIndex[vocabDisp->count + 1]);
		}
	};
	class Phrase {
	public:
		static size_t Size(size_t current, std::vector<PHRASEDATA*> phrases, ILinearKmeans*)
		{
			for (auto& phraseItem : phrases)
			{
				phraseItem->memoryOffset = (int)current;
				int countOfWords = (int)phraseItem->rawWordList.size();
				THROW_IF_FALSE(countOfWords <= maxWordsInVocab);
				current += sizeof(VOCAB_WORDARRAY) + sizeof(uint8_t[3]) * (countOfWords - 1);
			}
			return current;
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, std::vector<PHRASEDATA*>& phrases, ILinearKmeans* logPosPhrase)
		{
			for (auto& phraseItem : phrases)
			{
				VOCAB_WORDARRAY* wordArray = reinterpret_cast<VOCAB_WORDARRAY*>(base);
				wordArray->id = VOCAB_ID_WORDARRAY;
				wordArray->count = phraseItem->rawWordList.size() - 1;
				wordArray->posInside = logPosPhrase->GetIndex(phraseItem->probPosInside);

				for (size_t i = 0; i < phraseItem->rawWordList.size(); ++i)
				{
					Write24(wordArray->wordIndex[i], (int)phraseItem->rawWordList[i]->memoryOffset);
				}
				base = reinterpret_cast<VOCAB_BASE*>(wordArray->wordIndex[phraseItem->rawWordList.size()]);
			}
			return base;
		}
	};
	class PredCache {
	public:
		static size_t Size(size_t current, PREDICTIONCACHE* predCache)
		{
			return current + sizeof(VOCAB_PREDICTIONCACHE) + sizeof(uint8_t[3]) * (predCache->predictionItems.size() - 1);
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, PREDICTIONCACHE* predCache)
		{
			VOCAB_PREDICTIONCACHE* vocabPredCache = reinterpret_cast<VOCAB_PREDICTIONCACHE*>(base);
			vocabPredCache->id = VOCAB_ID_PREDICTIONCACHE;
			vocabPredCache->count = predCache->predictionItems.size() - 1;

			for (size_t i = 0; i < predCache->predictionItems.size(); ++i)
			{
				int targetOffset = 0;
				if (predCache->predictionItems[i]->rawWordList.size() == 1)
				{
					targetOffset = predCache->predictionItems[i]->rawWordList[0]->memoryOffset;
				}
				else
				{
					targetOffset = predCache->predictionItems[i]->memoryOffset;
				}
				Write24(vocabPredCache->wordIndex[i], targetOffset);
			}
			return reinterpret_cast<VOCAB_BASE*>(&vocabPredCache->wordIndex[predCache->predictionItems.size()]);
		}
	};
	class Termination {
	public:
		static size_t Size(size_t current, int readingIndex, int bigramEnd)
		{
			size_t mySize = sizeof(VOCAB_TERMINATION);
			if (readingIndex >= 0) mySize += sizeof(uint8_t[3]);
			if (bigramEnd >= 0) mySize += sizeof(uint8_t[3]);
			return current + mySize;
		}
		static VOCAB_BASE* Make(VOCAB_BASE* base, int readingIndex, int bigramEnd)
		{
			VOCAB_TERMINATION* vocabTerm = (VOCAB_TERMINATION*)base;
			vocabTerm->hasReadingIdx = (uint8_t)(readingIndex >= 0);
			vocabTerm->hasBigramEnd = (uint8_t)(bigramEnd >= 0);
			vocabTerm->_free = 0;
			vocabTerm->id = VOCAB_ID_TERMINATION;

			uint8_t* myEnd = (uint8_t*)(vocabTerm + 1);
			if (readingIndex >= 0)
			{
				Write24(myEnd, readingIndex);
				myEnd += 3;
			}
			if (bigramEnd >= 0)
			{
				Write24(myEnd, bigramEnd);
				myEnd += 3;
			}
			return (VOCAB_BASE*)myEnd;
		}
	};
}
namespace NgramHelper {
	class PosAsWord
	{
	public:
		static size_t Size(size_t current, POSPRIM* posPrim, ILinearKmeans*)
		{
			if (posPrim->rawNext.size() == 0)
			{
				posPrim->bigramOffset = (int)-1;
				return current;
			}
			posPrim->bigramOffset = (int)current;
			return current + posPrim->rawNext.size();
		}
		static void Make(std::vector<BIGRAM_ENTRY>& bigramBuf, POSPRIM* posPrim, ILinearKmeans* logPosBigram)
		{
			if (posPrim->rawNext.size() == 0)
			{
				return;
			}

			THROW_IF_FALSE((int)bigramBuf.size() == posPrim->bigramOffset);

			for (auto& posBigram : posPrim->rawNext)
			{
				bigramBuf.emplace_back(BIGRAM_ENTRY());
				bigramBuf.back().wordId = (uint32_t)posBigram.first->memoryOffset;
				bigramBuf.back().bigram = logPosBigram->GetIndex(posBigram.second.probNgram);
			}

			std::sort(bigramBuf.begin() + posPrim->bigramOffset, bigramBuf.end(),
				[](const BIGRAM_ENTRY& lhs, const BIGRAM_ENTRY& rhs)
				{
					return lhs.wordId < rhs.wordId;
				});
		}
	};
	class Word
	{
	public:
		static size_t Size(size_t current, std::vector<WORDPRIM*> words, ILinearKmeans*)
		{
			for (auto wordPrim : words)
			{
				if (wordPrim->rawNext.size() == 0)
				{
					wordPrim->bigramOffset = (int)-1;
					continue;
				}
				wordPrim->bigramOffset = (int)current;
				current += wordPrim->rawNext.size();
			}
			return current;
		}
		static void Make(std::vector<BIGRAM_ENTRY>& bigramBuf, std::vector<WORDPRIM*> words, ILinearKmeans* logBigram)
		{
			for (auto wordPrim : words)
			{
				if (wordPrim->rawNext.size() == 0)
				{
					continue;
				}

				THROW_IF_FALSE((int)bigramBuf.size() == wordPrim->bigramOffset);

				for (auto& wordBigram : wordPrim->rawNext)
				{
					bigramBuf.emplace_back(BIGRAM_ENTRY());
					bigramBuf.back().wordId = (uint32_t)wordBigram.first->memoryOffset;
					bigramBuf.back().bigram = logBigram->GetIndex(wordBigram.second.probNgram);
				}

				std::sort(bigramBuf.begin() + wordPrim->bigramOffset, bigramBuf.end(),
					[](const BIGRAM_ENTRY& lhs, const BIGRAM_ENTRY& rhs)
				{
					return lhs.wordId < rhs.wordId;
				});
			}
		}
	};
}


struct DictionaryConstructor :
	public std::enable_shared_from_this<DictionaryConstructor>,
	public IDictionaryConstructor,
	public IObject
{
	void BuildDictionary(WordContainer& allWords, PosContainer& allPos, PhraseContainer& allPhrases, PredCacheContainer& allPredCache) override
	{
		m_allWords.swap(allWords);
		m_allPos.swap(allPos);
		m_allPhrases.swap(allPhrases);
		m_allPredCache.swap(allPredCache);

		m_wArrayIndex = FACTORYCREATE(WArrayMaker);
		m_logPosInside = FACTORYCREATE(LinearKmeans);
		m_logPosPhrase = FACTORYCREATE(LinearKmeans);
		m_logPosBigram = FACTORYCREATE(LinearKmeans);
		m_logPosTrigram = FACTORYCREATE(LinearKmeans);
		m_logWordBigram = FACTORYCREATE(LinearKmeans);
		m_logWordTrigram = FACTORYCREATE(LinearKmeans);
		m_logBackOff = FACTORYCREATE(LinearKmeans);

		Platform->Printf("Creating Index\n");
		CreateLetterGroupForPos();
		CreateLetterGroupForWord();
		CreateLetterGroupForPredCache();
		CreateLetterGroupForPhrase();

#if 0
		m_logBackOff.SaveSource(u"D:\\Text\\m_logBackOff.bin");
		m_logPosBigram.SaveSource(u"D:\\Text\\m_logPosBigram..bin");
		m_logPosTrigram.SaveSource(u"D:\\Text\\m_logPosTrigram.bin");
		m_logPosInside.SaveSource(u"D:\\Text\\m_logPosInside.bin");
		m_logWordBigram.SaveSource(u"D:\\Text\\m_logWordBigram.bin");
		m_logWordTrigram.SaveSource(u"D:\\Text\\m_logWordTrigram.bin");
#endif

		std::vector<std::shared_ptr<AsyncTask<bool>>> concurrentTasks;
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			CreateIndexPartList();
			m_wArrayIndex->BuildFlat(m_indexPartList);
			Ribbon::Platform->Printf("Created WAray Index\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logBackOff->Calculate();
			Ribbon::Platform->Printf("Backoff K-means\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logPosBigram->Calculate();
			Ribbon::Platform->Printf("Pos Bigram K-means\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logPosTrigram->Calculate();
			Ribbon::Platform->Printf("Pos Trigram K-means\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logPosInside->Calculate();
			Ribbon::Platform->Printf("Pos Inside K-means\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logPosPhrase->Calculate();
			Ribbon::Platform->Printf("Phrase Pos Inside K-means\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logWordBigram->Calculate();
			Ribbon::Platform->Printf("Word Bigram K-means\n"); return true;
		}));
		concurrentTasks.push_back(GlobalThreadPool.Request<bool>([&]() {
			m_logWordTrigram->Calculate();
			Ribbon::Platform->Printf("Word Trigram K-means\n"); return true;
		}));
		for (auto& task : concurrentTasks) { task->GetResult(); }

		Ribbon::Platform->Printf("Making Boday / n-Gram\n");
		BuildVocabBodyPart();

		Platform->Printf("Writing system dictionary\n");
		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		std::string outputName = setting->GetExpandedString("Dictionaries", "SystemDictionary");
		WriteOutSystemDictionary(outputName.c_str());

		// TODO: clean up
		m_allWords.swap(allWords);
		m_allPos.swap(allPos);
	}

	//private:
	WordContainer m_allWords;
	PosContainer m_allPos;
	PhraseContainer m_allPhrases;
	PredCacheContainer m_allPredCache;
	std::map<const uint8_t*, std::unique_ptr<LETTERGROUP>, textCompare<uint8_t>> m_allLetters;
	std::vector<const uint8_t*> m_indexPartList;
	std::vector<LETTERGROUP*> m_letterGroupList;
	std::shared_ptr<IWArrayMaker> m_wArrayIndex;
	std::shared_ptr<ILinearKmeans> m_logPosInside;
	std::shared_ptr<ILinearKmeans> m_logPosPhrase;
	std::shared_ptr<ILinearKmeans> m_logPosBigram;
	std::shared_ptr<ILinearKmeans> m_logPosTrigram;
	std::shared_ptr<ILinearKmeans> m_logWordBigram;
	std::shared_ptr<ILinearKmeans> m_logWordTrigram;
	std::shared_ptr<ILinearKmeans> m_logBackOff;
	std::vector<uint8_t> m_vocabPart;
	size_t m_totalBigram = 0;
	std::vector<BIGRAM_ENTRY> m_bigramBlock;

	void CreateLetterGroupForPos()
	{
		for (auto it = m_allPos.begin(); it != m_allPos.end(); ++it)
		{
			POSPRIM* posPrim(it->second.get());

			char16_t wchBuf[] = { PREFIX_POS_IN_POSNGRAM, (char16_t)it->second->rawPosID, 0 };
			LETTERGROUP* letterGroup = GetOrCreateLetterGroup(wchBuf);
			letterGroup->orgLetter = nullptr; // because this is on stack
			letterGroup->posList.push_back(posPrim);

			// setup n-gram value table
			for (auto itPos = posPrim->rawNext.begin(); itPos != posPrim->rawNext.end(); ++itPos)
			{
				m_logPosBigram->AddValue(itPos->second.probNgram);

				if (itPos->second.rawNext.size() > 0)
				{
					m_logBackOff->AddValue(itPos->second.probBackOff);
				}
				for (auto itTri = itPos->second.rawNext.begin(); itTri != itPos->second.rawNext.end(); ++itTri)
				{
					m_logPosTrigram->AddValue(itTri->second.probNgram);
				}
			}
		}
	}

	void CreateLetterGroupForWord()
	{
		for (auto it = m_allWords.begin(); it != m_allWords.end(); ++it)
		{
			WORDPRIM* wordPrim(it->second.get());

			LETTERGROUP* letterGroup = GetOrCreateLetterGroup(wordPrim->rawReading.c_str());
			letterGroup->readList.push_back(wordPrim);

			if (wordPrim->rawReading != wordPrim->rawDisplay)
			{
				LETTERGROUP* dispLetterGroup = GetOrCreateLetterGroup(wordPrim->rawDisplay.c_str());
				dispLetterGroup->dispList.push_back(wordPrim);
			}

			// setup n-gram value table
			m_logPosInside->AddValue(wordPrim->probPosInside);
			if (wordPrim->rawNext.size() > 0)
			{
				m_logBackOff->AddValue(wordPrim->probPosBackOff);
			}
			for (auto itBi = wordPrim->rawNext.begin(); itBi != wordPrim->rawNext.end(); ++itBi)
			{
				m_logWordBigram->AddValue(itBi->second.probNgram);

				if (itBi->second.rawNext.size() > 0)
				{
					m_logBackOff->AddValue(itBi->second.probBackOff);
				}
				for (auto itTri = itBi->second.rawNext.begin(); itTri != itBi->second.rawNext.end(); ++itTri)
				{
					m_logWordTrigram->AddValue(itTri->second.probNgram);
				}
			}
		}
	}

	void CreateLetterGroupForPredCache()
	{
		for (auto& predCache : m_allPredCache)
		{
			PREDICTIONCACHE* predCahe = predCache.get();

			LETTERGROUP* letterGroup = GetOrCreateLetterGroup(predCahe->rawReading.c_str());
			letterGroup->predCacheList.push_back(predCahe);
		}
	}

	void CreateLetterGroupForPhrase()
	{
		for (auto& phrasePtr : m_allPhrases)
		{
			PHRASEDATA* phrase = phrasePtr.get();

			if (phrase->rawWordList.size() > 1)
			{
				LETTERGROUP* letterGroup = GetOrCreateLetterGroup(phrase->rawReading.c_str());
				letterGroup->phraseList.push_back(phrase);
			}
			else
			{
				phrase->rawWordList[0]->isPrediciton = true;
				phrase->rawWordList[0]->probPredictionScore = phrase->probPosInside;
			}
			// inside
			m_logPosPhrase->AddValue(phrase->probPosInside);
		}
	}

	LETTERGROUP* GetOrCreateLetterGroup(const char16_t* rawReading)
	{
		uint8_t dictReading[maxIndexLen + 1];
		DICTCHAR::FromWStr(rawReading, dictReading, COUNT_OF(dictReading));

		auto findRes = m_allLetters.find(dictReading);
		if (findRes == m_allLetters.end())
		{
			LETTERGROUP *letterGroup = new LETTERGROUP();
			letterGroup->orgLetter = rawReading;
			letterGroup->fullLetter = (char*)dictReading;
			m_allLetters[(const uint8_t*)letterGroup->fullLetter.c_str()] = std::unique_ptr<LETTERGROUP>(letterGroup);
			return letterGroup;
		}
		else
		{
			return findRes->second.get();
		}
	}

	void CreateIndexPartList()
	{
		// split index string to unique part and tail part
		const uint8_t *preIndex, *curIndex, *nxtIndex;
		LETTERGROUP *curGroup;
		auto itLetter = m_allLetters.begin();

		preIndex = nullptr;
		curIndex = itLetter->first;
		curGroup = itLetter->second.get();
		++itLetter;
		nxtIndex = itLetter->first;

		for (;;)
		{
			size_t preCommonLen = 0;
			if (preIndex != nullptr)
			{
				size_t ich;
				for (ich = 0; preIndex[ich] == curIndex[ich]; ++ich);
				preCommonLen = ich;
			}

			size_t nxtCommonLen = 0;
			if (nxtIndex != nullptr)
			{
				size_t ich;
				for (ich = 0; nxtIndex[ich] == curIndex[ich]; ++ich);
				nxtCommonLen = ich;
			}

			size_t keepLen = std::max(preCommonLen, nxtCommonLen);
			if (curIndex[keepLen] != 0) keepLen++;

			if ((curGroup->fullLetter.length() - keepLen) > VOCAB_TAILSTRING::maxTailLen)
			{
				keepLen = curGroup->fullLetter.length() - VOCAB_TAILSTRING::maxTailLen;
			}

			curGroup->indexPart = curGroup->fullLetter.substr(0, keepLen);
			curGroup->tailPart = (const uint8_t*)curGroup->fullLetter.c_str() + keepLen;

			m_indexPartList.push_back((const uint8_t*)curGroup->indexPart.c_str());
			m_letterGroupList.push_back(curGroup);

			// loop process
			if (nxtIndex == nullptr)
			{
				break;
			}
			curGroup = itLetter->second.get();
			preIndex = curIndex;
			curIndex = nxtIndex;
			if (++itLetter == m_allLetters.end())
			{
				nxtIndex = nullptr;
			}
			else
			{
				nxtIndex = itLetter->first;
			}
		}
	}

	size_t BuildLetterGroup(uint8_t* buf, size_t offset, int nodeNumber, LETTERGROUP* letterGroup)
	{
		VOCAB_BASE* base = (VOCAB_BASE*)buf;
		size_t orgOffset = offset;
		bool needLetterNode = false;
		bool needBigramEnd = false;

		if (buf != nullptr)
		{
			// second pass
			THROW_IF_FALSE(letterGroup->memoryOffset == offset);
			THROW_IF_FALSE(letterGroup->letterNode == nodeNumber);
		}
		else
		{
			letterGroup->memoryOffset = offset;
			letterGroup->letterNode = nodeNumber;
		}

		if (letterGroup->tailPart != nullptr && letterGroup->tailPart[0] != 0)
		{
			size_t newOffset = VocabHelper::TailString::Size(offset, letterGroup->tailPart);
			if (base != nullptr)
			{
				VOCAB_BASE* newBase = VocabHelper::TailString::Make(base, letterGroup->tailPart);
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		if (letterGroup->posList.size() > 0)
		{
			THROW_IF_FALSE(letterGroup->readList.size() == 0);
			THROW_IF_FALSE(letterGroup->dispList.size() == 0);
			THROW_IF_FALSE(letterGroup->phraseList.size() == 0);
			THROW_IF_FALSE(letterGroup->predCacheList.size() == 0);
			THROW_IF_FALSE(letterGroup->posList.size() == 1);

			POSPRIM* posPrim = letterGroup->posList[0];

			m_totalBigram = NgramHelper::PosAsWord::Size(m_totalBigram, posPrim, m_logPosBigram.get());
			size_t newOffset = VocabHelper::PosAsWord::Size(offset, posPrim, nodeNumber);

			if (base != nullptr)
			{
				NgramHelper::PosAsWord::Make(m_bigramBlock, posPrim, m_logPosBigram.get());

				VOCAB_BASE* newBase = VocabHelper::PosAsWord::Make(base, posPrim, nodeNumber);
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		if (letterGroup->predCacheList.size() > 0)
		{
			PREDICTIONCACHE* predCahe = letterGroup->predCacheList[0];

			size_t newOffset = VocabHelper::PredCache::Size(offset, predCahe);
			if (base != nullptr)
			{
				VOCAB_BASE* newBase = VocabHelper::PredCache::Make(base, predCahe);
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		if (letterGroup->phraseList.size() > 0)
		{
			size_t newOffset = VocabHelper::Phrase::Size(offset, letterGroup->phraseList, m_logPosPhrase.get());
			if (base != nullptr)
			{
				VOCAB_BASE* newBase = VocabHelper::Phrase::Make(base, letterGroup->phraseList, m_logPosPhrase.get());
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		if (letterGroup->readList.size() > 0)
		{
			needLetterNode = true;

			m_totalBigram = NgramHelper::Word::Size(m_totalBigram, letterGroup->readList, m_logWordBigram.get());
			size_t newOffset = VocabHelper::Word::Size(offset, letterGroup->readList, nodeNumber, m_logPosInside.get(), m_logBackOff.get(), m_logPosPhrase.get());
			if (base != nullptr)
			{
				NgramHelper::Word::Make(m_bigramBlock, letterGroup->readList, m_logWordBigram.get());

				VOCAB_BASE* newBase = VocabHelper::Word::Make(base, letterGroup->readList, nodeNumber, m_logPosInside.get(), m_logBackOff.get(), m_logPosPhrase.get());
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		if (letterGroup->dispList.size() > 0)
		{
			size_t newOffset = VocabHelper::Display::Size(offset, letterGroup->dispList, nodeNumber);
			if (base != nullptr)
			{
				VOCAB_BASE* newBase = VocabHelper::Display::Make(base, letterGroup->dispList, nodeNumber);
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		// termination
		{
			size_t newOffset = VocabHelper::Termination::Size(offset, needLetterNode ? 1 : -1, needBigramEnd ? 1 : -1);
			if (base != nullptr)
			{
				VOCAB_BASE* newBase = VocabHelper::Termination::Make(base, needLetterNode ? letterGroup->letterNode : -1, needBigramEnd ? 1 : -1);
				THROW_IF_FALSE((newBase - base) == (ptrdiff_t)(newOffset - offset));
				base = newBase;
			}
			offset = newOffset;
		}

		if (buf != nullptr)
		{
			THROW_IF_FALSE((offset - orgOffset) == (size_t)((const char*)base - (const char*)buf));
		}
		return offset;
	}

	void BuildVocabBodyPart()
	{
		size_t m_vocabPartOffset = 1;
		m_totalBigram = 0;
		m_wArrayIndex->UpdateBaseValue([&](int nodeNumber, int indexToLetter) -> int {

			LETTERGROUP *letterGroup = m_letterGroupList[(size_t)indexToLetter];

			// sort word list for reading
			std::sort(letterGroup->readList.begin(), letterGroup->readList.end(),
				[](const WORDPRIM* lhs, const WORDPRIM* rhs)
			{
				if (lhs->rawPos != lhs->rawPos)
				{
					return lhs->rawPos->rawPosID < lhs->rawPos->rawPosID;
				}
				if (lhs->probPosInside != rhs->probPosInside)
				{
					return lhs->probPosInside > rhs->probPosInside;
				}
				return lhs->rawDisplay < rhs->rawDisplay;
			});
			// sort word list for display
			std::sort(letterGroup->dispList.begin(), letterGroup->dispList.end(),
				[](const WORDPRIM* lhs, const WORDPRIM* rhs)
			{
				if (lhs->rawPos != lhs->rawPos)
				{
					return lhs->rawPos->rawPosID < lhs->rawPos->rawPosID;
				}
				if (lhs->probPosInside != rhs->probPosInside)
				{
					return lhs->probPosInside > rhs->probPosInside;
				}
				return lhs->rawReading < rhs->rawReading;
			});

			m_vocabPartOffset = BuildLetterGroup(nullptr, m_vocabPartOffset, nodeNumber, m_letterGroupList[(size_t)indexToLetter]);
			return indexToLetter; // no change
		});

		m_bigramBlock.reserve(m_totalBigram);
		m_totalBigram = 0;

		m_vocabPart.resize(m_vocabPartOffset);
		m_vocabPartOffset = 1; // reset
		m_wArrayIndex->UpdateBaseValue([&](int nodeNumber, int indexToLetter) -> int {
			int newValue = (int)m_vocabPartOffset;
			uint8_t* writePtr = &m_vocabPart[0] + m_vocabPartOffset;
			m_vocabPartOffset = BuildLetterGroup(writePtr, m_vocabPartOffset, nodeNumber, m_letterGroupList[(size_t)indexToLetter]);
			return newValue;
		});
	}

	std::vector<uint8_t> BuildPosNameTextBlock()
	{
		uint32_t posCount = 0;
		size_t stringBufLen = 0;
		for (auto it = m_allPos.begin(); it != m_allPos.end(); ++it)
		{
			posCount++;
			stringBufLen += (it->second->rawPosName.length() + 1) * sizeof(char16_t);
		}
		BLOCK_POSNAMETEXT* nullPtr = nullptr;
		size_t blockSize = (size_t)((uint8_t*)&nullPtr->posList[posCount + 1] - (uint8_t*)nullptr) + stringBufLen;

		std::vector<uint8_t> blockBuf(blockSize);
		//char16_t * blockEnd = (char16_t*)(&blockBuf[0] + blockSize);

		BLOCK_POSNAMETEXT* posNameTxt((BLOCK_POSNAMETEXT*)&blockBuf[0]);
		posNameTxt->countOfPos = posCount;
		char16_t *stringBuf = (char16_t*)&posNameTxt->posList[posCount + 1];
		int iPos = 0;
		for (auto it = m_allPos.begin(); it != m_allPos.end(); ++iPos, ++it)
		{
			posNameTxt->posList[iPos].posID = (uint16_t)it->second->rawPosID;
			posNameTxt->posList[iPos].textOffset = (uint16_t)((uint8_t*)stringBuf - (uint8_t*)posNameTxt);
			memcpy(stringBuf, it->second->rawPosName.c_str(), (it->second->rawPosName.length() + 1) * sizeof(char16_t));
			stringBuf += it->second->rawPosName.length() + 1;
		}
		// Guard
		posNameTxt->posList[iPos].posID = (uint16_t)-1;
		posNameTxt->posList[iPos].textOffset = 0;

		std::sort(posNameTxt->posList, posNameTxt->posList + posCount, [](auto lhs, auto rhs) -> bool {
			return lhs.posID < rhs.posID;
		});
		return blockBuf;
	}

	void WriteOutSystemDictionary(const char* fileName)
	{
		size_t offset = RoundUp(sizeof(SDICTHEAD), SDICTBLOCKSIZE);

		// PROBABILITY
		size_t probabilityOffset = offset;
		std::unique_ptr<BLOCK_PROBABILITY> m_blockProb(new BLOCK_PROBABILITY);
		m_logPosInside->GetTable(m_blockProb->posInside);
		m_logPosPhrase->GetTable(m_blockProb->posPhrase);
		m_logPosBigram->GetTable(m_blockProb->posBigram);
		m_logPosTrigram->GetTable(m_blockProb->posTrigram);
		m_logWordBigram->GetTable(m_blockProb->wordBigram);
		m_logWordTrigram->GetTable(m_blockProb->wordTrigram);
		m_logBackOff->GetTable(m_blockProb->backOff);
		offset = RoundUp(probabilityOffset + sizeof(BLOCK_PROBABILITY), SDICTBLOCKSIZE);

		// Double Array
		size_t doubleArrayOffset = offset;
		std::vector<BaseCheck3232> wArray;
		m_wArrayIndex->GetBaseCheck3232(wArray);
		offset = RoundUp(doubleArrayOffset + sizeof(wArray[0]) * wArray.size(), SDICTBLOCKSIZE);

		// Vocaburary
		size_t vocabOffset = offset;
		offset = RoundUp(vocabOffset + m_vocabPart.size(), SDICTBLOCKSIZE);

		// bigram
		size_t bigramOffset = offset;
		size_t bigramSize = sizeof(m_bigramBlock[0]) * m_bigramBlock.size();
		offset = RoundUp(bigramOffset + bigramSize, SDICTBLOCKSIZE);

		// pos name block
		std::vector<uint8_t> posNameText = BuildPosNameTextBlock();
		size_t posNextTextOffset = offset;
		offset = RoundUp(posNextTextOffset + posNameText.size(), SDICTBLOCKSIZE);

		// header
		std::unique_ptr<uint8_t[]> headerPtr(new uint8_t[SDICTBLOCKSIZE]);
		SDICTHEAD* dictHead = (SDICTHEAD*)headerPtr.get();
		memset(dictHead, 0, SDICTBLOCKSIZE);

		dictHead->sdictUuid = UUID_DICTIONARY;
		dictHead->thisUuid = Platform->CreateUUID();

		time_t nowTime = time(nullptr);
		struct tm tmNow;
		tmNow = *localtime(&nowTime);

		dictHead->formatVersion = DICTFORMAT_VERSION;
		dictHead->dictType = DICTTYPE_SYSTEM;
		
		dictHead->buildYear = (uint32_t)tmNow.tm_year + 1900;
		dictHead->buildMonth = (uint32_t)tmNow.tm_mon + 1;
		dictHead->buildDay = (uint32_t)tmNow.tm_mday;
		dictHead->buildHour = (uint32_t)tmNow.tm_hour;
		dictHead->buildMinutes = (uint32_t)tmNow.tm_min;

		memset(dictHead->description, 0, sizeof(dictHead->description));
		sprintf(dictHead->description,
			"Static Dictionary\n"
			"Format version: %d.%04d\n"
			"Built time: %d/%02d/%02d %02d:%02d\n\x1a",
			DICTFORMAT_VERSION >> 16, DICTFORMAT_VERSION & 0xFFFF,
			dictHead->buildYear, dictHead->buildMonth, dictHead->buildDay,
			dictHead->buildHour, dictHead->buildMinutes);

		dictHead->blockCount = 0;
		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_PROBABILITY_TABLE;
		dictHead->blockList[dictHead->blockCount].blockOffset = (uint32_t)probabilityOffset;
		dictHead->blockList[dictHead->blockCount].blockSize = sizeof(BLOCK_PROBABILITY);
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;

		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_DOUBLEARRAY;
		dictHead->blockList[dictHead->blockCount].blockOffset = (uint32_t)doubleArrayOffset;
		dictHead->blockList[dictHead->blockCount].blockSize = (uint32_t)(sizeof(wArray[0]) * wArray.size());
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;

		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_VOCABURARY;
		dictHead->blockList[dictHead->blockCount].blockOffset = (uint32_t)vocabOffset;
		dictHead->blockList[dictHead->blockCount].blockSize = (uint32_t)m_vocabPart.size();
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;

		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_BIGRAM;
		dictHead->blockList[dictHead->blockCount].blockOffset = (uint32_t)bigramOffset;
		dictHead->blockList[dictHead->blockCount].blockSize = (uint32_t)bigramSize;
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;

#if 0
		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_TRIGRAM;
		dictHead->blockList[dictHead->blockCount].blockOffset = 0;
		dictHead->blockList[dictHead->blockCount].blockSize = 0;
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;
#endif
		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_POSNAMETEXT;
		dictHead->blockList[dictHead->blockCount].blockOffset = (uint32_t)posNextTextOffset;
		dictHead->blockList[dictHead->blockCount].blockSize = (uint32_t)posNameText.size();
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;

		offset = RoundUp(dictHead->blockList[dictHead->blockCount - 1].blockOffset
			+ dictHead->blockList[dictHead->blockCount - 1].blockSize, SDICTBLOCKSIZE);
		dictHead->blockList[dictHead->blockCount].blockType = BLOCKTYPE_END;
		dictHead->blockList[dictHead->blockCount].blockOffset = (uint32_t)offset;
		dictHead->blockList[dictHead->blockCount].blockSize = 0;
		dictHead->blockList[dictHead->blockCount]._free = 0;
		dictHead->blockCount++;

		{
			FILE *pf;
			if ((pf = fopen(fileName, "wb")) != nullptr)
			{
				auto scopeExit = ScopeExit([&]() { fclose(pf);  });

				std::unique_ptr<uint8_t[]> flushBuffer(new uint8_t[SDICTBLOCKSIZE]);
				memset(flushBuffer.get(), 0, SDICTBLOCKSIZE);

				int iDictBlock = 0;
				fwrite(&headerPtr[0], SDICTBLOCKSIZE, 1, pf);
				THROW_IF_FALSE((dictHead->blockList[0].blockOffset - ftell(pf)) <= SDICTBLOCKSIZE);
				fwrite(flushBuffer.get(), dictHead->blockList[iDictBlock].blockOffset - ftell(pf), 1, pf);

				fwrite(m_blockProb.get(), dictHead->blockList[iDictBlock].blockSize, 1, pf);
				iDictBlock++;
				THROW_IF_FALSE((dictHead->blockList[iDictBlock].blockOffset - ftell(pf)) <= SDICTBLOCKSIZE);
				fwrite(flushBuffer.get(), dictHead->blockList[iDictBlock].blockOffset - ftell(pf), 1, pf);

				fwrite(&wArray[0], dictHead->blockList[iDictBlock].blockSize, 1, pf);
				iDictBlock++;
				THROW_IF_FALSE((dictHead->blockList[iDictBlock].blockOffset - ftell(pf)) <= SDICTBLOCKSIZE);
				fwrite(flushBuffer.get(), dictHead->blockList[iDictBlock].blockOffset - ftell(pf), 1, pf);

				fwrite(&m_vocabPart[0], dictHead->blockList[iDictBlock].blockSize, 1, pf);
				iDictBlock++;
				THROW_IF_FALSE((dictHead->blockList[iDictBlock].blockOffset - ftell(pf)) <= SDICTBLOCKSIZE);
				fwrite(flushBuffer.get(), dictHead->blockList[iDictBlock].blockOffset - ftell(pf), 1, pf);

				fwrite(&m_bigramBlock[0], dictHead->blockList[iDictBlock].blockSize, 1, pf);
				iDictBlock++;
				THROW_IF_FALSE((dictHead->blockList[iDictBlock].blockOffset - ftell(pf)) <= SDICTBLOCKSIZE);
				fwrite(flushBuffer.get(), dictHead->blockList[iDictBlock].blockOffset - ftell(pf), 1, pf);

				fwrite(&posNameText[0], dictHead->blockList[iDictBlock].blockSize, 1, pf);
				iDictBlock++;
				THROW_IF_FALSE((dictHead->blockList[iDictBlock].blockOffset - ftell(pf)) <= SDICTBLOCKSIZE);
				fwrite(flushBuffer.get(), dictHead->blockList[iDictBlock].blockOffset - ftell(pf), 1, pf);
			}
		}
	}

	DictionaryConstructor() {}
	virtual ~DictionaryConstructor() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(DictionaryConstructor);
} /* namespace Dictionary */ } /* namespace Ribbon */
 