#pragma once
#ifndef _RIBBON_DOUBLEARRAY_H_
#define _RIBBON_DOUBLEARRAY_H_
#include "../DictionaryFormat.h"
namespace Ribbon { namespace Dictionary {

struct IWArrayMaker
{
	virtual void BuildFlat(std::vector<const uint8_t*>& keys, bool isUniqueBase = false) = 0;
	virtual void UpdateBaseValue(const std::function<int(int, int)>& calllBack) = 0;
	virtual void GetBaseCheck3232(std::vector<BaseCheck3232>& wArray) const = 0;
};

FACTORYEXTERN(WArrayMaker);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_DOUBLEARRAY_H_
