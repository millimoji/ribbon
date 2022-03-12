#include "pch.h"
#include "HistoryStruct.h"
#include "HistoryStore.h"
#include "HistoryReuser.h"

namespace Ribbon { namespace History {

struct InternalLatticeNode
{
	HistoryText* text;
	int frameLen;
	bool prefixMatch;
};
	
struct InternalLattice
{
	std::vector<std::vector<InternalLatticeNode>> frames;
};

// Logic 	
struct BuildWordLattice
{
	ILattice* m_inputLattice;
	InternalLattice* m_internalLattice;
	TrieNode* m_rootTrie;
	int m_frameCount;
	bool m_predictionMode;
	bool m_hasEndEdgeWord;
	bool m_latinMode;

	BuildWordLattice(const std::shared_ptr<ILattice>& inputLattice, TrieNode* rootTrie, bool latinMode, InternalLattice* outputLattice) :
		m_latinMode(latinMode)
	{
		m_inputLattice = inputLattice.get();
		m_internalLattice = outputLattice;
		m_rootTrie = rootTrie;
		m_frameCount = m_inputLattice->FrameCount() - 1;
		THROW_IF_FALSE(m_frameCount > 0);
		m_internalLattice->frames.resize(static_cast<size_t>(m_frameCount));
		m_predictionMode = true;
		m_hasEndEdgeWord = false;
	}
	bool Build()
	{
		for (int frameIdx = 0; frameIdx < m_frameCount; ++frameIdx) {
			if (m_latinMode && frameIdx > 0) break; // only 0 index
			TrieTracer tracerRoot(m_rootTrie);
			BuildRecursive(tracerRoot, frameIdx, frameIdx);
		}
		return m_hasEndEdgeWord;
	}
	void BuildRecursive(const TrieTracer& trieTracer, int startFrame, int nextFrame)
	{
		auto histText = trieTracer.GetHistoryText();

		if (histText) {
			m_internalLattice->frames[static_cast<size_t>(startFrame)].emplace_back(InternalLatticeNode());
			InternalLatticeNode& node(m_internalLattice->frames[static_cast<size_t>(startFrame)].back());
			node.frameLen = nextFrame - startFrame;
			node.prefixMatch = false;
			node.text = histText.get();
			if (nextFrame >= m_frameCount) {
				m_hasEndEdgeWord = true;
			}
		}
		// prediction process
		if (nextFrame >= m_frameCount) {
			if (m_predictionMode) {
				int initialLevel = trieTracer.TrieLevel();

				trieTracer.TraceChildren([&](HistoryText* text) {
					if (text->text.u16ptr()[initialLevel] != 0) {
						m_internalLattice->frames[(size_t)startFrame].emplace_back(InternalLatticeNode());
						InternalLatticeNode& node(m_internalLattice->frames[(size_t)startFrame].back());
						node.frameLen = nextFrame - startFrame;
						node.prefixMatch = true;
						node.text = text;
						m_hasEndEdgeWord = true;
					}
				});
			}
			return;
		}

		int linkCount = m_inputLattice->TopLinkCount(nextFrame);
		for (int linkIdx = 0; linkIdx < linkCount; ++linkIdx)
		{
			auto prim = m_inputLattice->TopLink(nextFrame, linkIdx);

			TrieTracer nextTracer(trieTracer);
			const char16_t* pch = prim->Reading().u16ptr();
			for (; *pch != 0; ++pch) {
				if (!nextTracer.Down(*pch)) {
					break;
				}
			}
			if (*pch == 0) { // Succeeded down
				BuildRecursive(nextTracer, startFrame, prim->EndFrame());
			}
		}
	}
};

// Logic 	
struct BuildPredictionPhrases
{
	const std::vector<HistoryPrimitive*>& m_prevPrims;
	std::shared_ptr<IPhraseList> m_phraseList;
	InternalLattice* m_internalLattice;
	const NGramNode* m_rootNgram;
	size_t m_frameCount;
	const int m_maxWordsInPhrase;
	const bool m_isLatinMode;
	bool m_predictionMode = false;
	size_t m_maxReturnPhrases = 50;
	
	struct NGramInfo {
		const NGramNode* nGramNode;
		int nGramQueryFrame;
		float nGramProbability;

