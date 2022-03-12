#pragma once
#ifndef _RIBBON_HISTORYSTORE_H_
#define _RIBBON_HISTORYSTORE_H_

namespace Ribbon { namespace Dictionary {
	struct IDictionaryReader;
} }
namespace Ribbon { namespace History {

struct HistoryPrimitive;
struct HistoryClass;
struct TrieNode;
struct NGramNode;

struct IHistoryClassList : public IFlexibleBinBase
{
	virtual void Initialize(Dictionary::IDictionaryReader* dictionaryReader) = 0;
	virtual void UpdateUsingMark(uint32_t updateMark) = 0; // need to call before save
	virtual HistoryClass* GetFromClassId(uint16_t classId) = 0;
	virtual HistoryClass* GetFromFileIndex(uint16_t fileIndex) = 0;
	virtual HistoryClass* GetBOS() = 0;
	virtual HistoryClass* GetEOS() = 0;
	virtual HistoryClass* GetUNK() = 0;
};

struct IHistoryPrimList : public IFlexibleBinBase
{
	virtual void Initialize(IHistoryClassList* histClassList) = 0;
	virtual void PrepareBeforeLoading(IHistoryClassList* histClassList) = 0;
	virtual void PrepareBeforeSaving(uint32_t usingMark) = 0;

	virtual HistoryPrimitive* AddOrFindWord(const RefString& displayText, const RefString& readingText, HistoryClass* classPtr, uint32_t bitFlags, bool addIfNotFound) = 0;
	virtual HistoryPrimitive* GetFromFileIndex(uint32_t fileIndex) = 0;
	virtual std::vector<const HistoryPrimitive*> AllPrimitives() = 0;
	virtual TrieNode* GetTrieNode() = 0;
	virtual HistoryPrimitive* GetBOS() = 0;
	virtual HistoryPrimitive* GetEOS() = 0;
};

struct IHistoryNGramTree : public IFlexibleBinBase
{
	virtual void PrepareBeforeLoading(IHistoryPrimList* histPrimList) = 0;
	virtual void InsertSequence(HistoryPrimitive** primList, uint32_t primCount, bool isStrong) = 0;
	virtual void InsertReverseSequence(HistoryPrimitive** primList, uint32_t primCount, bool isStrong) = 0;
	virtual NGramNode* GetNGramNode() = 0;
};

struct IEnglishReinforce : public IFlexibleBinBase
{
	virtual bool IsShowCandidate(const std::shared_ptr<IPhrase> phrase, float candidateScore, int skipChar) = 0;
	virtual void Rewarded(const std::shared_ptr<IPhrase> phrase, int skipChar) = 0;
	virtual void SetPenalty(const std::shared_ptr<IPhrase> phrase, int skipChar) = 0;
};

struct IHistoryFile
{
	virtual void Initialize(const char* filePrefix) = 0;
	virtual void Load(IHistoryClassList*, IHistoryPrimList*, IHistoryNGramTree*, IEnglishReinforce*) = 0;
	virtual void Save(IHistoryClassList*, IHistoryPrimList*, IHistoryNGramTree*, IEnglishReinforce*) = 0;
};

FACTORYEXTERN(HistoryClassList);
FACTORYEXTERN(HistoryPrimList);
FACTORYEXTERN(HistoryNGramTree);
FACTORYEXTERN(EnglishReinforce);
FACTORYEXTERN(HistoryFile);
} } /* namespace Ribbon::History */
#endif // _RIBBON_HISTORYSTORE_H_
