#pragma once
#ifndef _RIBBON_HISTORYCLASS_H_
#define _RIBBON_HISTORYCLASS_H_

// forward reference
namespace Ribbon::Dictionary {
struct IDictionaryReader;
}

namespace Ribbon::History {

struct IClassList : public IFlexibleBinBase
{
	virtual void Initialize(Ribbon::Dictionary::IDictionaryReader* dictionaryReader) = 0;
	virtual uint16_t GetDictionaryClassId(uint32_t classIdInFile) = 0;
};

FACTORYEXTERN(ClassList);
} /* namespace Ribbon::History */
#endif // _RIBBON_HISTORYCLASS_H_
