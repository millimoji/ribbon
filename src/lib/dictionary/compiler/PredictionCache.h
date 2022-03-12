#pragma once
#ifndef _RIBBON_PREDICTINOCACHE_H_
#define _RIBBON_PREDICTINOCACHE_H_
#include "../DictionaryFormat.h"
#include "DictionaryCompiler.h"
namespace Ribbon { namespace Dictionary {

struct IPredictionCacheMaker
{
	virtual void Build(PhraseContainer& phrases, PredCacheContainer& predCaches) = 0;
};

FACTORYEXTERN(PredictionCacheMaker);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_PREDICTINOCACHE_H_
