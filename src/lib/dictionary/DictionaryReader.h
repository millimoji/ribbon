#pragma once
#ifndef _RIBBON_DICTIONARYREADER_H_
#define _RIBBON_DICTIONARYREADER_H_
namespace Ribbon { namespace Dictionary {

class IndexTracer;
class VocabTracer;
class BigramReader;
class PosNameReader;
class PrimitiveLazy;
struct DictionaryReader;

struct DECODEDVOCAB
{
	enum class Type {
		Word,
		Composite,
		Display,
		PredCache,
		Termination,
	} type;
	union U {
		struct WORD {
			uint32_t wordId;
			int display;
			uint16_t pos;
			uint8_t posInside;
			uint8_t posBackOff;
			uint8_t predScore;
			int bigramTop;
			int bigramSize;
			bool isNgramTail;
		} Word;
		struct COMPOSITE {
			int wordCount;
			uint8_t predScore;
			int wordIdx[32];
		} Composite;
		struct DISPLAY {
			int wordCount;
			int wordIdx[32];
		} Display;
		struct PREDCACHE {
			int predCouunt;
			int predIdx[32];
		} PredCache;
		struct TERMINATION {
			int readingIdx;
			int bigramEnd;
		} Termination;
	} u;
};

class IndexTracer
{
	std::shared_ptr<const DictionaryReader> m_reader;
	const void *m_pvBaseCheck;
	const void *m_pvTailString;
	int m_tailIndex;

public:
	IndexTracer() :
		m_pvBaseCheck(nullptr), m_pvTailString(nullptr), m_tailIndex(-1)
	{}
	IndexTracer(const IndexTracer& src) :
		m_reader(src.m_reader), m_pvBaseCheck(src.m_pvBaseCheck), m_pvTailString(src.m_pvTailString),
		m_tailIndex(src.m_tailIndex)
	{}
	IndexTracer& operator = (const IndexTracer& src) {
		m_reader = src.m_reader; m_pvBaseCheck = src.m_pvBaseCheck; m_pvTailString = src.m_pvTailString;
		m_tailIndex = src.m_tailIndex;
		return *this;
	}
	IndexTracer(std::shared_ptr<const DictionaryReader> reader, int offset);
	bool GoChild(uint8_t dc);
	bool TrackDown(const char16_t* text);
	bool IsTailString() const;
	bool OnLeaf() const;
	bool TryGetVocab(VocabTracer& vocab, bool skipTailString = false) const;
	void BuildLetter(char16_t *buffer, size_t bufSize)  const;
	int GetLeafNodeIndex() const;
	RefString ToRefString() const;
};

class VocabTracer
{
	std::shared_ptr<const DictionaryReader> m_reader;
	const void *m_vocabTop = nullptr;

public:
	VocabTracer() {}
	VocabTracer(const VocabTracer& src) :
		m_reader(src.m_reader), m_vocabTop(src.m_vocabTop)
	{}
	VocabTracer& operator = (const VocabTracer& src) {
		m_reader = src.m_reader; m_vocabTop = src.m_vocabTop;
		return *this;
	}
	VocabTracer(VocabTracer&& src) :
		m_reader(std::move(src.m_reader)), m_vocabTop(src.m_vocabTop)
	{}
	VocabTracer& operator = (VocabTracer&& src) {
		m_reader = std::move(src.m_reader); m_vocabTop = src.m_vocabTop;
		return *this;
	}

	VocabTracer(std::shared_ptr<const DictionaryReader> reader, const void* vocabTop) :
		m_reader(reader), m_vocabTop(vocabTop)
	{}
	VocabTracer& Set(int offset);
	int VocabType() const;
	int GetLetterIndex() const;
	int VocabOffset() const;
	bool Expand(DECODEDVOCAB& vocab); // tracer moves next
	bool SkipToType(int vocabType);
};

class BigramReader
{
	std::shared_ptr<const DictionaryReader> m_reader;
	const void *m_bigramTop = nullptr;
	int m_bigramSize = 0;

public:
	BigramReader() {}
	BigramReader(const BigramReader& src) :
		m_reader(src.m_reader), m_bigramTop(src.m_bigramTop), m_bigramSize(src.m_bigramSize)
	{}
	BigramReader& operator = (const BigramReader& src) {
		m_reader = src.m_reader;  m_bigramTop = src.m_bigramTop; m_bigramSize = src.m_bigramSize;
		return *this;
	}

	BigramReader(std::shared_ptr<const DictionaryReader>, int, int);
	BigramReader& Set(int top, int size);
	bool FindBigram(uint32_t rhsWord, int* globalIdx, float* bigram);
	void Enumerate(const std::function<bool(int wordId, uint8_t probBigram)>& callBack);
};

class PosNameReader
{
	std::shared_ptr<const DictionaryReader> m_reader;

public:
	PosNameReader() {}
	PosNameReader(std::shared_ptr<const DictionaryReader> reader) :
		m_reader(reader)
	{}
	PosNameReader(const PosNameReader& src) :
		m_reader(src.m_reader)
	{}
	PosNameReader& operator = (const PosNameReader& src) {
		m_reader = src.m_reader;
		return *this;
	}