		NGramInfo(const NGramNode* _node, int _frame, float _prob) :
			nGramNode(_node), nGramQueryFrame(_frame), nGramProbability(_prob) {}
		NGramInfo(const NGramInfo& src) :
			nGramNode(src.nGramNode), nGramQueryFrame(src.nGramQueryFrame), nGramProbability(src.nGramProbability) {}
		NGramInfo& operator = (const NGramInfo& src) {
			nGramNode = src.nGramNode; nGramQueryFrame = src.nGramQueryFrame; nGramProbability = src.nGramProbability;
			return *this; }
	};
	std::vector<NGramInfo> m_nGramList;
	uint32_t m_queryTargetIndex = 0;

	struct PhraseInfo {
		const NGramNode* nodeList[MAX_NGRAM_LENGTH + 1];
		uint32_t nodeCount;
		float probability;
		uint32_t contextWords;

		PhraseInfo(const std::vector<NGramInfo>& _nGramList, uint32_t _contextWords) {
			nodeCount = static_cast<uint32_t>(_nGramList.size());
			for (uint32_t i = 0; i < nodeCount; ++i) {
				nodeList[i] = _nGramList[i].nGramNode;
			}
			probability = _nGramList.back().nGramProbability;
			contextWords = _contextWords;
		}
		PhraseInfo(const PhraseInfo& src) {
			memcpy(this, &src, sizeof(PhraseInfo)); }
		PhraseInfo& operator = (const PhraseInfo& src) {
			memcpy(this, &src, sizeof(PhraseInfo));
			return *this; }
		bool operator < (const PhraseInfo& rhs) const {
			if (contextWords == rhs.contextWords) {
				if (probability == rhs.probability) {
					return nodeCount < rhs.nodeCount;
				}
				return probability < rhs.probability;
			}
			return (contextWords < rhs.contextWords);
		}
	};
	std::multiset<PhraseInfo> m_sortedPhrase;

	BuildPredictionPhrases(const std::vector<HistoryPrimitive*>& prevPrims, InternalLattice* internalLattice,
		const NGramNode* rootNgram, int maxWordsInPhrase, const std::shared_ptr<IPhraseList>& phraseList) :
		m_prevPrims(prevPrims), m_maxWordsInPhrase(maxWordsInPhrase), m_isLatinMode(maxWordsInPhrase < 0)
	{
		m_internalLattice = internalLattice;
		m_rootNgram = rootNgram;
		m_phraseList = phraseList;
		m_frameCount = m_internalLattice != nullptr ? m_internalLattice->frames.size() : 0;
		m_predictionMode = true;
	}
	void Build()
	{
		const uint32_t startContextCount = (m_frameCount == 0) ? 1u : 0u;
		constexpr uint32_t MAX_CONTEXTCOUNT = 6;

		for (uint32_t contextCount = startContextCount; contextCount <= MAX_CONTEXTCOUNT; ++contextCount) {
			m_nGramList.clear();
			m_nGramList.emplace_back(NGramInfo(m_rootNgram, -1, 1.0f));
			m_queryTargetIndex = contextCount + 1;

			if (contextCount > 0) {
				if (contextCount > m_prevPrims.size()) {
					break;
				}
				ptrdiff_t startIndex = static_cast<ptrdiff_t>(m_prevPrims.size() - contextCount);
				bool isSucceeded = true;
				for (auto it = m_prevPrims.begin() + startIndex; it != m_prevPrims.end(); ++it) {
					auto nGramNode = m_nGramList.back().nGramNode->Find(*it);
					if (!nGramNode) {
						isSucceeded = false;
						break;
					}
					m_nGramList.emplace_back(NGramInfo(nGramNode, -1, 1.0f));
				}
				if (!isSucceeded) {
					continue;
				}
			}

			m_nGramList.back().nGramQueryFrame = 0;

			BuildRecursive(0);
		}

		std::set<RefString> existingDisplay;
		for (auto rit = m_sortedPhrase.rbegin(); rit != m_sortedPhrase.rend(); ++rit) {
			auto phrase = FACTORYCREATE(Phrase);
			for (size_t index = static_cast<size_t>(rit->contextWords); index < rit->nodeCount; ++index) {
				auto prim = CreatePrimitiveFromHistory(rit->nodeList[index]->primitive);
				phrase->Push(prim);
			}
			if (existingDisplay.insert(phrase->Display()).second) {
				m_phraseList->Push(phrase);
			}
		}
	}

