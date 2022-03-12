#include "pch.h"
#include "PredictionCache.h"
namespace Ribbon { namespace Dictionary {

struct PredictionCacheMaker :
	public std::enable_shared_from_this<PredictionCacheMaker>,
	public IPredictionCacheMaker,
	public IObject
{
	int m_maxReading;
	int m_maxPredItem;

	struct TRIENODE
	{
		std::map<char16_t, TRIENODE> children;
		std::vector<PHRASEDATA*> preditions;
	};
	TRIENODE m_trieRoot;

	void Build(PhraseContainer& phrases, PredCacheContainer& predCaches) override
	{
		{
			std::shared_ptr<ISetting> setting = Platform->GetSettings();
			m_maxReading = setting->GetInt("Dictionaries", "PredictionCacheLevel");
			m_maxPredItem = setting->GetInt("Dictionaries", "PredictionPerEntry");
		}

		// make character trie
		for (auto& phraseItem : phrases)
		{
			size_t maxLen = std::min((size_t)m_maxReading, phraseItem->rawReading.length() - 1);
			TRIENODE* trieNode = &m_trieRoot;
			for (size_t ich = 0; ich < maxLen; ++ich)
			{
				auto insRes = trieNode->children.insert(
					std::make_pair(phraseItem->rawReading[ich], TRIENODE()));

				trieNode = &insRes.first->second;
				trieNode->preditions.push_back(phraseItem.get());
			}
		}

		for (auto& mapPair : m_trieRoot.children)
		{
			std::u16string indexText;
			indexText += mapPair.first;
			RecursivelyLimitPrediction(indexText, &mapPair.second, predCaches);
		}
	}

	void RecursivelyLimitPrediction(const std::u16string& prefixText, TRIENODE* trieNode, PredCacheContainer& predCaches)
	{
		std::sort(trieNode->preditions.begin(), trieNode->preditions.end(),
			[](const PHRASEDATA* lhs, const PHRASEDATA* rhs)
			{
				return lhs->rawCount > rhs->rawCount;
			});

		if (trieNode->preditions.size() > (size_t)m_maxPredItem)
		{
			trieNode->preditions.resize((size_t)m_maxPredItem);
		}
		if (trieNode->preditions.size() > 0)
		{
			std::unique_ptr<PREDICTIONCACHE> predCaheItem(new PREDICTIONCACHE);
			predCaheItem->rawReading = prefixText;
			predCaheItem->predictionItems = std::move(trieNode->preditions);
			predCaches.push_back(std::move(predCaheItem));
		}

		for (auto& mapPair : trieNode->children)
		{
			std::u16string indexText(prefixText);
			indexText += mapPair.first;
			RecursivelyLimitPrediction(indexText, &mapPair.second, predCaches);
		}
	}

	virtual ~PredictionCacheMaker() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(PredictionCacheMaker);
} /* namespace Dictionary */ } /* namespace Ribbon */
