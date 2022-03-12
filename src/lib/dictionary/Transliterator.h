#pragma once
#ifndef _RIBBON_TRANSLITERATOR_H_
#define _RIBBON_TRANSLITERATOR_H_
namespace Ribbon { namespace Dictionary {
struct IDictionaryReader;
} }
namespace Ribbon { namespace Transliteration {

struct ITransliterator
{
	virtual std::shared_ptr<Dictionary::IDictionaryReader> GetDictinoaryReader() = 0;
	virtual std::shared_ptr<ILattice> StringToInputLattice(const char16_t* srcText) = 0;
	virtual RefString SimpleStringConversion(const char16_t* srcText) = 0;
	virtual RefString SimpleStringPrediction(const char16_t* srcText) = 0;
	virtual std::shared_ptr<IPhraseList> Query(ILattice* lattice) = 0;
};

FACTORYEXTERN(Transliterator);
} /* namespace Transliteration */ } /* namespace Ribbon */
#endif // _RIBBON_TRANSLITERATOR_H_
