#pragma once
#ifndef _RIBBON_DICTIONARYCONSTRUCTOR_H_
#define _RIBBON_DICTIONARYCONSTRUCTOR_H_
namespace Ribbon { namespace Dictionary {

struct IDictionaryConstructor
{
	virtual void BuildDictionary(WordContainer& allWords, PosContainer& allPos, PhraseContainer& allPhrases, PredCacheContainer& allPredCache) = 0;
};

FACTORYEXTERN(DictionaryConstructor);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_DICTIONARYCONSTRUCTOR_H_
