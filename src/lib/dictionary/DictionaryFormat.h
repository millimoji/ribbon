#pragma once
#ifndef _RIBBON_DICTIONARYFORMAT_H_
#define _RIBBON_DICTIONARYFORMAT_H_
namespace Ribbon { namespace Dictionary {

const int maxIndexLen = 255;
const uint8_t maxReadingLetter = 0xA1;

const char16_t TEXT_BOS[] = u"[BOS]";
const char16_t TEXT_EOS[] = u"[EOS]";
const char16_t TEXT_UNK[] = u"[UNK]";
const char TEXT_BOS_UTF8[] = "[BOS]";
const char TEXT_EOS_UTF8[] = "[EOS]";
const char TEXT_UNK_UTF8[] = "[UNK]";

inline void Write24(uint8_t *pb, int val)
{
	pb[0] = (uint8_t)val;
	pb[1] = (uint8_t)(val >> 8);
	pb[2] = (uint8_t)(val >> 16);
}
inline int Read24(const uint8_t *pb)
{
	return (int)pb[0] + ((int)pb[1] << 8) + ((int)pb[2] << 16);
}
inline void Write16(uint8_t *pb, int val)
{
	pb[0] = (uint8_t)val;
	pb[1] = (uint8_t)(val >> 8);
}
inline int Read16(const uint8_t *pb)
{
	return (int)pb[0] + ((int)pb[1] << 8);
}

const int POSID_BOS = 0xF001;
const int POSID_EOS = 0xF002;;
const int POSID_UNK = 0xF003;
const int POSID_UNKALP = 0xF004;;
const int POSID_UNKSYM = 0xF005;
const int POSID_UNKKAT = 0xF006;
const int POSID_NUMJPN = 0xF007;
const int POSID_NUMARA = 0xF008;
const int POSID_BEGIN = 0x1001;

const char16_t PREFIX_POS_IN_WORDNGRAM = L'\v';
const char16_t PREFIX_POS_IN_POSNGRAM = L'\t';

/*
DICTCHAR
3041-3096	=> 01-56	// HIRAGANA
3001		=> 57		// TOUTEN
3002		=> 58		// KUTEN
30FB,30FC	=> 59,5A	// DOT,CHOON
301C		=> 5B		// TILDA
300C,300D	=> 5C,5D	// []
7B-7D		=> 5E-60	// {|}
21-60		=> 61-A0	// ALPHA&SYM
*Free		=> A1-A9 (9)
30**		=> AA **	// KATAKANA TODO: more compact
00**		=> AB **
**00		=> AC **
Other		=> AD ** **
4E00-9FFF	=> AE-FF	// TODO: using JIS is better

Alternative Idea
1 Byte codec

0021-005F => 01-3F
0020      => 40
3041-3096 => 41-96
7B({)     => 97
7C(|)     => 98
7D(})     => 99
7E(~)     => 9A
60(`)     => 9D
Free      => 9E,9F
30A0-30FC => A0-FC
Free      => FD,FE
Switch    => FF

2 Bytes codec

00xx      => F1 xx
xx00      => F2 xx
PUE/SP    => F3 80-FF,80-FF,80-FF
xxyy      => xx yy

// 3041 - 309F  97,98,99,9A,9D,9E,9F
// 30A1 - 30FF  A0,FD,FE,FF
// `{|}~
*/
class DICTCHAR
{
public:
	static int FromWChar(char16_t wch, uint8_t *buf, int bufSiz)
	{
		THROW_IF_FALSE_MSG(bufSiz >= 2, "Not enough buffer");
		if (wch >= 0x3041 && wch <= 0x3096)
		{
			buf[0] = (uint8_t)((wch & 0xFF) - 0x40);
			return 1;
		}
		switch (wch)
		{
		case 0x3001: buf[0] = 0x57; return 1; // TOUTEN
		case 0x3002: buf[0] = 0x58; return 1; // KUTEN
		case 0x30FB: buf[0] = 0x59; return 1; // Nakaguro
		case 0x30FC: buf[0] = 0x5A; return 1; // Choon
		case 0x301C: buf[0] = 0x5B; return 1; // Tilda
		case 0x300C: buf[0] = 0x5C; return 1; // [
		case 0x300D: buf[0] = 0x5D; return 1; // ]
		case 0x007B: buf[0] = 0x5E; return 1; // {
		case 0x007C: buf[0] = 0x5F; return 1; // |
		case 0x007D: buf[0] = 0x60; return 1; // }
		}
		if (wch >= 0x21 && wch <= 0x60) {
			buf[0] = (uint8_t)(wch + 0x40);
			return 1;
		}
		THROW_IF_FALSE_MSG(bufSiz >= 3, "Not enough buffer");
		if (wch >= 0x3001 && wch <= 0x30FF)
		{
			buf[0] = 0xAA;
			buf[1] = (uint8_t)wch;
			return 2;
		}
		if (wch < 0x100)
		{
			buf[0] = 0xAB;
			buf[1] = (uint8_t)wch;
			return 2;
		}
		if ((wch & 0xFF) == 0)
		{
			buf[0] = 0xAC;
			buf[1] = (uint8_t)(wch >> 8);
			return 2;
		}
		if (wch >= 0x4E00 && wch <= 0x9FFF)
		{
			buf[0] = (uint8_t)((wch - 0x4E00 + 0xAE00) >> 8);
			buf[1] = (uint8_t)wch;
			return 2;
		}
		THROW_IF_FALSE_MSG(bufSiz >= 4, "Not enough buffer");
		buf[0] = 0xAD;
		buf[1] = (uint8_t)(wch >> 8);
		buf[2] = (uint8_t)wch;
		return 3;
	}
	static std::tuple<char16_t, short> ToWChar(const uint8_t *src)
	{
		static char16_t range57_60[] = { 0x3001, 0x3002, 0x30FB, 0x30FC, 0x301C, 0x300C,  0x300D,  0x007B, 0x007C, 0x007D };
		if (src[0] <= 0x56) { return std::tuple<char16_t, short>((char16_t)(0x3040 + src[0]), (short)1); }
		if (src[0] >= 0xAE) { return std::tuple<char16_t, short>((char16_t)((src[0] - 0x60) * 0x100 + src[1]), (short)2); }
		if (src[0] <= 0x60) { return std::tuple<char16_t, short>(range57_60[src[0] - 0x57], (short)1); }
		if (src[0] <= 0xA0) { return std::tuple<char16_t, short>((char16_t)(src[0] - 0x40), (short)1); }
		THROW_IF_FALSE_MSG(src[0] >= 0xAA, "Unknown DiCT CHAR");
		if (src[0] == 0xAA) { return std::tuple<char16_t, short>((char16_t)(0x3000 + src[1]), (short)2); }
		if (src[0] == 0xAB) { return std::tuple<char16_t, short>((char16_t)src[1], (short)2); }
		if (src[0] == 0xAC) { return std::tuple<char16_t, short>((char16_t)(src[1] * 0x100), (short)2); }
		if (src[0] == 0xAD) { return std::tuple<char16_t, short>((char16_t)(src[1] * 0x100 + src[2]), (short)3); }
		THROW_MSG("Unknown DICT CHAR");
	}
	static void FromWStr(const char16_t* src, uint8_t* dst, int dstLen)
	{
		for (; *src != 0 && dstLen > 0; ++src)
		{
			THROW_IF_FALSE_MSG(dstLen >= 1, "not enough buffer");

			int writtenChar = FromWChar(*src, dst, dstLen);
			dstLen -= writtenChar;
			dst += writtenChar;
		}
		*dst = 0;
	}
	static void ToWStr(const uint8_t* src, char16_t* dst, int dstLen)
	{
		char16_t* endPos = dst + dstLen - 1;
		while (*src != 0)
		{
			THROW_IF_FALSE_MSG(dst < endPos, "not enough buffer");

			auto result = ToWChar(src);
			*dst++ = std::get<0>(result);
			src += std::get<1>(result);
		}
		*dst = 0;
	}
};

const int SDICTBLOCKSIZE = 4096;

const uint32_t DICTFORMAT_VERSION          = 0x00010001;
const uint32_t DICTTYPE_SYSTEM             = 0x10000000;
const uint32_t BLOCKTYPE_PROBABILITY_TABLE = 0x10000000;
const uint32_t BLOCKTYPE_DOUBLEARRAY       = 0x20000000;
const uint32_t BLOCKTYPE_VOCABURARY        = 0x30000000;
const uint32_t BLOCKTYPE_BIGRAM            = 0x40000000;
const uint32_t BLOCKTYPE_TRIGRAM           = 0x50000000;
const uint32_t BLOCKTYPE_POSNAMETEXT       = 0xE0000000;
const uint32_t BLOCKTYPE_END               = 0xF0000000;

struct SDICTHEAD
{
	char description[128];
	UUID sdictUuid;				// for format
	UUID thisUuid;					// for identify build

