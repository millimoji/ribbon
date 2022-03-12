#pragma once
#ifndef _RIBBON_DICTIONARYCOMPILER_H_
#define _RIBBON_DICTIONARYCOMPILER_H_
#include "../DictionaryFormat.h"
namespace Ribbon { namespace Dictionary {

struct POSPRIM;
struct POSLINK;
struct WORDPRIM;
struct WORDLINK;
struct WORDGROUP;
struct WORDINFO;

static const char16_t POSID_BOS_IN_WORDNGRAM[] = { PREFIX_POS_IN_WORDNGRAM , POSID_BOS , 0 };
static const char16_t POSID_EOS_IN_WORDNGRAM[] = { PREFIX_POS_IN_WORDNGRAM , POSID_EOS , 0 };

struct POSLINK
{
	// raw data
	POSPRIM					*rawPosPrim;
	uint64_t					rawCount;	// used count in corpus
	std::map<POSPRIM*, POSLINK>	rawNext;

	// calculated values
	double					probNgram;
	double					probBackOff;


	POSLINK() : rawPosPrim(nullptr), rawCount(0LL), probNgram(0.0), probBackOff(1.0) {}
	POSLINK(POSLINK&& src) :
		rawPosPrim(src.rawPosPrim), rawCount(src.rawCount), rawNext(std::move(src.rawNext)),
		probNgram(src.probNgram), probBackOff(src.probBackOff) {}
	POSLINK& operator = (POSLINK&& src) {
		rawPosPrim = src.rawPosPrim; rawCount = src.rawCount; rawNext = std::move(src.rawNext);
		probNgram = src.probNgram; probBackOff = src.probBackOff;
		return *this;
	}

	POSLINK(const POSLINK& src) = delete;
	POSLINK& operator = (const POSLINK& src) = delete;
};

struct POSPRIM : public POSLINK
{
	// raw data
	std::u16string			rawPosName;
	int						rawPosID;

	int						allWordCount;		// belonging word in dict src
	int						unigramWordCount;	// belonging word in corpus
	bool					isNgramTail;

	// address assignemnt
	int						memoryOffset;
	int						bigramOffset;

	POSPRIM() : rawPosID(0), allWordCount(0), unigramWordCount(0), isNgramTail(false) {}

	POSPRIM(const POSPRIM& src) = delete;
	POSPRIM& operator = (const POSPRIM& src) = delete;
};

struct WORDLINK
{
	// raw data
	WORDLINK				*rawParentNode;
	WORDPRIM				*rawWordPrim;
	uint64_t				rawCount;
	std::map<WORDPRIM*, WORDLINK> rawNext;

	// calculated
	uint64_t				discountedCount;	// only for phrase filter
	double					probNgram;
	double					probBackOff;		// Tri->Bi back off

	// prediction filter
	std::vector<WORDLINK*>	predictionList;

	WORDLINK() : rawParentNode(nullptr), rawWordPrim(nullptr),
		rawCount(0LL), discountedCount(0ULL),
		probNgram(0.0), probBackOff(1.0)
	{}
	WORDLINK(WORDLINK&& src) :
		rawParentNode(src.rawParentNode),
		rawWordPrim(src.rawWordPrim), rawCount(src.rawCount), rawNext(std::move(src.rawNext)),
		discountedCount(src.discountedCount), probBackOff(src.probBackOff) {}
	WORDLINK& operator = (WORDLINK&& src) {
		rawParentNode = src.rawParentNode;
		rawWordPrim = src.rawWordPrim; rawCount = src.rawCount; rawNext = std::move(src.rawNext);
		probBackOff = src.probBackOff;
		discountedCount = src.discountedCount;
		return *this;
	}

	WORDLINK(const WORDLINK&) = delete;
	WORDLINK& operator = (const WORDLINK&) = delete;
};

struct WORDPRIM : public WORDLINK
{
	// raw data
	std::u16string			rawReading;
	std::u16string			rawDisplay;
	POSPRIM					*rawPos;
	bool					isNgramTail;
	bool					isPrediciton;

	// reverse link for prediction
	WORDLINK				reverseInk;

	// calculated
	double					probPosInside;
	double					probPosBackOff;	// Bi->POS back off
	double					probPredictionScore;

	// address assignemnt
	int						memoryOffset;
	int						readingNode;
	int						displayNode;
	int						bigramOffset;

	WORDPRIM() : rawPos(nullptr),
		isNgramTail(false), isPrediciton(false),
		probPosInside(0.0), probPosBackOff(1.0), probPredictionScore(0.0)
	{
		rawWordPrim = this;
		reverseInk.rawWordPrim = this;
		reverseInk.rawParentNode = nullptr;
	}
	WORDPRIM(const WORDPRIM&) = delete;
	WORDPRIM& operator = (const WORDPRIM&) = delete;
};

struct WORDINFO
{
	const char16_t*			read;
	const char16_t*			disp;
	const char16_t*			pos;

	bool operator < (const WORDINFO& dst) const {
		int val = textcmp(disp, dst.disp);
		if (val != 0) return val < 0;
		val = textcmp(read, dst.read);
		if (val != 0) return val < 0;
		return textcmp(pos, dst.pos) < 0;
	}
};

struct PHRASEDATA
{
	// raw data
	std::u16string			rawReading;
	std::vector<WORDPRIM*>	rawWordList;
	uint64_t				rawCount;

	// calculated
	double					probPosInside;

											// address assignemnt
	int						memoryOffset;
	int						readingNode;
};

struct PREDICTIONCACHE
{
	std::u16string			rawReading;
	std::vector<PHRASEDATA*>	predictionItems;
};

struct LETTERGROUP
{
	const char16_t*			orgLetter = nullptr;
	std::string				fullLetter;
	std::string				indexPart;
	const uint8_t*			tailPart;

	// address assignemnt
	size_t					memoryOffset;
	int						letterNode;

	std::vector<WORDPRIM*>	readList;
	std::vector<WORDPRIM*>	dispList;
	std::vector<POSPRIM*>	posList;
	std::vector<PHRASEDATA*> phraseList;
	std::vector<PREDICTIONCACHE*> predCacheList;
};

typedef std::map<const char16_t*, std::unique_ptr<POSPRIM>, textCompare<char16_t>> PosContainer;
typedef std::map<WORDINFO, std::unique_ptr<WORDPRIM>> WordContainer;
typedef std::vector<std::unique_ptr<PHRASEDATA>> PhraseContainer;
typedef std::vector<std::unique_ptr<PREDICTIONCACHE>> PredCacheContainer;

FACTORYEXTERN2(IProgramMain, DictinoaryCompiler);
} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_DICTIONARYCOMPILER_H_
