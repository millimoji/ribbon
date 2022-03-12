#pragma once
#ifndef _RIBBON_PHRASECOUNTER_H_
#define _RIBBON_PHRASECOUNTER_H_
#include "../compiler/DictionaryCompiler.h"
namespace Ribbon { namespace Dictionary {

struct IPhraseCounter
{
	virtual void DoCount(WordContainer& /*wordContainer*/, int /*cutOffLine*/) {}
};

FACTORYEXTERN(PhraseCounter);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_PHRASECOUNTER_H_
