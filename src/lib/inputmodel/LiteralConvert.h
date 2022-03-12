#pragma once
#ifndef _RIBBON_LITERALCONVERT_H_
#define _RIBBON_LITERALCONVERT_H_
namespace Ribbon {

struct ILiteralProvider
{
	virtual RefString GetSourceBeforeCaret() = 0;
	virtual void Apply(int fromPos, int length, RefString newText) = 0;
	virtual void Finalize() = 0;
};

struct IEditLine;
struct KeyEvent;

// Japanese
struct IJaLiteralConvert
{
	virtual void Jpn12keyKanaInput(IEditLine*, const std::shared_ptr<KeyEvent>&) = 0;
	virtual void Jpn12keyModifierInput(IEditLine*, const std::shared_ptr<KeyEvent>&) = 0;
	virtual void Jpn12keyBreakersInput(IEditLine*, const std::shared_ptr<KeyEvent>&) = 0;
	virtual void Jpn12keyBackToggleInput(IEditLine*, const std::shared_ptr<KeyEvent>&) = 0;
};

struct ILiteralConvert
{
	virtual RefString ConvertText(const RefString&) const = 0;
	virtual void ConvertStream(ILiteralProvider*) const = 0;

	virtual std::shared_ptr<IJaLiteralConvert> GetJaLiteralConvert() = 0;
};

struct ILiteralUtility
{
	virtual bool NeedToStoreHistory(const RefString& contextString, char16_t nextChar) = 0;
	virtual bool IsNextWordPrediction(const RefString& contextString, char16_t nextChar) = 0;
	virtual std::shared_ptr<IPhrase> WordBreak(const RefString& contextString, char16_t nextChar) = 0;
};

FACTORYEXTERN2(ILiteralConvert, JaLiteralConvert);
FACTORYEXTERN2(ILiteralUtility, EnglishUtility);
} // Ribbon
#endif //_RIBBON_LITERALCONVERT_H_
