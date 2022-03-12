#include "pch.h"
#include "DictionaryFormat.h"
#include "DictionaryReader.h"
namespace Ribbon { namespace Dictionary {

struct DictionaryReader :
	public std::enable_shared_from_this<DictionaryReader>,
	public IDictionaryReader,
	public IObject
{
	std::shared_ptr<IMemoryMappedFile> m_mappedFile;
	const SDICTHEAD* m_dictionaryHead = nullptr;
	const BLOCK_PROBABILITY* m_blockProb = nullptr;
	const BaseCheck3232* m_blockWArray = nullptr;
	const BaseCheck3232* m_blockWArrayEnd = nullptr;
	const uint8_t* m_blockVocab = nullptr;
	const uint8_t* m_blockVocabEnd = nullptr;
	const BIGRAM_ENTRY* m_blockBigram = nullptr;
	const BLOCK_POSNAMETEXT *m_blockPosName = nullptr;
	IndexTracer m_posWordNgramTop;
	IndexTracer m_posPosNgramTop;

	DictionaryReader()
	{
	}

	void Open(const char* fileName) override {
		m_mappedFile = Platform->OpenMemoryMappedFile(fileName);

		// setup pointers
		const uint8_t* dictTop = reinterpret_cast<const uint8_t*>(m_mappedFile->GetAddress());
		m_dictionaryHead = (const SDICTHEAD*)dictTop;

		THROW_IF_FALSE(dictTop != nullptr);
		HeaderCheck(m_dictionaryHead);

		SetupBlockPointers();

		SetupPosIndexTop();
	}

	void HeaderCheck(const SDICTHEAD *head) {
		THROW_IF_FALSE_MSG(head->sdictUuid == UUID_DICTIONARY, "UUID");
		THROW_IF_FALSE_MSG(head->formatVersion == DICTFORMAT_VERSION, "format version");
		THROW_IF_FALSE_MSG(head->dictType == DICTTYPE_SYSTEM, "dictionary type");
		THROW_IF_FALSE_MSG(head->blockCount > 0, "block count");

		const uint8_t* headLimit = (const uint8_t*)head + SDICTBLOCKSIZE;
		THROW_IF_FALSE_MSG((const uint8_t*)(&head->blockList[head->blockCount]) < headLimit, "over block count");
		THROW_IF_FALSE_MSG(head->blockList[head->blockCount - 1].blockType == BLOCKTYPE_END, "block end mark");
	}

	void SetupBlockPointers() {
		const uint8_t* dictTop = (const uint8_t*)m_dictionaryHead;

		for (uint32_t i = 0; i < m_dictionaryHead->blockCount; ++i) {
			switch (m_dictionaryHead->blockList[i].blockType) {
			case BLOCKTYPE_PROBABILITY_TABLE:
				m_blockProb = (const BLOCK_PROBABILITY*)(dictTop + m_dictionaryHead->blockList[i].blockOffset);
				THROW_IF_FALSE_MSG(sizeof(*m_blockProb) == m_dictionaryHead->blockList[i].blockSize, "Block Size");
				break;

			case BLOCKTYPE_DOUBLEARRAY:
				m_blockWArray = (const BaseCheck3232*)(dictTop + m_dictionaryHead->blockList[i].blockOffset);
				m_blockWArrayEnd = (const BaseCheck3232*)((const uint8_t*)m_blockWArray + m_dictionaryHead->blockList[i].blockSize);
				break;

			case BLOCKTYPE_VOCABURARY:
				m_blockVocab = (const uint8_t*)(dictTop + m_dictionaryHead->blockList[i].blockOffset);
				m_blockVocabEnd = m_blockVocab + m_dictionaryHead->blockList[i].blockSize;
				break;

			case BLOCKTYPE_BIGRAM:
				m_blockBigram = (const BIGRAM_ENTRY*)(dictTop + m_dictionaryHead->blockList[i].blockOffset);
				break;

			case BLOCKTYPE_TRIGRAM:
				break; // TODO

			case BLOCKTYPE_POSNAMETEXT:
				m_blockPosName = (const BLOCK_POSNAMETEXT *)(dictTop + m_dictionaryHead->blockList[i].blockOffset);
				break;

			case BLOCKTYPE_END:
				break;

			default:
				THROW_MSG("Unknown block");
			}
		}
	}

	void SetupPosIndexTop()
	{
		m_posWordNgramTop = IndexTracer(shared_from_this(), 0);
		uint8_t dictCharBuf[16];
		int dictCharSiz = DICTCHAR::FromWChar(PREFIX_POS_IN_WORDNGRAM, dictCharBuf, COUNT_OF(dictCharBuf));
		for (int i = 0; i < dictCharSiz; ++i) {
			THROW_IF_FALSE_MSG(m_posWordNgramTop.GoChild(dictCharBuf[i]), "No Inddex for POS-Word Ngram");
		}

		m_posPosNgramTop = IndexTracer(shared_from_this(), 0);
		dictCharSiz = DICTCHAR::FromWChar(PREFIX_POS_IN_POSNGRAM, dictCharBuf, COUNT_OF(dictCharBuf));
		for (int i = 0; i < dictCharSiz; ++i) {
			THROW_IF_FALSE_MSG(m_posPosNgramTop.GoChild(dictCharBuf[i]), "No Inddex for POS-Ngram");
		}
	}

	uint8_t FindBigram(int topIdx, int bigramSize, uint32_t wordId, int *globalIdx) const override
	{
		const BIGRAM_ENTRY* bigramFakeTop = m_blockBigram - 1;
		int endIdx = topIdx + bigramSize + 1;

		int midIdx;
		for (midIdx = (topIdx + endIdx) / 2; topIdx < midIdx; midIdx = (topIdx + endIdx) / 2) {
			if (bigramFakeTop[midIdx].wordId == wordId) {
				*globalIdx = midIdx + 1;
				return bigramFakeTop[midIdx].bigram;
			}
			else if (bigramFakeTop[midIdx].wordId < wordId) {
				topIdx = midIdx;
			} else {
				endIdx = midIdx;
			}
		}
		return 0;
	}

	std::string WStrToDictStr(const char16_t* src) const override {
		uint8_t dictCharBuf[maxIndexLen + 1];
		DICTCHAR::FromWStr(src, dictCharBuf, (int)COUNT_OF(dictCharBuf));
		return std::string((char*)dictCharBuf);
	}

	const float* GetPosInsideTable() const override { return m_blockProb->posInside; }
	const float* GetPosPhraseTable() const override { return m_blockProb->posPhrase; }
	const float* GetPosBigramTable() const override { return m_blockProb->posBigram; }
	const float* GetPosTrigramTable() const override { return m_blockProb->posTrigram; }
	const float* GetWordBigramTable() const override { return m_blockProb->wordBigram; }
	const float* GetWordTrigramTable() const override { return m_blockProb->wordTrigram; }
	const float* GetBackOffTable() const override { return m_blockProb->backOff; }

	IndexTracer CreateIndexTracer(int offset) const override { return IndexTracer(shared_from_this(), offset); }
	IndexTracer GetPosWordNgramTop() const override { return m_posWordNgramTop; }
	IndexTracer GetPosPosNgramTop() const override { return m_posPosNgramTop; }
	VocabTracer CreateVocabTracer(int offset) const override { return VocabTracer(shared_from_this(), m_blockVocab + offset); }
	BigramReader CreateBigramReader(int top, int size) const override { return BigramReader(shared_from_this(), top, size); }
	PosNameReader CreatePosNameReader() const override { return PosNameReader(shared_from_this()); }

	void GetPosDecodedVocab(uint16_t pos, bool isInWord, DECODEDVOCAB& decodedVocab) const override
	{
		IndexTracer idxTrc(isInWord ? GetPosWordNgramTop() : GetPosPosNgramTop());
		uint8_t posKey[8];
		// setup lhs pos
		posKey[DICTCHAR::FromWChar(pos, posKey, (int)COUNT_OF(posKey))] = 0;
		for (uint8_t *pdc = posKey; *pdc != 0; ++pdc) {
			if (!idxTrc.GoChild(*pdc)) {
				THROW_MSG("Fail to find POS vocab");
			}
		}
		VocabTracer vocabTracer;
		if (!idxTrc.TryGetVocab(vocabTracer)) {
			THROW_MSG("TryGetVocab() failed");
		}
		bool isExpanded;
		for (isExpanded = vocabTracer.Expand(decodedVocab);
			isExpanded && (decodedVocab.type != DECODEDVOCAB::Type::Word);
			isExpanded = vocabTracer.Expand(decodedVocab))
			;
	}
/*
	// small utility which is used in both decoder and complementer
	void ConvertGenericLatticeToDictCharLattice(ILattice* genericLattice, std::vector<std::vector<DictCharPrimitive>>& dictCharLattice) const override
	{
		dictCharLattice.clear();
		int frameCount = genericLattice->FrameCount();
		dictCharLattice.resize(static_cast<size_t>(frameCount - 1));

		for (int frameIdx = 0; frameIdx < (frameCount - 1); ++frameIdx)
		{
			std::vector<DictCharPrimitive >& dictCharFrame(dictCharLattice[(size_t)frameIdx]);
			int primCount = genericLattice->TopLinkCount(frameIdx);
			dictCharFrame.reserve((size_t)primCount);

			for (int primIdx = 0; primIdx < primCount; ++primIdx)
			{
				std::shared_ptr<IPrimitive> prim = genericLattice->TopLink(frameIdx, primIdx);
				DictCharPrimitive& dictCharPrim(dictCharFrame.emplace_back(DictCharPrimitive()));
				dictCharPrim.dictCharDisplay = WStrToDictStr(prim->Display().u16ptr());
				dictCharPrim.dictCharReading = WStrToDictStr(prim->Reading().u16ptr());
				dictCharPrim.endFrame = prim->EndFrame();
				dictCharPrim.srcPrim = prim;
			}
		}
	}
*/

	virtual ~DictionaryReader() {}
	IOBJECT_COMMON_METHODS
};

// Index Tracer
IndexTracer::IndexTracer(std::shared_ptr<const DictionaryReader> reader, int offset) :
	m_reader(reader),
	m_pvBaseCheck(reader->m_blockWArray),
	m_pvTailString(nullptr),
	m_tailIndex(-1)
{
	m_pvBaseCheck = reader->m_blockWArray + offset;
}
bool IndexTracer::GoChild(uint8_t dc)
{
	if (m_pvTailString != nullptr) {
		const VOCAB_TAILSTRING* tailString = reinterpret_cast<const VOCAB_TAILSTRING*>(m_pvTailString);
		if (m_tailIndex > tailString->length) {
			return false;
		}
		if (tailString->tailString[m_tailIndex] == dc) {
			++m_tailIndex;
			return true;
		}
		return false;
	}

	const BaseCheck3232* baseCheck = (const BaseCheck3232*)m_pvBaseCheck;
	if ((int)baseCheck->base < 0) {
		return false;
	}

	const BaseCheck3232* childNode = m_reader->m_blockWArray + baseCheck->base + dc;
	if ((m_reader->m_blockWArray + childNode->check) == baseCheck) {
		m_pvBaseCheck = childNode;
		if ((int)childNode->base < 0) {
			int vocabOffset = -((int)childNode->base + 1);
			const VOCAB_TAILSTRING* vocabTail = (const VOCAB_TAILSTRING*)(m_reader->m_blockVocab + vocabOffset);
			if (vocabTail->id == VOCAB_ID_TAILSTRING) {
				m_pvTailString = vocabTail;
				m_tailIndex = 0;
			}
		}
		return true;
	}
	return false;
}
bool IndexTracer::TrackDown(const char16_t* text)
{
	uint8_t dictCharBuf[16];
	for (const char16_t* charPtr = text; *charPtr != 0; ++charPtr)
	{
		int dictCharCount = DICTCHAR::FromWChar(*charPtr, dictCharBuf, (int)COUNT_OF(dictCharBuf));
		const uint8_t* dictCharEnd = dictCharBuf + dictCharCount;

		for (const uint8_t* dictCharPtr = dictCharBuf; dictCharPtr < dictCharEnd; ++dictCharPtr)
		{
			if (!GoChild(*dictCharPtr))
			{
				return false;
			}
		}
	}
	return true;
}
bool IndexTracer::IsTailString() const
{
	return m_pvTailString != nullptr;
}
bool IndexTracer::OnLeaf() const
{
	return (int)((const BaseCheck3232*)m_pvBaseCheck)->base < 0;
}
bool IndexTracer::TryGetVocab(VocabTracer& vocab, bool skipTailString) const
{
	if (m_pvTailString != nullptr) {
		const VOCAB_TAILSTRING* tailString = (const VOCAB_TAILSTRING*)m_pvTailString;
		if (!skipTailString && m_tailIndex <= tailString->length) {
			return false;
		}
		vocab = VocabTracer(m_reader, tailString->GoNext());
		return true;
	}
	// at leaf node
	const BaseCheck3232* baseCheck = (const BaseCheck3232*)m_pvBaseCheck;
	if ((int)baseCheck->base < 0) {
		int vocabOffset = -((int)baseCheck->base + 1);
		const VOCAB_BASE* vocabTop = (const VOCAB_BASE*)(m_reader->m_blockVocab + vocabOffset);
		vocab = VocabTracer(m_reader, vocabTop);
		THROW_IF_FALSE(vocabTop->id != VOCAB_ID_TAILSTRING);
		return true;
	}
	// parent node and if index:0 is leaf node
	const BaseCheck3232* childNode = m_reader->m_blockWArray + baseCheck->base;
	if ((m_reader->m_blockWArray + childNode->check) == baseCheck) {
		int vocabOffset = -((int)childNode->base + 1);
		const VOCAB_BASE* vocabTop = (const VOCAB_BASE*)(m_reader->m_blockVocab + vocabOffset);
		vocab = VocabTracer(m_reader, vocabTop);
		THROW_IF_FALSE(vocabTop->id != VOCAB_ID_TAILSTRING);
		return true;
	}
	return false;
}
void IndexTracer::BuildLetter(char16_t *buffer, size_t bufSize) const
{
	uint8_t dictStr[maxIndexLen + 1];

	int writintgIndex = 0;
	ptrdiff_t wArrayIdx = (const BaseCheck3232*)m_pvBaseCheck - m_reader->m_blockWArray;
	while (wArrayIdx != 0) {
		ptrdiff_t parentIdx = (int)m_reader->m_blockWArray[wArrayIdx].check;
		uint8_t dictChar = (uint8_t)(wArrayIdx - m_reader->m_blockWArray[parentIdx].base);
		if (dictChar > 0) {
			dictStr[writintgIndex++] = dictChar;
		}
		wArrayIdx = parentIdx;
	}
	dictStr[writintgIndex] = 0;
	textrev((char*)dictStr);

	const BaseCheck3232* leafNode = (const BaseCheck3232*)m_pvBaseCheck;
	if ((int)leafNode->base < 0) {
		int vocabOffset = -((int)leafNode->base + 1);
		const VOCAB_TAILSTRING* vocabTail = (const VOCAB_TAILSTRING*)(m_reader->m_blockVocab + vocabOffset);
		if (vocabTail->id == VOCAB_ID_TAILSTRING) {
			memcpy(dictStr + writintgIndex, vocabTail->tailString, (size_t)vocabTail->length + 1);
			dictStr[writintgIndex + vocabTail->length + 1] = 0;
		}
	}

	DICTCHAR::ToWStr(dictStr, buffer, (int)bufSize);
}
RefString IndexTracer::ToRefString() const
{
	char16_t textBuf[maxIndexLen + 1];
	BuildLetter(textBuf, COUNT_OF(textBuf));
	return RefString(textBuf);
}
int IndexTracer::GetLeafNodeIndex() const {
	const BaseCheck3232* baseCheck = (const BaseCheck3232*)m_pvBaseCheck;
	if ((int)baseCheck->base < 0) {
		return (int)(baseCheck - m_reader->m_blockWArray);
	}

	// parent node and if index:0 is leaf node
	const BaseCheck3232* childNode = m_reader->m_blockWArray + baseCheck->base;
	if ((m_reader->m_blockWArray + childNode->check) == baseCheck) {
		return (int)(childNode - m_reader->m_blockWArray);
	}
	THROW_MSG("Invalid leaf node");
}

// Vocab Tracer
VocabTracer& VocabTracer::Set(int offset)
{
	m_vocabTop = m_reader->m_blockVocab + offset;
	return *this;
}
int VocabTracer::VocabType() const
{
	const VOCAB_BASE* vocab = reinterpret_cast<const VOCAB_BASE*>(m_vocabTop);
	return vocab->id;
}
int VocabTracer::GetLetterIndex() const
{
	const VOCAB_TERMINATION* vocabTerm = VOCAB_TERMINATION::GoEnd(reinterpret_cast<const uint8_t*>(m_vocabTop));
	if (vocabTerm->hasReadingIdx) {
		const uint8_t* termEnd = (const uint8_t*)(vocabTerm + 1);
		return Read24(termEnd);
	}
	return -1;
}
int VocabTracer::VocabOffset() const
{
	return static_cast<int>(reinterpret_cast<const uint8_t*>(m_vocabTop) - m_reader->m_blockVocab);
}
bool VocabTracer::Expand(DECODEDVOCAB &decoded)
{
	const VOCAB_BASE* vocab = reinterpret_cast<const VOCAB_BASE*>(m_vocabTop);
	for (;;)
	{
		switch (vocab->id)
		{
		case VOCAB_ID_TAILSTRING:
			vocab = ((const VOCAB_TAILSTRING*)vocab)->GoNext();
			continue;

		case VOCAB_ID_WORD:
		{
			const VOCAB_WORD* vocabWord = reinterpret_cast<const VOCAB_WORD*>(vocab);
			decoded.type = DECODEDVOCAB::Type::Word;
			decoded.u.Word.wordId = (uint32_t)((const uint8_t*)vocab - reinterpret_cast<const uint8_t*>(m_reader->m_blockVocab));
			decoded.u.Word.display = Read24(vocabWord->display);
			decoded.u.Word.posInside = vocabWord->posInside;
			decoded.u.Word.pos = 0;
			decoded.u.Word.posBackOff = 0;
			decoded.u.Word.bigramTop = 0;
			decoded.u.Word.bigramSize = 0;
			decoded.u.Word.isNgramTail = !!vocabWord->isNgramTail;
			decoded.u.Word.predScore = 0;

			const uint8_t* wordEnd = (const uint8_t*)(vocabWord + 1);
			if (vocabWord->hasPos) {
				decoded.u.Word.pos = (uint16_t)Read16(wordEnd);
				wordEnd += 2;
			}
			if (vocabWord->isBigramHead) {
				decoded.u.Word.posBackOff = wordEnd[0];
				decoded.u.Word.bigramTop = Read24(wordEnd + 1);
				decoded.u.Word.bigramSize = Read24(wordEnd + 4);
				wordEnd += 7;
			}
			if (vocabWord->hasPhraseScore) {
				decoded.u.Word.predScore = wordEnd[0];
				wordEnd++;
			}
			m_vocabTop = wordEnd;
			return true;
		}
		case VOCAB_ID_DISPLAY:
		{
			const VOCAB_DISPLAY* vocabDisp = reinterpret_cast<const VOCAB_DISPLAY*>(vocab);
			decoded.type = DECODEDVOCAB::Type::Display;
			decoded.u.Display.wordCount = vocabDisp->count + 1;
			for (int i = 0; i < decoded.u.Display.wordCount; ++i) {
				decoded.u.Display.wordIdx[i] = Read24(vocabDisp->wordIndex[i]);
			}
			m_vocabTop = vocabDisp->GoNext();
			return true;
		}
		case VOCAB_ID_WORDARRAY:
		{
			const VOCAB_WORDARRAY* vocabWordArray = reinterpret_cast<const VOCAB_WORDARRAY*>(vocab);
			decoded.type = DECODEDVOCAB::Type::Composite;
			decoded.u.Composite.wordCount = vocabWordArray->count + 1;
			decoded.u.Composite.predScore = vocabWordArray->posInside;
			for (int i = 0; i < decoded.u.Composite.wordCount; ++i) {
				decoded.u.Composite.wordIdx[i] = Read24(vocabWordArray->wordIndex[i]);
			}
			m_vocabTop = vocabWordArray->GoNext();
			return true;
		}
		case VOCAB_ID_PREDICTIONCACHE:
		{
			const VOCAB_PREDICTIONCACHE* vocabPredCache = reinterpret_cast<const VOCAB_PREDICTIONCACHE*>(vocab);
			decoded.type = DECODEDVOCAB::Type::PredCache;
			decoded.u.PredCache.predCouunt = vocabPredCache->count + 1;
			for (int i = 0; i < decoded.u.PredCache.predCouunt; ++i) {
				decoded.u.PredCache.predIdx[i] = Read24(vocabPredCache->wordIndex[i]);
			}
			m_vocabTop = vocabPredCache->GoNext();
			return true;
		}
		case VOCAB_ID_TERMINATION:
		{
			const VOCAB_TERMINATION* vocabTerm = reinterpret_cast<const VOCAB_TERMINATION*>(vocab);
			decoded.type = DECODEDVOCAB::Type::Termination;
			decoded.u.Termination.readingIdx = -1;
			decoded.u.Termination.bigramEnd = 1;

			const uint8_t* termEnd = reinterpret_cast<const uint8_t*>(vocabTerm + 1);
			if (vocabTerm->hasReadingIdx) {
				decoded.u.Termination.readingIdx = Read24(termEnd);
				termEnd += 3;
			}
			if (vocabTerm->hasBigramEnd) {
				decoded.u.Termination.bigramEnd = Read24(termEnd);
				termEnd += 3;
			}
			// vocabTop should not be updated
			return false; // end of block
		}
		default:
			THROW_IF_FALSE(false);
		}
	}
}
bool VocabTracer::SkipToType(int vocabType)
{
	const VOCAB_BASE* vocabEnd = reinterpret_cast<const VOCAB_BASE*>(m_reader->m_blockVocabEnd);

	for (const VOCAB_BASE* vocab = reinterpret_cast<const VOCAB_BASE*>(m_vocabTop); vocab < vocabEnd; ) {
		if (vocab->id == vocabType) {
			m_vocabTop = vocab;
			return true;
		}
		switch (vocab->id) {
		case VOCAB_ID_TAILSTRING:
			vocab = ((const VOCAB_TAILSTRING*)vocab)->GoNext();
			continue;

		case VOCAB_ID_WORD:
			vocab = ((const VOCAB_WORD*)vocab)->GoNext();
			continue;

		case VOCAB_ID_DISPLAY:
			vocab = ((const VOCAB_DISPLAY*)vocab)->GoNext();
			continue;

		case VOCAB_ID_WORDARRAY:
			vocab = ((const VOCAB_WORDARRAY*)vocab)->GoNext();
			continue;

		case VOCAB_ID_PREDICTIONCACHE:
			vocab = ((const VOCAB_PREDICTIONCACHE*)vocab)->GoNext();
			continue;

		case VOCAB_ID_TERMINATION:
			return false;
		}
	}
	return false;
}

// Bigram reader
BigramReader::BigramReader(std::shared_ptr<const DictionaryReader> reader, int top, int size) :
	m_reader(reader)
{
	m_bigramTop = m_reader->m_blockBigram + top;
	m_bigramSize = size;
}
BigramReader& BigramReader::Set(int top, int size)
{
	m_bigramTop = m_reader->m_blockBigram + top;
	m_bigramSize = size;
	return *this;
}

// BigramReader
bool BigramReader::FindBigram(uint32_t rhsWordId, int* globalIdx, float* bigramProb)
{
	const BIGRAM_ENTRY* bigramRangeTop = reinterpret_cast<const BIGRAM_ENTRY*>(m_bigramTop) - 1;
	int rangeTop = 0;
	int rangeEnd = m_bigramSize + 1;
	int rangeMid;

	for (rangeMid = (rangeTop + rangeEnd) / 2; rangeTop < rangeMid; rangeMid = (rangeTop + rangeEnd) / 2) {
		if (bigramRangeTop[rangeMid].wordId == rhsWordId) {
			if (bigramRangeTop[rangeMid].bigram != 0) {
				*bigramProb = m_reader->m_blockProb->wordBigram[bigramRangeTop[rangeMid].bigram];
				*globalIdx = (int)(&bigramRangeTop[rangeMid] - m_reader->m_blockBigram);
				return true;
			}
			return false;
		}
		if (bigramRangeTop[rangeMid].wordId < rhsWordId) {
			rangeTop = rangeMid;
		} else {
			rangeEnd = rangeMid;
		}
	}
	return false;
}

void BigramReader::Enumerate(const std::function<bool(int wordId, uint8_t probBigram)>& callBack)
{
	const BIGRAM_ENTRY* _bigramTop = (const BIGRAM_ENTRY*)m_bigramTop;
	const BIGRAM_ENTRY* _bigramEnd = _bigramTop + m_bigramSize;

	for (const BIGRAM_ENTRY* entry = _bigramTop; entry != _bigramEnd; ++entry) {
		callBack((int)entry->wordId, entry->bigram);
	}
}

// PosNameReader
const char16_t* PosNameReader::GetPosName(uint16_t pos) const
{



	BLOCK_POSNAMETEXT::PosIdAndOffset idAndOff;
	idAndOff.posID = pos;
	idAndOff.textOffset = 0;

	auto it = std::lower_bound(m_reader->m_blockPosName->posList,
		m_reader->m_blockPosName->posList + m_reader->m_blockPosName->countOfPos, idAndOff,
		[](const BLOCK_POSNAMETEXT::PosIdAndOffset& lhs, const BLOCK_POSNAMETEXT::PosIdAndOffset& rhs) {
			return lhs.posID < rhs.posID;
		});
	if (it->posID == pos) {
		return (const char16_t*)((const uint8_t*)m_reader->m_blockPosName + it->textOffset);
	}
	return TEXT_UNK;
}

void PosNameReader::EnumeratePosName(const std::function<void(const char16_t* posName, uint16_t classId)>& callback) const
{
	/*
	struct BLOCK_POSNAMETEXT
	{
	uint32_t		countOfPos;
	struct PosIdAndOffset {
	uint16_t		posID;
	uint16_t		textOffset;
	}				posList[1];
	};
	*/
	const BLOCK_POSNAMETEXT::PosIdAndOffset* curPtr = m_reader->m_blockPosName->posList;
	const BLOCK_POSNAMETEXT::PosIdAndOffset* endPtr = curPtr + m_reader->m_blockPosName->countOfPos;

	for (; curPtr < endPtr; ++curPtr) {
		callback((const char16_t*)((const uint8_t*)m_reader->m_blockPosName + curPtr->textOffset), curPtr->posID);
	}
}

uint16_t PosNameReader::GetBOS() const { return POSID_BOS; }
uint16_t PosNameReader::GetEOS() const { return POSID_EOS; }
uint16_t PosNameReader::GetUNK() const { return POSID_UNK; }

FACTORYDEFINE(DictionaryReader);
} /* namespace Dictionary */ } /* namespace Ribbon */
