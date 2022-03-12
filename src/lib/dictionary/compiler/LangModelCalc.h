#pragma once
#ifndef _RIBBON_LANGMODELCALC_H_
#define _RIBBON_LANGMODELCALC_H_
#include "DictionaryCompiler.h"
#include "LexiconReader.h"
namespace Ribbon { namespace Dictionary {

struct ILangModelCalculater
{
	virtual void AllowUnknownWords(bool allowUnknownWords) = 0;
	virtual void BuildReverseLink(bool useReverseLink) = 0;
	virtual void CalculateNgramProbability(WordContainer& allWords, PosContainer& allPos, PhraseContainer& allPhrases) = 0;
	virtual void GetWordContainer(WordContainer& allWords) = 0;
};

FACTORYEXTERN(LangModelCalculater);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_LANGMODELCALC_H_
