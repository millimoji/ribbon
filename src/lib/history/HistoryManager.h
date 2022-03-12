#pragma once
#ifndef _RIBBON_HISTORYMANAGER_H_
#define _RIBBON_HISTORYMANAGER_H_
namespace Ribbon { namespace Dictionary {
struct IDictionaryReader;
} }
namespace Ribbon { namespace History {

struct IHistoryReuser;

struct IHistoryManager
{
	virtual void Initialize(Dictionary::IDictionaryReader* dictionaryReader, const char* filePrefix, int maxWordsInPhrase) = 0;
	virtual std::shared_ptr<IHistoryReuser> GetReuser() = 0;
	virtual void Save() = 0;
	virtual void Load() = 0;
};

FACTORYEXTERN(HistoryManager);
} }/* namespace Ribbon::History */
#endif // _RIBBON_HISTORYMANAGER_H_