	void BuildRecursive(size_t nextFrame)
	{
		if (nextFrame >= m_frameCount) {
			TraceNgramRecursive();
			return;
		}

		std::vector<std::tuple<const NGramNode*, size_t>> targetNodes;
		uint32_t probDenomi = 0;

		for (const auto& latNode : m_internalLattice->frames[nextFrame]) {
			for (auto histPrim : latNode.text->primitives) {
				const NGramNode* nextChild = m_nGramList.back().nGramNode->Find(histPrim);
				if (nextChild) {
					size_t nextNextFrame = nextFrame + latNode.frameLen;
					targetNodes.emplace_back(std::make_pair(nextChild, nextNextFrame));
					probDenomi += nextChild->usedCount;
				}
			}
		}
		for (const auto& nextChildPair : targetNodes) {
			const NGramNode* nextChild = std::get<0>(nextChildPair);
			size_t nextNextFrame = std::get<1>(nextChildPair);

			float nextProbability = probDenomi <= 0 ? 0.0f :
				(m_nGramList.back().nGramProbability * static_cast<float>(nextChild->usedCount) / static_cast<float>(probDenomi));

			m_nGramList.emplace_back(NGramInfo(nextChild, (int)nextNextFrame, nextProbability));

			BuildRecursive(nextNextFrame);

			m_nGramList.pop_back();
		}
	}

	void TraceNgramRecursive()
	{
		size_t wordsInPhrase = (m_nGramList.size() - m_queryTargetIndex);

		if (m_maxWordsInPhrase < 0 || wordsInPhrase < static_cast<size_t>(m_maxWordsInPhrase)) {
			const auto& currentBack = m_nGramList.back().nGramNode;
			uint32_t probDenomi = 0;
			for (const auto& nextChild : currentBack->children) {
				probDenomi += nextChild.usedCount;
			}
			for (const auto& nextChild : currentBack->children) {
				float probability = probDenomi <= 0 ? 0.0f :
					(m_nGramList.back().nGramProbability * static_cast<float>(nextChild.usedCount) / static_cast<float>(probDenomi));

				m_nGramList.emplace_back(NGramInfo(&nextChild, m_nGramList.back().nGramQueryFrame, probability));

				TraceNgramRecursive();

				m_nGramList.pop_back();
			}
		}

		float targetMinimumProbability = (m_sortedPhrase.size() < m_maxReturnPhrases) ?
			0.0f : m_sortedPhrase.begin()->probability;

		if (m_nGramList.back().nGramProbability > targetMinimumProbability) {
			m_sortedPhrase.emplace(PhraseInfo(m_nGramList, m_queryTargetIndex));

			if (m_sortedPhrase.size() > m_maxReturnPhrases) {
				m_sortedPhrase.erase(m_sortedPhrase.begin());
			}
		}
	}

	std::shared_ptr<IPrimitive> CreatePrimitiveFromHistory(const HistoryPrimitive* histPrim)
	{
		std::shared_ptr<IPrimitive> prim = FACTORYCREATE(Primitive);
		prim->Display(histPrim->display->text);
		prim->Reading(histPrim->reading->text);
		prim->Class(histPrim->classPtr->classId);
		prim->BitFlags(histPrim->bitFlags);
		return prim;
	}

