#pragma once
#ifndef _RIBBON_DECODER_H_
#define _RIBBON_DECODER_H_
#include "pch.h"
#include "DictionaryReader.h"

namespace Ribbon { namespace Transliteration {

struct DECODERARAM
{
	static const int N_BEST_CONVERSION = 1;
	static const int CANDIDATE_LIST = 2;

	int decodeMode = N_BEST_CONVERSION;
	int candidateTop = -1;
	int candidateEnd = 1;

	Dictionary::IDictionaryReader* dictionary;

	DECODERARAM() {}
	DECODERARAM(const DECODERARAM& src) = delete;
	DECODERARAM& operator = (const DECODERARAM& src) = delete;
};

struct IDecoder
{
	virtual std::shared_ptr<IPhraseList> Decode(const DECODERARAM& param, ILattice* inputLattice) = 0;
};

FACTORYEXTERN(Decoder);
} /* namespace Transliteration */ } /* namespace Ribbon */
#endif // _RIBBON_DECODER_H_