	const char16_t* GetPosName(uint16_t pos) const;
	void EnumeratePosName(const std::function<void(const char16_t* posName, const uint16_t classId)>& callback) const;
	uint16_t GetBOS() const;
	uint16_t GetEOS() const;
	uint16_t GetUNK() const;
	uint16_t GetEmojiClass() const { return GetUNK(); } // TODO:
};

struct IDictionaryReader
{
	virtual void Open(const char* fiileName) = 0;
	virtual uint8_t FindBigram(int topIdx, int bigramSize, uint32_t wordId, int *globalIdx) const = 0;
	virtual std::string WStrToDictStr(const char16_t* src) const = 0;

	virtual const float* GetPosInsideTable() const = 0;
	virtual const float* GetPosPhraseTable() const = 0;
	virtual const float* GetPosBigramTable() const = 0;
	virtual const float* GetPosTrigramTable() const = 0;
	virtual const float* GetWordBigramTable() const = 0;
	virtual const float* GetWordTrigramTable() const = 0;
	virtual const float* GetBackOffTable() const = 0;
	virtual IndexTracer CreateIndexTracer(int offset = 0) const = 0;
	virtual IndexTracer GetPosWordNgramTop() const = 0;
	virtual IndexTracer GetPosPosNgramTop() const = 0;
	virtual VocabTracer CreateVocabTracer(int offset) const = 0;
	virtual BigramReader CreateBigramReader(int top, int size) const = 0;
	virtual PosNameReader CreatePosNameReader() const = 0;
	virtual void GetPosDecodedVocab(uint16_t pos, bool isInWord, DECODEDVOCAB& vocab) const = 0;

	// utility methods
	float GetPosInside(int idx) const { return GetPosInsideTable()[idx]; }
	float GetPosPhrase(int idx) const { return GetPosPhraseTable()[idx]; }
	float GetPosBigram(int idx) const { return GetPosBigramTable()[idx]; }
	float GetPosTrigram(int idx) const { return GetPosTrigramTable()[idx]; }
	float GetWordBigram(int idx) const { return GetWordBigramTable()[idx]; }
	float GetWordTrigram(int idx) const { return GetWordTrigramTable()[idx]; }
	float GetBackOff(int idx) const { return GetBackOffTable()[idx]; }

	float GetPosBigram(uint16_t lhsPos, uint16_t rhsPos) const
	{
		DECODEDVOCAB lhsDecoded, rhsDecoded;
		GetPosDecodedVocab(lhsPos, false, lhsDecoded);
		GetPosDecodedVocab(rhsPos, false, rhsDecoded);

		int globalIdx;
		uint8_t bigramVal = FindBigram(lhsDecoded.u.Word.bigramTop, lhsDecoded.u.Word.bigramSize, rhsDecoded.u.Word.wordId, &globalIdx);
		return GetPosBigramTable()[bigramVal];
	}
	std::shared_ptr<IPrimitive> CreatePrimitive(const DECODEDVOCAB& decoded, int readingNode) const
	{
		THROW_IF_FALSE(decoded.type == DECODEDVOCAB::Type::Word);
		std::shared_ptr<IPrimitive> prim = FACTORYCREATE(Primitive);
		prim->Display(CreateIndexTracer(decoded.u.Word.display).ToRefString());
		prim->Reading(CreateIndexTracer(readingNode).ToRefString());
		prim->Class(decoded.u.Word.pos);
		return prim;
	}
	std::shared_ptr<IPrimitive> CreatePrimitive(int vocabOffset) const
	{
		VocabTracer vocabTracer = CreateVocabTracer(vocabOffset);
		DECODEDVOCAB decoded;
		THROW_IF_FALSE(vocabTracer.Expand(decoded));
		int readingIndex = vocabTracer.GetLetterIndex();
		return CreatePrimitive(decoded, readingIndex);
	}
	std::shared_ptr<IPhrase> CreatePhrase(const DECODEDVOCAB& decoded) const
	{
		THROW_IF_FALSE(decoded.type == DECODEDVOCAB::Type::Composite);
		std::shared_ptr<IPhrase> phrase = FACTORYCREATE(Phrase);
		for (int primIdx = 0; primIdx < decoded.u.Composite.wordCount; ++primIdx) {
			std::shared_ptr<IPrimitive> prim = CreatePrimitive(decoded.u.Composite.wordIdx[primIdx]);
			phrase->Push(prim);
		}
		return phrase;
	}
	std::shared_ptr<IPhrase> CreatePhrase(int vocabComposite) const
	{
		VocabTracer vocabTracer = CreateVocabTracer(vocabComposite);
		DECODEDVOCAB decoded;
		THROW_IF_FALSE(vocabTracer.Expand(decoded));
		return CreatePhrase(decoded);
	}
};

FACTORYEXTERN(DictionaryReader);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_DICTIONARYREADER_H_