	uint32_t formatVersion;
	uint32_t dictType;
	uint32_t blockCount;

	uint32_t buildYear : 12;
	uint32_t buildMonth : 4;
	uint32_t buildDay : 5;
	uint32_t buildHour : 5;
	uint32_t buildMinutes : 6;

	struct BLOCKATTR
	{
		uint32_t blockType;
		uint32_t blockOffset;
		uint32_t blockSize;
		uint32_t _free;
	} blockList[1];

	SDICTHEAD(const SDICTHEAD&) = delete;
	SDICTHEAD& operator = (const SDICTHEAD&) = delete;
};

const UUID UUID_DICTIONARY = UUID(0x6135657a, 0xa8a5, 0x45a1, 0xa3, 0x33, 0x8a, 0x2b, 0x80, 0xfc, 0xcb, 0xe3);
 
struct BLOCK_PROBABILITY
{
	float	posInside[256];
	float	posPhrase[256];
	float	posBigram[256];
	float	posTrigram[256];
	float	wordBigram[256];
	float	wordTrigram[256]; // TODO:Backoff
	float	backOff[256];
};

struct BaseCheck3232
{
	uint32_t	base;
	uint32_t	check;
};

// Vocaburary block
const uint8_t VOCAB_ID_TAILSTRING = 1;
const uint8_t VOCAB_ID_WORD = 2;
const uint8_t VOCAB_ID_DISPLAY = 3;
const uint8_t VOCAB_ID_WORDARRAY = 4;
const uint8_t VOCAB_ID_PREDICTIONCACHE = 5;
const uint8_t VOCAB_ID_TERMINATION = 7;

struct VOCAB_BASE;
struct VOCAB_TAILSTRING;
struct VOCAB_WORD;
struct VOCAB_DISPLAY;
struct VOCAB_WORDARRAY;
struct VOCAB_TERMINATION;

struct VOCAB_BASE
{
	uint8_t		anyVal : 5;
	uint8_t		id : 3;
};

struct VOCAB_TAILSTRING
{
	uint8_t		length : 5;
	uint8_t		id : 3;
	uint8_t		tailString[1];

