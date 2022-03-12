#include "pch.h"
#include "Complementer.h"
#include "DictionaryFormat.h"
#include "DictionaryReader.h"

namespace Ribbon { namespace Transliteration {

struct IntOutPhraseItem
{
	int vocabOffset;
	float phraseInside;
	float probability;
	uint16_t pos;
	bool isComposite;
};

struct Complementer :
	public std::enable_shared_from_this<Complementer>,
	public IComplementer,
	public IObject
{
	const Dictionary::IDictionaryReader* m_reader = nullptr;
	const COMPLEMENTPARAM* m_param = nullptr;
	ILattice* m_inputLattice = nullptr;
	int m_lastFrame;
	std::vector<std::unique_ptr<IntOutPhraseItem>> m_outputItems;

	Complementer()
	{
	}

	std::shared_ptr<IPhraseList> Complement(const COMPLEMENTPARAM& param, ILattice* lattice) override
	{
		if (lattice == nullptr || lattice->FrameCount() < 2) {
			return FACTORYCREATE(PhraseList);
		}

		m_param = &param;
		m_reader = m_param->dictionaryReader;
		m_inputLattice = lattice;
		m_lastFrame = m_inputLattice->FrameCount() - 1;

		RecursivelyTraceInput(0, m_reader->CreateIndexTracer());

		SortAndSelectPredictionItems();

		return ConvertIntPrimToContainer();
	}

	void RecursivelyTraceInput(int targetFrame, const Dictionary::IndexTracer& parentIndex)
	{
		int primCount = m_inputLattice->TopLinkCount(targetFrame);
		for (int primIdx = 0; primIdx < primCount; ++primIdx) {
			Dictionary::IndexTracer nextIndex(parentIndex);
			std::shared_ptr<IPrimitive> prim = m_inputLattice->TopLink(targetFrame, primIdx);

			if (!nextIndex.TrackDown(prim->Reading().u16ptr())) {
				continue;
			}
			if (prim->EndFrame() < m_lastFrame) {
				RecursivelyTraceInput(prim->EndFrame(), nextIndex);
				continue;
			}

			if (!CheckAndApplyPredcitoinCacheBlock(nextIndex)) {
				RecursivelyTraceIndex(nextIndex);
			}

			CheckAndAddPredictionPrimitives(nextIndex);
		}
	}

	bool CheckAndApplyPredcitoinCacheBlock(const Dictionary::IndexTracer& indexTracer)
	{
		Dictionary::VocabTracer vocabTracer;
		if (!indexTracer.TryGetVocab(vocabTracer, true)) {
			return false;
		}
		if (!vocabTracer.SkipToType(Dictionary::VOCAB_ID_PREDICTIONCACHE)) {
			return false;
		}
		Dictionary::DECODEDVOCAB decodedVocab;
		vocabTracer.Expand(decodedVocab);
		if (decodedVocab.type != Dictionary::DECODEDVOCAB::Type::PredCache) {
			return false; // confirm, just in case
		}

		// Register prediction items in prediction cache
		for (int itemIdx = 0; itemIdx < decodedVocab.u.PredCache.predCouunt; ++itemIdx) {
			int vocabOffset = decodedVocab.u.PredCache.predIdx[itemIdx];
			Dictionary::VocabTracer vocabPred = m_reader->CreateVocabTracer(vocabOffset);
			Dictionary::DECODEDVOCAB decodedPred;
			vocabPred.Expand(decodedPred);

			AddPredictionItem(decodedPred, vocabOffset);
		}
		return true;
	}

	void RecursivelyTraceIndex(const Dictionary::IndexTracer& indexTracer)
	{
		for (uint8_t ch = 1; ch < Dictionary::maxReadingLetter; ++ch) {
			Dictionary::IndexTracer nextIndex(indexTracer);

			if (!nextIndex.GoChild(ch)) {
				continue;
			}
			CheckAndAddPredictionPrimitives(nextIndex);

			RecursivelyTraceIndex(nextIndex);
		}
	}

	void CheckAndAddPredictionPrimitives(const Dictionary::IndexTracer& indexTracer)
	{
		Dictionary::VocabTracer vocabTracer;
		if (!indexTracer.TryGetVocab(vocabTracer, true)) {
			return;
		}
		for (;;)
		{
			Dictionary::DECODEDVOCAB decodedPred;
			int vocabOffset = vocabTracer.VocabOffset();
			vocabTracer.Expand(decodedPred);

			if (decodedPred.type == Dictionary::DECODEDVOCAB::Type::Word) {
				AddPredictionItem(decodedPred, vocabOffset);
			}
			else if (decodedPred.type == Dictionary::DECODEDVOCAB::Type::Composite) {
				AddPredictionItem(decodedPred, vocabOffset);
			}
			else if (decodedPred.type == Dictionary::DECODEDVOCAB::Type::Termination) {
				break;
			}
		}
	}

	void AddPredictionItem(const Dictionary::DECODEDVOCAB& decodedVocab, int vocabOffset)
	{
		if (decodedVocab.type == Dictionary::DECODEDVOCAB::Type::Composite) {
			m_outputItems.emplace_back(std::unique_ptr<IntOutPhraseItem>(new IntOutPhraseItem));
			IntOutPhraseItem* outPhraseItem = m_outputItems.back().get();
			outPhraseItem->isComposite = true;
			outPhraseItem->phraseInside = m_reader->GetPosPhrase(decodedVocab.u.Composite.predScore);
			outPhraseItem->vocabOffset = vocabOffset;

			Dictionary::VocabTracer vocabTopPrim = m_reader->CreateVocabTracer(decodedVocab.u.Composite.wordIdx[0]);
			Dictionary::DECODEDVOCAB decodedTopPrim;
			vocabTopPrim.Expand(decodedTopPrim);
			THROW_IF_FALSE(decodedTopPrim.type == Dictionary::DECODEDVOCAB::Type::Word);
			outPhraseItem->pos = decodedTopPrim.u.Word.pos;
		}
		else if (decodedVocab.type == Dictionary::DECODEDVOCAB::Type::Word) {
			m_outputItems.emplace_back(std::unique_ptr<IntOutPhraseItem>(new IntOutPhraseItem));
			IntOutPhraseItem* outPhraseItem = m_outputItems.back().get();
			outPhraseItem->isComposite = false;
			outPhraseItem->vocabOffset = vocabOffset;
			outPhraseItem->pos = decodedVocab.u.Word.pos;
			outPhraseItem->phraseInside = m_reader->GetPosPhrase(decodedVocab.u.Word.predScore);
		}
		else {
			THROW_IF_FALSE(false);
		}
	}

	void SortAndSelectPredictionItems()
	{
		for (const auto& phraseItem : m_outputItems) {
			float posBigram = m_reader->GetPosBigram(Dictionary::POSID_BOS, phraseItem->pos);
			phraseItem->probability = posBigram + phraseItem->phraseInside;
		}
		std::sort(m_outputItems.begin(), m_outputItems.end(), [](const auto& lhs, const auto& rhs) {
			return lhs->probability > rhs->probability;
		});
		if (m_param->maxPredictionItems > 0 && m_outputItems.size() > (size_t)m_param->maxPredictionItems) {
			m_outputItems.resize((size_t)m_param->maxPredictionItems);
		}
	}

	std::shared_ptr<IPhraseList> ConvertIntPrimToContainer()
	{
		std::shared_ptr<IPhraseList> phraseList = FACTORYCREATE(PhraseList);
		for (const auto& phraseItem : m_outputItems) {
			if (phraseItem->isComposite) {
				phraseList->Push(m_reader->CreatePhrase(phraseItem->vocabOffset));
			}
			else {
				std::shared_ptr<IPhrase> phrase = FACTORYCREATE(Phrase);
				phrase->Push(m_reader->CreatePrimitive(phraseItem->vocabOffset));
				phraseList->Push(phrase);
			}
		}
		return phraseList;
	}

	virtual ~Complementer() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(Complementer);
} /* namespace Transliteration */ } /* namespace Ribbon */
