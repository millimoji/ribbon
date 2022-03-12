#pragma once
#ifndef _RIBBON_EDITLINE_H_
#define _RIBBON_EDITLINE_H_
#include "LiteralConvert.h"
namespace Ribbon {

const uint32_t TextRangeType_RawInput			= 0x01;
const uint32_t TextRangeType_PhoneticText		= 0x02;
const uint32_t TextRangeType_Transliterated		= 0x03;
const uint32_t TextRangeType_StateMask			= 0x0F;
const uint32_t TextRangeType_Generated			= 0x10;
const uint32_t TextRangeType_PhoneticSource		= 0x20;
const uint32_t TextRangeType_Translated			= 0x30;
const uint32_t TextRangeType_AttributeMask		= 0xF0;


struct ITextRange
{
	virtual RefString VisualText() const = 0;
	virtual int VisualLength() const = 0;
	virtual uint32_t RangeType() const = 0;
	virtual int SubRangeCount() const = 0;
	virtual std::shared_ptr<ITextRange> SubRange(int rangeIndex) const = 0;
};

struct IEditLine
{
	// getter
	virtual bool IsEmpty() const = 0;
	virtual RefString GetPlainText() const = 0;
	virtual std::shared_ptr<IPhrase> GetSegmentedText() const = 0;
	virtual std::tuple<int, int> GetCaret() const = 0;
	virtual int GetVisualLength() const = 0;
	virtual std::tuple<std::shared_ptr<ITextRange>, int> GetRange(int pos) const = 0;
	virtual std::shared_ptr<ILattice> CreateInputLattice() const = 0;
	virtual RefString GetCaretText() const = 0;

	// modifier
	virtual bool GetAndResetDirtyFlag() = 0;
	virtual void Clear() = 0;
	virtual void SetCaret(int pos, int length) = 0;
	virtual void UpdateText(RefString text, uint32_t rangeType) = 0;
	virtual void DeleteText() = 0;

	virtual std::shared_ptr<ILiteralProvider> GetLiteralProvider() = 0;
};

FACTORYEXTERN(EditLine);
} // Ribbon
#endif // _RIBBON_EDITLINE_H_