	BuildPredictionPhrases(const BuildPredictionPhrases&) = delete;
	BuildPredictionPhrases& operator = (const BuildPredictionPhrases&) = delete;
	BuildPredictionPhrases& operator = (BuildPredictionPhrases&&) = delete;
};

struct HistoryReuser :
	public std::enable_shared_from_this<HistoryReuser>,
	public IHistoryReuser,
	public IObject
{
	std::shared_ptr<IHistoryClassList> m_classList;
	std::shared_ptr<IHistoryPrimList> m_primList;
	std::shared_ptr<IHistoryNGramTree> m_nGramTree;
	std::shared_ptr<IEnglishReinforce> m_engRein;
	std::vector<HistoryPrimitive*> m_prevPrims;
	int m_maxWordsInPhrase;

	void Initialize(IHistoryClassList* classList, IHistoryPrimList* primList, IHistoryNGramTree* nGramTree, IEnglishReinforce* engRein, int maxWordsInPhrase) override
	{
		m_classList = GetSharedPtr<IHistoryClassList>(classList);
		m_primList = GetSharedPtr<IHistoryPrimList>(primList);
		m_nGramTree = GetSharedPtr<IHistoryNGramTree>(nGramTree);
		m_engRein = GetSharedPtr<IEnglishReinforce>(engRein);
		m_maxWordsInPhrase = maxWordsInPhrase;
	}
	void SetContext(const std::shared_ptr<IPhrase>& previousPhrase) override
	{
		m_prevPrims.clear();
		if (!previousPhrase) return;

		m_prevPrims = ConvertPhraseToHistVector(previousPhrase);
	}
	std::shared_ptr<IPhraseList> Query(const std::shared_ptr<ILattice>& inputLattice) override
	{
		bool isSucceeded = true;
		InternalLattice internalLattice;

		if (inputLattice) { // type ahead prediction
			bool isLatin = (m_maxWordsInPhrase < 0); // TODO: consider configuration combination
			isSucceeded = BuildWordLattice(inputLattice, m_primList->GetTrieNode(), isLatin, &internalLattice).Build();
		}
		std::shared_ptr<IPhraseList> result = FACTORYCREATE(PhraseList);
		if (isSucceeded) {
			BuildPredictionPhrases(m_prevPrims, &internalLattice, m_nGramTree->GetNGramNode(), m_maxWordsInPhrase, result).Build();
		}
		return result;
	}
	void RegisterPhrase(const std::shared_ptr<IPhrase>& registerPhrase) override
	{
		if (!registerPhrase) return;

		std::vector<HistoryPrimitive*> registerPrims = ConvertPhraseToHistVector(registerPhrase);
		registerPrims.push_back(m_primList->GetEOS());

		std::vector<HistoryPrimitive*> registerSequence;
		size_t primCount = registerPrims.size();
		for (size_t primIdx = 0; primIdx < primCount; ++primIdx) {
			// build register primitive sequence
			registerSequence.clear();
			registerSequence.emplace_back(registerPrims[primIdx]);

			for (size_t prevIdx = primIdx - 1; prevIdx < primIdx; --prevIdx) {
				registerSequence.emplace_back(registerPrims[prevIdx]);
				if (registerSequence.size() >= MAX_NGRAM_LENGTH) {
					break;
				}
			}
			// register with BOS
			if (registerSequence.size() < MAX_NGRAM_LENGTH) {
				registerSequence.emplace_back(m_primList->GetBOS());
				m_nGramTree->InsertReverseSequence(&registerSequence[0], (uint32_t)registerSequence.size(), false);
				registerSequence.pop_back();
			}

			if (registerSequence.size() < MAX_NGRAM_LENGTH && m_prevPrims.size() > 0) {
				size_t determinedCount = m_prevPrims.size();
				for (size_t detIdx = determinedCount - 1; detIdx < determinedCount; --detIdx) {
					registerSequence.push_back(m_prevPrims[detIdx]);
					if (registerSequence.size() >= MAX_NGRAM_LENGTH) {
						break;
					}
				}
			}
			for (uint32_t sequenceLen = (uint32_t)registerSequence.size(); sequenceLen > 0; --sequenceLen) {
				m_nGramTree->InsertReverseSequence(&registerSequence[0], sequenceLen, false);
			}
		}
	}
	void RegisterLastWordWithOpenPhrases(const std::shared_ptr<IPhrase>& registerPhrase) override
	{
		if (!registerPhrase) return;

		std::vector<HistoryPrimitive*> registerPrims = ConvertPhraseToHistVector(registerPhrase);

		size_t primCount = registerPrims.size();
		for (size_t primIdx = 0; primIdx < primCount; ++primIdx) {
			m_nGramTree->InsertSequence(&registerPrims[registerPrims.size() - 1 - primIdx], static_cast<uint32_t>(primIdx + 1), false);
		}
	}
	std::vector<HistoryPrimitive*> ConvertPhraseToHistVector(const std::shared_ptr<IPhrase>& phrase)
	{
		std::vector<HistoryPrimitive*> primList;
		if (phrase) {
			int startPrimitive = 0;
			int endPrimitive = phrase->PrimitiveCount();

			if (endPrimitive > MAX_NGRAM_LENGTH) {
				startPrimitive = endPrimitive - (int)MAX_NGRAM_LENGTH;
			}
			for (int primIdx = startPrimitive; primIdx < endPrimitive; ++primIdx) {
				primList.emplace_back(ConvertPrimitiveToHistory(phrase->Primitive(primIdx).get()));
			}
		}
		return primList;
	}
	HistoryPrimitive* ConvertPrimitiveToHistory(IPrimitive* prim)
	{
		HistoryClass *histClass = m_classList->GetFromClassId(prim->Class());
		return m_primList->AddOrFindWord(prim->Display(), prim->Reading(), histClass, prim->BitFlags(), true);
	}
	std::shared_ptr<IEnglishReinforce> GetReinforce() override
	{
		return m_engRein;
	}

	virtual ~HistoryReuser() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(HistoryReuser);
} } /* namespace Ribbon::History */
