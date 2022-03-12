#pragma once
#ifndef _RIBBON_EMOJILIST_H_
#define _RIBBON_EMOJILIST_H_
namespace Ribbon {

enum class SymbolEmojiCategory
{
	Human = 0,
	Animal,
	Food,
	TimeRegion,
	Game,
	Items,
	SymbolMarks,
	Flags,
	SymStandard,
	SymParenthes,
	SymArrows,
	SymMath,
	Numbers,
	Images,
	FullWidth,
	Max,
};

struct ISymbolEmojiList
{
	virtual std::shared_ptr<IPhraseList> GetList(SymbolEmojiCategory category, uint16_t classId) = 0;
};

FACTORYEXTERN(SymbolEmojiList);
} // Ribbon
#endif // _RIBBON_EMOJILIST_H_
