#include "pch.h"
#include "LiteralConvert.h"

namespace Ribbon {

struct EnglishUtility :
	public std::enable_shared_from_this<EnglishUtility>,
	public ILiteralUtility,
	public IObject
{
	enum class TextState {
		Top,
		Space,
		Alphabet,
		Number,
		Punct,
		Control,
		Other
	};
	static bool IsControl(char16_t ch) { return ch > 0 && ch < 0x20; }
	static bool IsSpace(char16_t ch) { return ch == u' '; }
	static bool IsAlpha(char16_t ch) { return (u'a' <= ch && ch <= u'z') || (u'A' <= ch && ch <= u'Z'); }
	static bool IsNumber(char16_t ch) { return (u'0' <= ch && ch <= u'9'); }
	static bool IsPunct(char16_t ch) { return ch == u'.' || ch == u',' || ch == u'!' || ch == u'?'; }
	static const char16_t* SkipToSpace(const char16_t* src) {
		for (; *src != 0 && !IsSpace(*src); ++src);
		for (; IsSpace(*src); ++src);
		return src;
	}
	static TextState CharacterType(wchar_t ch) {
		if (IsSpace(ch)) return TextState::Space;
		if (IsAlpha(ch)) return TextState::Alphabet;
		if (IsNumber(ch)) return TextState::Number;
		if (IsPunct(ch)) return TextState::Punct;
		if (IsControl(ch)) return TextState::Control;
		return TextState::Other;
	}

	bool NeedToStoreHistory(const RefString& contextString, char16_t nextChar) override {
		TextState lastState = TextState::Top;
		if (contextString.length() > 0) {
			auto lastChar = contextString.u16ptr()[contextString.length() - 1];
			lastState = CharacterType(lastChar);
		}
		TextState nextState = CharacterType(nextChar);

		if (lastState == TextState::Top) {
			return false;
		}
		
		switch (nextState) {
		case TextState::Space:
			return false;
		case TextState::Alphabet:
			return (lastState != TextState::Alphabet);
		case TextState::Number:
			return (lastState != TextState::Number);
		case TextState::Punct:
			return true;
		case TextState::Control:
			return false;
		case TextState::Other:
			return (lastState != TextState::Other);
		}
		return false;
	}

	bool IsNextWordPrediction(const RefString& /*contextString*/, char16_t nextChar) override {
		TextState nextState = CharacterType(nextChar);

		switch (nextState) {
		case TextState::Alphabet:
		case TextState::Number:
		case TextState::Control:
			return false;
		case TextState::Space:
		case TextState::Punct:
		case TextState::Other:
			return true;
		}
		return true;
	}

	std::shared_ptr<IPhrase> WordBreak(const RefString& contextString, char16_t nextChar) override {
		auto wordList = FACTORYCREATE(Phrase);
		TextState textState = TextState::Top;

		const char16_t* srcText = contextString.u16ptr();
		std::u16string srcWorkText;
		if (nextChar != 0 && !IsControl(nextChar)) {
			srcWorkText = contextString.u16str();
			srcWorkText += nextChar;
			srcText = srcWorkText.c_str();
		}

		while (*srcText != 0) {
			switch (textState) {
			case TextState::Top: {
				const char16_t* next = srcText;
				for (; *next != 0 && !IsSpace(*next); ++next);
				wordList->Clear();
				textState = CharacterType(*(srcText = next));
				break;
			}
			case TextState::Space: {
				for (; IsSpace(*srcText); ++srcText);
				textState = CharacterType(*srcText);
				break;
			}
			case TextState::Alphabet: {
				const char16_t* next = srcText;
				for (; IsAlpha(*next); ++next);
				auto word = FACTORYCREATE(Primitive);
				auto text = RefString(srcText, static_cast<size_t>(next - srcText));
				word->Display(text);
				word->Reading(text);
				word->BitFlags((IsSpace(*(srcText - 1)) ? PRIMITIVE_BIT_HEADING_SPACE : 0) |
					(IsSpace(*next) ? PRIMITIVE_BIT_TAILING_SPACE : 0));
				wordList->Push(word);
				textState = CharacterType(*(srcText = next));
				break;
			}
			case TextState::Number: {
				const char16_t* next = srcText;
				for (; IsNumber(*next); ++next);
				auto word = FACTORYCREATE(Primitive);
				auto text = RefString(srcText, static_cast<size_t>(next - srcText));
				word->Display(text);
				word->Reading(text);
				word->BitFlags((IsSpace(*(srcText - 1)) ? PRIMITIVE_BIT_HEADING_SPACE : 0) |
					(IsSpace(*next) ? PRIMITIVE_BIT_TAILING_SPACE : 0));
				wordList->Push(word);
				textState = CharacterType(*(srcText = next));
				break;
			}
			case TextState::Punct: {
				auto word = FACTORYCREATE(Primitive);
				auto text = RefString(srcText, 1);
				word->Display(text);
				word->Reading(text);
				word->BitFlags((IsSpace(*(srcText - 1)) ? PRIMITIVE_BIT_HEADING_SPACE : 0) |
					(IsSpace(*(srcText + 1)) ? PRIMITIVE_BIT_TAILING_SPACE : 0));
				wordList->Push(word);
				textState = CharacterType(*++srcText);
				break;
			}
			case TextState::Control:
			case TextState::Other: {
				const char16_t* next = srcText;
				for (; *next != 0 && !IsSpace(*next); ++next);
				wordList->Clear();
				textState = CharacterType(*(srcText = next));
				break;
			}
			}
		}
		return wordList;
	}

	virtual ~EnglishUtility() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE2(ILiteralUtility, EnglishUtility);
} // Ribbon
