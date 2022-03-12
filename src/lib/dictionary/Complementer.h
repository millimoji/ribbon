#pragma once
#ifndef _RIBBON_COMPLEMENTER_H_
#define _RIBBON_COMPLEMENTER_H_
namespace Ribbon {
	struct IPhraseList;
	struct ILattice;

	namespace Dictionary {
		struct IDictionaryReader;
	}
}
namespace Ribbon { namespace Transliteration {

struct COMPLEMENTPARAM
{
	Dictionary::IDictionaryReader* dictionaryReader;
	int maxPredictionItems;
	int determinedFrame;
};

struct IComplementer
{
	virtual std::shared_ptr<IPhraseList> Complement(const COMPLEMENTPARAM& param, ILattice* lattice) = 0;
};

FACTORYEXTERN(Complementer);
} /* namespace Transliteration */ } /* namespace Ribbon */
#endif // _RIBBON_COMPLEMENTER_H_
