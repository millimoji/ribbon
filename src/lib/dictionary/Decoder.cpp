#include "pch.h"
#include "Decoder.h"
#include "DictionaryFormat.h"
#include "DictionaryReader.h"

namespace Ribbon { namespace Transliteration {

struct IntOutPrimitive {
	Dictionary::DECODEDVOCAB::U::WORD word;
	IPrimitive* primUnk = nullptr;
	int endFrame = -1;
	float viterbiBestScore = -FLT_MAX;
	IntOutPrimitive* viterbiBestNext = nullptr;
};

struct IntOutPrimLink {
	int targetTopFrame = 0;
	IntOutPrimitive* target = nullptr;
	IntOutPrimLink* lhsLink = nullptr;
};

struct Decoder :
	public std::enable_shared_from_this<Decoder>,
	public IDecoder,
	public IObject
{
	const DECODERARAM *m_decodeParam;
	Dictionary::IDictionaryReader* m_dictionary;
	ILattice *m_inputLattice;
	std::vector<std::vector<IntOutPrimitive>> m_intOutLattice;
	IntOutPrimitive* m_viterbiBestPrimtivie = nullptr;

	//
	bool m_DebugConversion_DumpLog;
	std::string m_DebugConversion_DumpFile;

	// TODO: move to TLS
	std::vector<float> rawPosBigramCache;

	Decoder()
	{
		std::shared_ptr<ISetting> sysSetting = Platform->GetSettings();
		m_DebugConversion_DumpLog = sysSetting->GetBool("Debug", "DumpConversionLog");
		if (m_DebugConversion_DumpLog) {
			m_DebugConversion_DumpFile = sysSetting->GetExpandedString("Debug", "DumpConversionFile");
		}
	}
	virtual ~Decoder() {}

	std::shared_ptr<IPhraseList> Decode(const DECODERARAM& param, ILattice* inputLattice) override
	{
		if (inputLattice == nullptr || inputLattice->FrameCount() < 2) {
			return FACTORYCREATE(PhraseList);
		}

		m_decodeParam = &param;
		m_dictionary = param.dictionary;

		m_inputLattice = inputLattice;

		Viterbi_BackwordViterbi();

		if (isNBestMode()) {
			if (m_viterbiBestPrimtivie == nullptr) {
				// TODO: need to return at least 1 phrase
				return FACTORYCREATE(PhraseList);
			}
			std::shared_ptr<IPhrase> resultPhrase = CreateBestPathToComposition(m_viterbiBestPrimtivie);

			if (m_DebugConversion_DumpLog) {
				std::shared_ptr<ILattice> lattice = Convertm_intOutLatticeToLattice();
				FILE* pf;
				if ((pf = fopen(m_DebugConversion_DumpFile.c_str(), "at")) != nullptr) {
					auto scopeExit = ScopeExit([&]() { fclose(pf); });
					Dictionary::PosNameReader posNameReader(m_dictionary->CreatePosNameReader());
					std::dynamic_pointer_cast<IDebugLogger>(lattice)->Dump(pf, [&](uint16_t pos) { return posNameReader.GetPosName(pos); });
					std::dynamic_pointer_cast<IDebugLogger>(resultPhrase)->Dump(pf, [&](uint16_t pos) { return posNameReader.GetPosName(pos); });
				}
			}

			std::shared_ptr<IPhraseList> phraseList = FACTORYCREATE(PhraseList);
			phraseList->Push(resultPhrase);
			return phraseList;
		}
		else if (isCandidateMode()) {
			return CreateCandidateList();
		}
		return std::shared_ptr<IPhraseList>(nullptr);
	}
	bool isNBestMode()
	{
		return m_decodeParam->decodeMode == DECODERARAM::N_BEST_CONVERSION;
	}
	bool isCandidateMode()
	{
		return m_decodeParam->decodeMode == DECODERARAM::CANDIDATE_LIST;
	}

	// Viterbi algorithm with looking up static dictionary
	void Viterbi_BackwordViterbi()
	{
		int frameCount = m_inputLattice->FrameCount();
		if (frameCount < 2) {
			return;
		}

		m_intOutLattice.resize((size_t)frameCount);
		for (int topFrameIdx = frameCount - 2; topFrameIdx >= 0; --topFrameIdx) {

			int wordVCount = m_inputLattice->TopLinkCount(topFrameIdx);

			for (int wordVIndex = 0; wordVIndex < wordVCount; ++wordVIndex) {
				std::shared_ptr<IPrimitive> primitive = m_inputLattice->TopLink(topFrameIdx, wordVIndex);
				RefString displayText = primitive->Display();
				RefString readingText = primitive->Reading();

				int wordTopFrame = topFrameIdx;
				int wordEndFrame = primitive->EndFrame();

				if (displayText.length() > 0 && readingText.length() > 0) {
					// word search
					Viterbi_LookupAndAppendWords(wordTopFrame, wordEndFrame, primitive.get());
				}
			}
			Dictionary::IndexTracer indexTracer(m_dictionary->CreateIndexTracer());
			Viterbi_TraceInputLattice(topFrameIdx, topFrameIdx, indexTracer);
		}
		IntOutPrimitive bosPrim;
		Viterbi_CalculateBigramWithBOS(bosPrim);
		m_viterbiBestPrimtivie = bosPrim.viterbiBestNext;
	}
	void Viterbi_TraceInputLattice(int topFrameIdx, int targetFrameIdx, Dictionary::IndexTracer& parentIdx)
	{
		int frameItemCount = m_inputLattice->TopLinkCount(targetFrameIdx);
		for (int frameItemIdx = 0; frameItemIdx < frameItemCount; ++frameItemIdx) {
			std::shared_ptr<IPrimitive> primitive = m_inputLattice->TopLink(targetFrameIdx, frameItemIdx);
			Dictionary::IndexTracer tryTracer(parentIdx);

			if (!tryTracer.TrackDown(primitive->Reading().u16ptr())) {
				continue;
			}
			int primitiveEndFrame = primitive->EndFrame();

			Dictionary::VocabTracer vocabTracer;
			if (tryTracer.TryGetVocab(vocabTracer)) {
				Viterbi_AppendWordsToLattice(topFrameIdx, primitiveEndFrame, vocabTracer);
			}
			// check frame range does not cross candidate range
			if (primitiveEndFrame < m_inputLattice->FrameCount()) {
				if (!isCandidateMode() ||
						((m_decodeParam->candidateTop < 0 || (topFrameIdx - m_decodeParam->candidateTop) * (primitiveEndFrame - m_decodeParam->candidateTop) >= 0) &&
						(m_decodeParam->candidateEnd < 0 || (topFrameIdx - m_decodeParam->candidateEnd) * (primitiveEndFrame - m_decodeParam->candidateEnd) >= 0))) {
					Viterbi_TraceInputLattice(topFrameIdx, primitiveEndFrame, tryTracer);
				}
			}
		}
	}
	void Viterbi_LookupAndAppendWords(int topFrameIdx, int endFrameIdx, IPrimitive* primitive) {
		std::vector<IntOutPrimitive>& topFrame(m_intOutLattice[(size_t)topFrameIdx]);
		bool reachingToEos = (endFrameIdx >= (m_inputLattice->FrameCount() - 1));
		bool foundInDictionary = false;

		// Trace display text, and then get display node id
		Dictionary::IndexTracer displayIndexTracer(m_dictionary->CreateIndexTracer());
		Dictionary::VocabTracer displayVocabTracer;

		if (displayIndexTracer.TrackDown(primitive->Display().u16ptr()) && displayIndexTracer.TryGetVocab(displayVocabTracer)) {
			// trace reading text, and then get vocaburary block
			Dictionary::IndexTracer readingIndexTracer(m_dictionary->CreateIndexTracer());
			Dictionary::VocabTracer readingVocabTracer;

			if (readingIndexTracer.TrackDown(primitive->Reading().u16ptr()) && readingIndexTracer.TryGetVocab(readingVocabTracer)) {
				
				int displayId = displayIndexTracer.GetLeafNodeIndex();
				Dictionary::DECODEDVOCAB decodedVocab;

				while (readingVocabTracer.Expand(decodedVocab)) {
					if (decodedVocab.type != Dictionary::DECODEDVOCAB::Type::Word ||
							decodedVocab.u.Word.display != displayId) {
						continue;
					}

					topFrame.push_back(IntOutPrimitive());
					topFrame.back().endFrame = endFrameIdx;
					topFrame.back().word = decodedVocab.u.Word;
					foundInDictionary = true;

					// Evaluate bigram
					if (reachingToEos) {
						// check with EOS
						Viterbi_CalculateBigramWithEOS(topFrame.back());
					} else {
						size_t rhsCount = m_intOutLattice[(size_t)endFrameIdx].size();
						rawPosBigramCache.resize(std::max(rhsCount, rawPosBigramCache.size()));

						Viterbi_CalculatePosBigramCache(topFrame.back());
						Viterbi_CalculateWordBigram(topFrame.back());
					}
				}
			}
		}
		if (!foundInDictionary) {
			topFrame.push_back(IntOutPrimitive());
			EstimateUnknownWord(primitive, endFrameIdx, topFrame.back());

			// Evaluate bigram
			if (reachingToEos) {
				// check with EOS
				Viterbi_CalculateBigramWithEOS(topFrame.back());
			} else  {
				size_t rhsCount = m_intOutLattice[(size_t)endFrameIdx].size();
				rawPosBigramCache.resize(std::max(rhsCount, rawPosBigramCache.size()));

				Viterbi_CalculatePosBigramCache(topFrame.back());
				Viterbi_CalculateWordBigram(topFrame.back());
			}
		}
	}
	void Viterbi_AppendWordsToLattice(int topFrameIdx, int endFrameIdx, Dictionary::VocabTracer& vocabTracer)
	{
		bool reachingToEos = (endFrameIdx >= (m_inputLattice->FrameCount() - 1));
		size_t rhsCount = (reachingToEos) ? 1 : m_intOutLattice[(size_t)endFrameIdx].size();
		rawPosBigramCache.resize(std::max(rhsCount, rawPosBigramCache.size()));

		std::vector<IntOutPrimitive>& topFrame(m_intOutLattice[(size_t)topFrameIdx]);

		bool enumerateAllWords = isCandidateMode() && topFrameIdx <= m_decodeParam->candidateTop && endFrameIdx <= m_decodeParam->candidateEnd;
		Dictionary::DECODEDVOCAB decodedVocab;
		uint16_t lastCheckedPos = 0;
		uint16_t lastPosBgiram = 0;
		while (vocabTracer.Expand(decodedVocab)) {
			if (decodedVocab.type != Dictionary::DECODEDVOCAB::Type::Word) {
				continue;
			}
			if (!enumerateAllWords && decodedVocab.u.Word.bigramSize <= 0 && !decodedVocab.u.Word.isNgramTail) {
				// on the Nbest mode, we don't need to check secondary word which is same pos with the last and no Ngram relation.
				// TODO: the word comes from the other word source than system dictionary.
				if (decodedVocab.u.Word.pos == lastCheckedPos) {
					continue;
				}
				lastCheckedPos = decodedVocab.u.Word.pos;
			}

			topFrame.push_back(IntOutPrimitive());
			topFrame.back().endFrame = endFrameIdx;
			topFrame.back().word = decodedVocab.u.Word;

			if (reachingToEos) {
				// check with EOS
				Viterbi_CalculateBigramWithEOS(topFrame.back());
			} else {
				if (lastPosBgiram != decodedVocab.u.Word.pos) {
					lastPosBgiram = decodedVocab.u.Word.pos;
					Viterbi_CalculatePosBigramCache(topFrame.back());
				}
				Viterbi_CalculateWordBigram(topFrame.back());
			}
		}
	}

	void Viterbi_CalculateBigramWithEOS(IntOutPrimitive& lhsWord)
	{
		float posBigramToEos = m_dictionary->GetPosBigram(lhsWord.word.pos, Dictionary::POSID_EOS);

		if (lhsWord.word.bigramSize > 0) {
			Dictionary::DECODEDVOCAB vocabEos;
			m_dictionary->GetPosDecodedVocab(Dictionary::POSID_EOS, true, vocabEos);

			Dictionary::BigramReader bigramReader(m_dictionary->CreateBigramReader(lhsWord.word.bigramTop, lhsWord.word.bigramSize));
			float bigramValue;
			int gloabalIndex;
			if (bigramReader.FindBigram(vocabEos.u.Word.wordId, &gloabalIndex, &bigramValue)) {
				lhsWord.viterbiBestScore = bigramValue;
				lhsWord.viterbiBestNext = nullptr;
			} else {
				lhsWord.viterbiBestScore = m_dictionary->GetBackOff(lhsWord.word.posBackOff) + posBigramToEos;
				lhsWord.viterbiBestNext = nullptr;
			}
		} else {
			lhsWord.viterbiBestScore = posBigramToEos;
			lhsWord.viterbiBestNext = nullptr;
		}
	}

	void Viterbi_CalculateBigramWithBOS(IntOutPrimitive& bosPrim)
	{
		rawPosBigramCache.resize(std::max(m_intOutLattice[0].size(), rawPosBigramCache.size()));

		// virutal output primitive
		Dictionary::DECODEDVOCAB wordBosVocab;
		m_dictionary->GetPosDecodedVocab(Dictionary::POSID_BOS, true, wordBosVocab);

		bosPrim.word = wordBosVocab.u.Word;
		bosPrim.endFrame = 0;

		Viterbi_CalculatePosBigramCache(bosPrim);
		Viterbi_CalculateWordBigram(bosPrim);
	}

	void Viterbi_CalculatePosBigramCache(IntOutPrimitive& lhsWord)
	{
		std::vector<IntOutPrimitive>& rhsWordList(m_intOutLattice[(size_t)lhsWord.endFrame]);

		uint16_t lastPos = 0;
		float lastPosBigram = 0.0f;
		for (size_t idxRhs = 0; idxRhs < rhsWordList.size(); ++idxRhs) {
			IntOutPrimitive& rhsWord(rhsWordList[idxRhs]);

			if (lastPos == rhsWord.word.pos) {
				rawPosBigramCache[idxRhs] = lastPosBigram;
			} else {
				lastPos = rhsWord.word.pos;
				lastPosBigram = m_dictionary->GetPosBigram(lhsWord.word.pos, lastPos);
				rawPosBigramCache[idxRhs] = lastPosBigram;
			}
			if (rawPosBigramCache[idxRhs] > -FLT_MAX) {
				rawPosBigramCache[idxRhs] += m_dictionary->GetPosInside(rhsWord.word.posInside);
			}
		}
	}

	void Viterbi_CalculateWordBigram(IntOutPrimitive& lhsWord)
	{
		std::vector<IntOutPrimitive>& rhsWordList(m_intOutLattice[(size_t)lhsWord.endFrame]);

		lhsWord.viterbiBestScore = -FLT_MAX;
		lhsWord.viterbiBestNext = nullptr;

		if (lhsWord.word.bigramSize == 0)
		{
			for (size_t idxRhs = 0; idxRhs < rhsWordList.size(); ++idxRhs)
			{
				IntOutPrimitive& rhsWord(rhsWordList[idxRhs]);
				float bigramScore = rawPosBigramCache[idxRhs] + rhsWord.viterbiBestScore;

				if (lhsWord.viterbiBestNext == nullptr || lhsWord.viterbiBestScore < bigramScore)
				{
					lhsWord.viterbiBestNext = &rhsWordList[idxRhs];
					lhsWord.viterbiBestScore = bigramScore;
				}
			}
			return;
		}

		float posBackOff = 0.0;
		if (lhsWord.word.posBackOff != 0)
		{
			posBackOff = m_dictionary->GetBackOff(lhsWord.word.posBackOff);
		}
		Dictionary::BigramReader bigramReader(m_dictionary->CreateBigramReader(lhsWord.word.bigramTop, lhsWord.word.bigramSize));

		for (size_t idxRhs = 0; idxRhs < rhsWordList.size(); ++idxRhs)
		{
			IntOutPrimitive& rhsWord(rhsWordList[idxRhs]);

			int globalIndex;
			float bigramScore;

			if (!rhsWord.word.isNgramTail || !bigramReader.FindBigram(rhsWord.word.wordId, &globalIndex, &bigramScore))
			{
				bigramScore = posBackOff + rawPosBigramCache[idxRhs];
			}
			bigramScore += rhsWord.viterbiBestScore;

			if (lhsWord.viterbiBestNext == nullptr || lhsWord.viterbiBestScore < bigramScore)
			{
				lhsWord.viterbiBestNext = &rhsWordList[idxRhs];
				lhsWord.viterbiBestScore = bigramScore;
			}
		}
	}

	float CalculateSimpleBigram(IntOutPrimitive* lhs, IntOutPrimitive* rhs)
	{
		IntOutPrimitive primBos;

		if (lhs == nullptr)
		{
			// virutal output primitive for BOS
			Dictionary::DECODEDVOCAB wordBosVocab;
			m_dictionary->GetPosDecodedVocab(Dictionary::POSID_BOS, true, wordBosVocab);

			primBos.word = wordBosVocab.u.Word;
			primBos.endFrame = 0;
			lhs = &primBos;
		}

		uint16_t rhsPos = (uint16_t)(rhs == nullptr ? Dictionary::POSID_EOS : rhs->word.pos);
		float posInside = (rhs == nullptr ? 0.0f : m_dictionary->GetPosInside(rhs->word.posInside));
		float bigramScore = m_dictionary->GetPosBigram(lhs->word.pos, rhsPos) + posInside;

		if (lhs->word.bigramSize > 0 && rhs != nullptr)
		{
			Dictionary::BigramReader bigramReader(m_dictionary->CreateBigramReader(lhs->word.bigramTop, lhs->word.bigramSize));

			int globalIndex;
			float wordBigramScore;
			if (bigramReader.FindBigram(rhs->word.wordId, &globalIndex, &wordBigramScore))
			{
				bigramScore = wordBigramScore;
			}
		}
		return bigramScore;
	}

	std::shared_ptr<IPhraseList> CreateCandidateList()
	{
		std::map<float, std::shared_ptr<IPhrase>> sortedResult;

		CreateCandidateListRecv(m_decodeParam->candidateTop, nullptr, sortedResult);

		std::shared_ptr<IPhraseList> phraseList = FACTORYCREATE(PhraseList);

		for (auto it = sortedResult.rbegin(); it != sortedResult.rend(); ++it)
		{
			phraseList->Push(it->second);
		}
		return phraseList;
	}

	void CreateCandidateListRecv(int targetFrame, IntOutPrimLink* lhsLink, std::map<float, std::shared_ptr<IPhrase>>& result)
	{
		std::vector<IntOutPrimitive>& primitiveList(m_intOutLattice[(size_t)targetFrame]);

		for (auto& primitive: primitiveList)
		{
			if (primitive.endFrame > m_decodeParam->candidateEnd) {
				continue;
			}
			IntOutPrimLink link;
			link.targetTopFrame = targetFrame;
			link.target = &primitive;
			link.lhsLink = lhsLink;

			if (primitive.endFrame == m_decodeParam->candidateEnd)
			{
				CreateCandidateListEvaluateAndStore(&link, result);
			}
			else
			{
				CreateCandidateListRecv(primitive.endFrame, &link, result);
			}
		}
	}

	void CreateCandidateListEvaluateAndStore(IntOutPrimLink* lhsLink, std::map<float,std::shared_ptr<IPhrase>>& result)
	{
		// find left side word
		IntOutPrimitive *lhsPrim;
		if (m_decodeParam->candidateTop <= 0) {
			lhsPrim = nullptr;
		}
		else {
			IntOutPrimitive *tracingLhs = m_viterbiBestPrimtivie;
			for (;
				tracingLhs != nullptr && tracingLhs->endFrame < m_decodeParam->candidateTop;
				tracingLhs = tracingLhs->viterbiBestNext)
				;
			lhsPrim = tracingLhs;
		}

		// calculate bigram with making composition
		std::shared_ptr<IPhrase> phrase = FACTORYCREATE(Phrase);

		std::shared_ptr<IPrimitive> primitive = IntOutPirmitiveToLatticePrimitive(lhsLink->target, lhsLink->targetTopFrame);
		phrase->Push(primitive);

		IntOutPrimLink* leftEdgeLink = lhsLink;
		float cumulativeScore = lhsLink->target->viterbiBestScore;
		IntOutPrimLink* tracingLink;
		for (tracingLink = lhsLink; tracingLink->lhsLink != nullptr; tracingLink = tracingLink->lhsLink) {
			cumulativeScore += CalculateSimpleBigram(tracingLink->lhsLink->target, tracingLink->target);
			leftEdgeLink = tracingLink;

			primitive = IntOutPirmitiveToLatticePrimitive(tracingLink->lhsLink->target, tracingLink->lhsLink->targetTopFrame);
			phrase->Push(primitive);
		}
		cumulativeScore += CalculateSimpleBigram(lhsPrim, leftEdgeLink->target);

		if (cumulativeScore > -FLT_MAX)
		{
			phrase->ReverseList(); // because we add primitives from right
			result.insert(std::make_pair(cumulativeScore, phrase));
		}
	}

	std::shared_ptr<IPhrase> CreateBestPathToComposition(IntOutPrimitive* bestPrimitive)
	{
		std::shared_ptr<IPhrase> bestPath = FACTORYCREATE(Phrase);

		int topFrame = 0;
		IntOutPrimitive* intOutPrimitvePtr = bestPrimitive;
		while (intOutPrimitvePtr != nullptr)
		{
			std::shared_ptr<IPrimitive> latPrimitive = IntOutPirmitiveToLatticePrimitive(intOutPrimitvePtr, topFrame);

			bestPath->Push(latPrimitive);

			topFrame = intOutPrimitvePtr->endFrame;
			intOutPrimitvePtr = intOutPrimitvePtr->viterbiBestNext;
		}
		return bestPath;
	}

	std::shared_ptr<ILattice> Convertm_intOutLatticeToLattice()
	{
		std::shared_ptr<ILattice> lattice = FACTORYCREATE(Lattice);

		for (int frameIdx = 0; frameIdx < (int)m_intOutLattice.size(); ++frameIdx)
		{
			std::vector<IntOutPrimitive>& latticeFrame(m_intOutLattice[(size_t)frameIdx]);

			for (int primitiveIdx = 0; primitiveIdx < (int)latticeFrame.size(); ++primitiveIdx)
			{
				std::shared_ptr<IPrimitive> latPrimitive = IntOutPirmitiveToLatticePrimitive(&latticeFrame[(size_t)primitiveIdx], frameIdx);
				lattice->Add(latPrimitive);
			}
		}
		return lattice;
	}

	std::shared_ptr<IPrimitive> IntOutPirmitiveToLatticePrimitive(IntOutPrimitive* intOutPrimitive, int topFrame)
	{
		Dictionary::IndexTracer dispIdx(m_dictionary->CreateIndexTracer(intOutPrimitive->word.display));
		char16_t displayBuf[Dictionary::maxIndexLen];
		dispIdx.BuildLetter(displayBuf, COUNT_OF(displayBuf));

		Dictionary::VocabTracer vocabTracer(m_dictionary->CreateVocabTracer((int)intOutPrimitive->word.wordId));
		Dictionary::IndexTracer readIdx(m_dictionary->CreateIndexTracer(vocabTracer.GetLetterIndex()));
		char16_t readBuf[Dictionary::maxIndexLen];
		readIdx.BuildLetter(readBuf, COUNT_OF(readBuf));

		std::shared_ptr<IPrimitive> _latPrim = FACTORYCREATE(Primitive);
		_latPrim->Display(RefString(displayBuf));
		_latPrim->Reading(RefString(readBuf));
		_latPrim->Class(intOutPrimitive->word.pos);
		_latPrim->TopEndFrame(std::tuple<uint16_t, uint16_t>(static_cast<uint16_t>(topFrame), static_cast<uint16_t>(intOutPrimitive->endFrame)));

		return _latPrim;
	}

	void EstimateUnknownWord(IPrimitive* prim, int endFrame, IntOutPrimitive& intOutPrim)
	{
		// TODO: Alphabet, Katakana, Number
		intOutPrim.word.wordId = (uint32_t)-1;
		intOutPrim.word.display = -1;
		intOutPrim.word.pos = Dictionary::POSID_UNK;
		intOutPrim.word.posInside = 128; // TODO
		intOutPrim.word.posBackOff = 0;
		intOutPrim.word.bigramTop = 0;
		intOutPrim.word.bigramSize = 0;
		intOutPrim.word.isNgramTail = false;

		intOutPrim.primUnk = prim;
		intOutPrim.endFrame = endFrame;
	}

	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(Decoder);
} /* namespace Transliteration */ } /* namespace Ribbon */
