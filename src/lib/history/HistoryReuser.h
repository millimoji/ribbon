#pragma once
#ifndef _RIBBON_HISTORYREUSER_H_
#define _RIBBON_HISTORYREUSER_H_
namespace Ribbon { namespace History {

struct IHistoryClassList;
struct IHistoryPrimList;
struct IHistoryNGramTree;
struct IEnglishReinforce;

struct IHistoryReuser
{
	virtual void Initialize(IHistoryClassList*, IHistoryPrimList*, IHistoryNGramTree*, IEnglishReinforce*, int maxWordsInPhrase) = 0;
	virtual void SetContext(const std::shared_ptr<IPhrase>& previousPhrase) = 0;
	virtual std::shared_ptr<IPhraseList> Query(const std::shared_ptr<ILattice>& inputLattice) = 0;
	virtual void RegisterPhrase(const std::shared_ptr<IPhrase>& registerPhrase) = 0;
	virtual void RegisterLastWordWithOpenPhrases(const std::shared_ptr<IPhrase>&  registerPhrase) = 0;
	virtual std::shared_ptr<IEnglishReinforce> GetReinforce() = 0;
};

FACTORYEXTERN(HistoryReuser);
} } /* namespace Ribbon::History */
#endif // _RIBBON_HISTORYREUSER_H_
