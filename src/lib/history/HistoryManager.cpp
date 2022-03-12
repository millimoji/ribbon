#include "pch.h"
#include "HistoryManager.h"
#include "HistoryStruct.h"
#include "HistoryReuser.h"
#include "HistoryStore.h"

namespace Ribbon { namespace History {

struct HistoryManager :
	public std::enable_shared_from_this<HistoryManager>,
	public IHistoryManager,
	public IObject
{
	std::shared_ptr<IHistoryReuser> m_reuser;
	std::shared_ptr<IHistoryFile> m_historyFile;
	std::shared_ptr<IHistoryClassList> m_classList;
	std::shared_ptr<IHistoryPrimList> m_primList;
	std::shared_ptr<IHistoryNGramTree> m_nGramTree;
	std::shared_ptr<IEnglishReinforce> m_englishReinforce;

	HistoryManager() :
		m_reuser(FACTORYCREATE(HistoryReuser)),
		m_historyFile(FACTORYCREATE(HistoryFile)),
		m_classList(FACTORYCREATE(HistoryClassList)),
		m_primList(FACTORYCREATE(HistoryPrimList)),
		m_nGramTree(FACTORYCREATE(HistoryNGramTree)),
		m_englishReinforce(FACTORYCREATE(EnglishReinforce))
	{
	}

	void Initialize(Dictionary::IDictionaryReader* dictionaryReader, const char* filePrefix, int maxWordsInPhrase) override {
		m_classList->Initialize(dictionaryReader);
		m_primList->Initialize(m_classList.get());
		m_reuser->Initialize(m_classList.get(), m_primList.get(), m_nGramTree.get(), m_englishReinforce.get(), maxWordsInPhrase );
		m_historyFile->Initialize(filePrefix);
	}

	std::shared_ptr<IHistoryReuser> GetReuser() override {
		return m_reuser;
	}

	void Save() override {
		m_historyFile->Save(m_classList.get(), m_primList.get(), m_nGramTree.get(), m_englishReinforce.get());
	}
	void Load() override {
		m_historyFile->Load(m_classList.get(), m_primList.get(), m_nGramTree.get(), m_englishReinforce.get());
	}

	virtual ~HistoryManager() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(HistoryManager);
} } /* namespace Ribbon::History */
