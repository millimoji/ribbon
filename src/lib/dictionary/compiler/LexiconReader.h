#pragma once
#ifndef _RIBBON_LEXICONREADER_H_
#define _RIBBON_LEXICONREADER_H_
#include "DictionaryCompiler.h"
namespace Ribbon { namespace Dictionary {

struct ILexiconRegister
{
	virtual void RegisterWord(const WORDINFO* wordInfo) = 0;
	virtual void SetNgram(int nGram, const WORDINFO* wordList, uint64_t count) = 0;
	virtual void SetPhrase(int nGram, const WORDINFO* wordList, uint64_t count) = 0;
};

struct ILexiconReader
{
	virtual void ReadSources(ILexiconRegister* /*lexiconRegister*/, int /*nGram*/, int /*cutOffLine*/) {}
	virtual void ReadPhraseList(ILexiconRegister* /*lexiconRegister*/, int /*cutOffLine*/) {}
	virtual std::u16string WordInfoToString(const WORDINFO*) { return std::u16string(); }
};

FACTORYEXTERN(LexiconReader);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_LEXICONREADER_H_