	static const int maxTailLen = 32;
	const VOCAB_BASE* GoNext() const
	{
		return (const VOCAB_BASE*)&tailString[(int)length + 1];
	}
};

struct VOCAB_WORD
{
	uint8_t		hasPos : 1;
	uint8_t		isBigramHead : 1;
	uint8_t		isNgramTail : 1;
	uint8_t		hasPhraseScore : 1;
	uint8_t		_free : 1;
	uint8_t		id : 3;

	uint8_t		display[3];
	uint8_t		posInside;
	//uint8_t	pos[2];		// TODO: 1 or 2
	//uint8_t	posBackOff;
	//uint8_t	bigram[2][3];
	//unit8_t	phraseScore;

	const VOCAB_BASE* GoNext() const
	{
		const uint8_t* myEnd = (const uint8_t*)(this + 1);
		myEnd += (hasPos ? 2 : 0);
		myEnd += (isBigramHead ? (sizeof(uint8_t[2][3]) + 1) : 0);
		myEnd += (hasPhraseScore ? sizeof(uint8_t) : 0);
		return (const VOCAB_BASE*)myEnd;
	}
};

static const int maxWordsInVocab = 32;

struct VOCAB_DISPLAY
{
	uint8_t		count : 5;
	uint8_t		id : 3;
	uint8_t		wordIndex[1][3];

	const VOCAB_BASE* GoNext() const
	{
		return (const VOCAB_BASE*)(wordIndex[count + 1]);
	}
};

struct VOCAB_WORDARRAY
{
	uint8_t		count : 5;
	uint8_t		id : 3;
	uint8_t		posInside;
	uint8_t		wordIndex[1][3];

	const VOCAB_BASE* GoNext() const
	{
		return (const VOCAB_BASE*)(wordIndex[count + 1]);
	}
};

struct VOCAB_PREDICTIONCACHE
{
	uint8_t		count : 5;
	uint8_t		id : 3;
	uint8_t		wordIndex[1][3];

	const VOCAB_BASE* GoNext() const
	{
		return (const VOCAB_BASE*)(wordIndex[count + 1]);
	}
};

struct VOCAB_TERMINATION
{
	uint8_t		hasReadingIdx : 1;
	uint8_t		hasBigramEnd : 1;
	uint8_t		_free : 3;
	uint8_t		id : 3;
	//uint8_t		readingIndex[3];
	//uint8_t		bigramEnd[3]

	const VOCAB_BASE* GetNext() const
	{
		const uint8_t *myEnd = (uint8_t*)this + sizeof(*this);
		myEnd += (hasReadingIdx ? 3 : 0) + (hasBigramEnd ? 3 : 0);
		return (const VOCAB_BASE*)myEnd;
	}
	static const VOCAB_TERMINATION* GoEnd(const uint8_t* ptr)
	{
		const VOCAB_BASE* vocab = (const VOCAB_BASE*)ptr;
		for (;;)
		{
			switch (vocab->id)
			{
			case VOCAB_ID_TAILSTRING:
				vocab = ((const VOCAB_TAILSTRING*)vocab)->GoNext(); break;
			case VOCAB_ID_WORD:
				vocab = ((const VOCAB_WORD*)vocab)->GoNext(); break;
			case VOCAB_ID_DISPLAY:
				vocab = ((const VOCAB_DISPLAY*)vocab)->GoNext(); break;
			case VOCAB_ID_WORDARRAY:
				vocab = ((const VOCAB_WORDARRAY*)vocab)->GoNext(); break;
			case VOCAB_ID_PREDICTIONCACHE:
				vocab = ((const VOCAB_PREDICTIONCACHE*)vocab)->GoNext(); break;
			case VOCAB_ID_TERMINATION:
				return (const VOCAB_TERMINATION*)vocab;
			default:
				THROW_MSG("Unknown ID");
			}
		}
	}
};

struct BIGRAM_ENTRY
{
	uint32_t			wordId : 24;
	uint32_t			bigram : 8;
};

struct BLOCK_POSNAMETEXT
{
	uint32_t			countOfPos;
	struct PosIdAndOffset {
		uint16_t		posID;
		uint16_t		textOffset;
	}					posList[1];
};

} /* namespace Dictionary */ } /* namespace Ribbon */
#endif // _RIBBON_DICTIONARYFORMAT_H_
