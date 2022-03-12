#include "pch.h"
#include "LiteralConvert.h"
#include "InputModel.h"
#include "EditLine.h"

namespace Ribbon {


namespace { // anonymouse name space to hide ata

struct KanaData {
	char16_t	base;
	char16_t	next;
	char16_t	small;
	char16_t	dull;
	char16_t	halfdull;
};

constexpr KanaData s_kanaData[] = {
{	u'あ',	u'い',	u'ぁ',	0,		0 },
{	u'い',	u'う',	u'ぃ',	0,		0 },
{	u'う',	u'え',	u'ぅ',	u'ゔ',	0 },
{	u'え',	u'お',	u'ぇ',	0,		0 },
{	u'お',	u'あ',	u'ぉ',	0,		0 },
{	u'か',	u'き',	0 /*u'ヵ'*/,	u'が',	0 },
{	u'き',	u'く',	0,		u'ぎ',	0 },
{	u'く',	u'け',	0,		u'ぐ',	0 },
{	u'け',	u'こ',	0 /*u'ヶ'*/,	u'げ',	0 },
{	u'こ',	u'か',	0,		u'ご',	0 },
{	u'さ',	u'し',	0,		u'ざ',	0 },
{	u'し',	u'す',	0,		u'じ',	0 },
{	u'す',	u'せ',	0,		u'ず',	0 },
{	u'せ',	u'そ',	0,		u'ぜ',	0 },
{	u'そ',	u'さ',	0,		u'ぞ',	0 },
{	u'た',	u'ち',	0,		u'だ',	0 },
{	u'ち',	u'つ',	0,		u'ぢ',	0 },
{	u'つ',	u'て',	u'っ',	u'づ',	0 },
{	u'て',	u'と',	0,		u'で',	0 },
{	u'と',	u'た',	0,		u'ど',	0 },
{	u'な',	u'に',	0,		0,		0 },
{	u'に',	u'ぬ',	0,		0,		0 },
{	u'ぬ',	u'ね',	0,		0,		0 },
{	u'ね',	u'の',	0,		0,		0 },
{	u'の',	u'な',	0,		0,		0 },
{	u'は',	u'ひ',	0,		u'ば',	u'ぱ' },
{	u'ひ',	u'ふ',	0,		u'び',	u'ぴ' },
{	u'ふ',	u'へ',	0,		u'ぶ',	u'ぷ' },
{	u'へ',	u'ほ',	0,		u'べ',	u'ぺ' },
{	u'ほ',	u'は',	0,		u'ぼ',	u'ぽ' },
{	u'ま',	u'み',	0,		0,		0 },
{	u'み',	u'む',	0,		0,		0 },
{	u'む',	u'め',	0,		0,		0 },
{	u'め',	u'も',	0,		0,		0 },
{	u'も',	u'ま',	0,		0,		0 },
{	u'や',	u'ゆ',	u'ゃ',	0,		0 },
{	u'ゆ',	u'よ',	u'ゅ',	0,		0 },
{	u'よ',	u'や',	u'ょ',	0,		0 },
{	u'ら',	u'り',	0,		0,		0 },
{	u'り',	u'る',	0,		0,		0 },
{	u'る',	u'れ',	0,		0,		0 },
{	u'れ',	u'ろ',	0,		0,		0 },
{	u'ろ',	u'ら',	0,		0,		0 },
{	u'わ',	u'を',	u'ゎ',	0,		0 },
{	u'を',	u'ん',	0,		0,		0 },
{	u'ん',	u'ー',	0,		0,		0 },
{	u'ー',	u'わ',	0,		0,		0 },
{	u'、',	u'。',	0,		0,		0 },
{	u'。',	u'?',	0,		0,		0 },
{	u'?',	u'、',	0,		0,		0 },
};
constexpr char16_t c_modifierFirstChar[] = u"、";
}

struct JaLiteralConvert :
	public std::enable_shared_from_this<JaLiteralConvert>,
	public ILiteralConvert,
	public IJaLiteralConvert,
	public IObject
{
	static bool IsVowel(char16_t ch)
	{
		return ch == L'a' || ch == L'i' || ch == L'u' || ch == L'e' || ch == L'o';
	}
	static bool IsConsonant(char16_t ch)
	{
		return (ch >= L'a' && ch <= L'z') && !IsVowel(ch);
	}

	// ILiteralConvert
	RefString ConvertText(const RefString& src) const override
	{
		if (!m_romajiBuilt) BuildRomajiTree();

		std::u16string convertedResult;
		const char16_t* srcPtr = src.u16ptr();
		char16_t stackChar = 0; // for small tsu
		while (*srcPtr != 0) {
			std::u16string converted;
			int eatenChar = ApplyRuleFromTop(srcPtr, converted);
			if (eatenChar == 0) {
				stackChar = IsConsonant(*srcPtr) ? *srcPtr : u'\0';
				convertedResult += *srcPtr;
				++srcPtr;
			}
			else {
				if (stackChar != 0) {
					if (stackChar == *srcPtr) {
						convertedResult.pop_back();
						convertedResult += WCHAR_SMALL_TSU;
					}
					stackChar = 0;
				}
				convertedResult += converted;
			}
			srcPtr += eatenChar;
		}
		return RefString(convertedResult);
	}

	void ConvertStream(ILiteralProvider* provider) const override
	{
		if (!m_romajiBuilt) BuildRomajiTree();

		struct CONVERTEDITEM {
			int romajiStart;
			int romajiLength;
			RefString romajiText;

			CONVERTEDITEM(int rs, int rl, RefString rt) :
				romajiStart(rs), romajiLength(rl), romajiText(rt) {}
		};
		std::vector<CONVERTEDITEM> convertedItem;
		RefString sourceText = provider->GetSourceBeforeCaret();
		const char16_t* srcPtr = sourceText.u16ptr();
		char16_t stackChar = 0; // for small tsu
		while (*srcPtr != 0) {
			std::u16string converted;
			int eatenChar = ApplyRuleFromTop(srcPtr, converted);
			if (eatenChar == 0) {
				stackChar = IsConsonant(*srcPtr) ? *srcPtr : 0U;
				++srcPtr;
			}
			else {
				int startIdx = (int)(srcPtr - sourceText.u16ptr());
				if (stackChar != 0) {
					if (stackChar == *srcPtr) {
						--startIdx;
						++eatenChar;
						converted.insert(converted.begin(), WCHAR_SMALL_TSU);
					}
					stackChar = 0;
				}
				convertedItem.emplace_back(CONVERTEDITEM(startIdx, eatenChar, RefString(converted)));
			}
			srcPtr += eatenChar;
		}

		for (size_t idxp1 = convertedItem.size(); idxp1 > 0; --idxp1) {
			size_t idx = idxp1 - 1;
			provider->Apply(convertedItem[idx].romajiStart, convertedItem[idx].romajiLength, convertedItem[idx].romajiText);
		}
		provider->Finalize();
	}

	std::shared_ptr<IJaLiteralConvert> GetJaLiteralConvert() override
	{
		return shared_from_this();
	}


	mutable bool m_romajiBuilt = false;

	struct RomajiNode
	{
		std::map<char16_t, RomajiNode> children;
		RefString converted;
	};
	mutable RomajiNode m_root;

	void BuildRomajiTree() const
	{
		if (m_romajiBuilt) {
			return;
		}
		m_romajiBuilt = true;

		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		std::string romajiLine = setting->GetString("RomajiTable", "Romaji");
		if (romajiLine.length() == 0) {
			return;
		}
		std::unique_ptr<char []> splitBuf(new char [romajiLine.length() + 1]);
		strcpy(splitBuf.get(), romajiLine.c_str());
		
		std::vector<char*> romajiList;
		SplitText(splitBuf.get(), ",", romajiList);

		for (auto& romajiItem : romajiList) {
			std::u16string romajiItemW = to_utf16(romajiItem);
			if (romajiItemW[0] == WCHAR_FULL_COMMA && romajiItemW[1] == u'=') {
				romajiItemW[0] = u',';
			}
			size_t equalPos = romajiItemW.find_first_of(u'=');
			if (equalPos == romajiItemW.npos) {
				continue;
			}

			RomajiNode* targetNode = &m_root;
			for (size_t charIndex = 0; charIndex < equalPos; ++charIndex) {
				auto insertRes = targetNode->children.insert(std::make_pair(romajiItemW[charIndex], RomajiNode()));
				targetNode = &(insertRes.first->second);
			}
			targetNode->converted = RefString(romajiItemW.c_str() + equalPos + 1);
		}
	}

	static void SplitText(char* src, const char* separator, std::vector<char*>& result)
	{
		result.clear();
		char* token = src;

		for (token = strtok(token, separator);
			token != nullptr;
			token = strtok(nullptr, separator))
		{
			result.push_back(token);
		}
	}

	int ApplyRuleFromTop(const char16_t* text, std::u16string& converted) const
	{
		RomajiNode *node = &m_root;

		int i = 0;
		for (; text[i] != 0; ++i) {
			auto findRes = node->children.find(text[i]);
			if (findRes == node->children.end()) {
				findRes = node->children.find(L'\'');
				if (findRes != node->children.end()) {
					converted += findRes->second.converted.u16ptr();
					return i;
				}
				break;
			}
			node = &findRes->second;
		}

		if (node->converted.length() > 0) {
			converted += node->converted.u16ptr();
			return i;
		}
		return 0;
	}

	// IJaLiteralConvert
	mutable bool m_buildMultiTapTable = false;
	std::vector<std::pair<char16_t, char16_t>> m_forwardTable;
	std::vector<std::pair<char16_t, char16_t>> m_modifierTable;
	std::vector<std::pair<char16_t, char16_t>> m_backToggleTable;

	void BuildMultiTapTable()
	{
		if (m_buildMultiTapTable) {
			return;
		}
		for (const auto& kanaData : s_kanaData)
		{
			m_forwardTable.push_back(std::make_pair(kanaData.base, kanaData.next));
			m_backToggleTable.push_back(std::make_pair(kanaData.next, kanaData.base));

			char16_t modifierList[4] = { kanaData.base, 0, 0, 0 };
			int count = 1;
			if (kanaData.small != 0) {
				modifierList[count++] = kanaData.small;
			}
			if (kanaData.dull != 0) {
				modifierList[count++] = kanaData.dull;
			}
			if (kanaData.halfdull != 0) {
				modifierList[count++] = kanaData.halfdull;
			}
			if (count > 1) {
				for (int i = 0; i < count; ++i) {
					m_modifierTable.push_back(std::make_pair(modifierList[i], modifierList[(i + 1) % count]));
				}
			}
		}
		std::sort(m_forwardTable.begin(), m_forwardTable.end(),
			[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
				return lhs.first < rhs.first;
			});
		std::sort(m_modifierTable.begin(), m_modifierTable.end(),
			[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
			return lhs.first < rhs.first;
		});
		std::sort(m_backToggleTable.begin(), m_backToggleTable.end(),
			[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
			return lhs.first < rhs.first;
		});
		m_buildMultiTapTable = true;
	}

	void Jpn12keyKanaInput(IEditLine* editLine, const std::shared_ptr<KeyEvent>& keyEvent) override
	{
		BuildMultiTapTable();
		THROW_IF_FALSE(editLine != nullptr && keyEvent->sipKeyCode == SIPKEY_JPN12_KANA);
		bool isUpdated = false;
		int caretOffset, caretLength;
		std::tie(caretOffset, caretLength) = editLine->GetCaret();
		if (keyEvent->isRepeatedKey && caretOffset > 0) {
			editLine->SetCaret(caretOffset - 1, 1);
			auto targetText = editLine->GetCaretText();
			auto targetChar = std::make_pair(targetText.u16ptr()[0], u'\0');
			auto it = std::lower_bound(m_forwardTable.begin(), m_forwardTable.end(), targetChar,
					[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
					return lhs.first < rhs.first;
				});
			if (it != m_forwardTable.end() && it->first == targetChar.first) {
				editLine->DeleteText();
				RefString newText(&it->second, 1);
				editLine->UpdateText(newText, TextRangeType_RawInput);
				isUpdated = true;
			}
		}
		if (!isUpdated) {
			// insert normally
			editLine->SetCaret(caretOffset, 0);
			editLine->UpdateText(keyEvent->label, TextRangeType_RawInput);
		}
	}
	void Jpn12keyModifierInput(IEditLine* editLine, const std::shared_ptr<KeyEvent>& keyEvent) override
	{
		BuildMultiTapTable();
		THROW_IF_FALSE(editLine != nullptr && keyEvent->sipKeyCode == SIPKEY_JPN12_MOD);
		bool isUpdated = false;
		int caretOffset, caretLength;
		std::tie(caretOffset, caretLength) = editLine->GetCaret();
		if (caretOffset > 0) {
			editLine->SetCaret(caretOffset - 1, 1);
			auto targetText = editLine->GetCaretText();
			auto targetChar = std::make_pair(targetText.u16ptr()[0], u'\0');
			auto it = std::lower_bound(m_modifierTable.begin(), m_modifierTable.end(), targetChar,
				[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
				return lhs.first < rhs.first;
			});
			if (it != m_modifierTable.end() && it->first == targetChar.first) {
				editLine->DeleteText();
				RefString newText(&it->second, 1);
				editLine->UpdateText(newText, TextRangeType_RawInput);
				isUpdated = true;
			}
		}
		if (!isUpdated) {
			editLine->SetCaret(caretOffset, 0);
		}
	}
	void Jpn12keyBreakersInput(IEditLine* editLine, const std::shared_ptr<KeyEvent>& keyEvent) override
	{
		BuildMultiTapTable();
		THROW_IF_FALSE(editLine != nullptr && keyEvent->sipKeyCode == SIPKEY_JPN12_BRK);
		bool isUpdated = false;
		int caretOffset, caretLength;
		std::tie(caretOffset, caretLength) = editLine->GetCaret();
		if (keyEvent->isRepeatedKey && caretOffset > 0) {
			editLine->SetCaret(caretOffset - 1, 1);
			auto targetText = editLine->GetCaretText();
			auto targetChar = std::make_pair(targetText.u16ptr()[0], u'\0');
			auto it = std::lower_bound(m_forwardTable.begin(), m_forwardTable.end(), targetChar,
				[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
				return lhs.first < rhs.first;
			});
			if (it != m_forwardTable.end() && it->first == targetChar.first) {
				editLine->DeleteText();
				RefString newText(&it->second, 1);
				editLine->UpdateText(newText, TextRangeType_RawInput);
				isUpdated = true;
			}
		}
		if (!isUpdated) {
			// insert normally
			editLine->SetCaret(caretOffset, 0);
			editLine->UpdateText(RefString(c_modifierFirstChar), TextRangeType_RawInput);
		}
	}
	void Jpn12keyBackToggleInput(IEditLine* editLine, const std::shared_ptr<KeyEvent>& keyEvent) override
	{
		BuildMultiTapTable();
		THROW_IF_FALSE(editLine != nullptr && keyEvent->sipKeyCode == SIPKEY_JPN12_BCKTGL);
		bool isUpdated = false;
		int caretOffset, caretLength;
		std::tie(caretOffset, caretLength) = editLine->GetCaret();
		if (caretOffset > 0) {
			editLine->SetCaret(caretOffset - 1, 1);
			auto targetText = editLine->GetCaretText();
			auto targetChar = std::make_pair(targetText.u16ptr()[0], u'\0');
			auto it = std::lower_bound(m_backToggleTable.begin(), m_backToggleTable.end(), targetChar,
				[](const std::pair<char16_t, char16_t>& lhs, const std::pair<char16_t, char16_t>& rhs) {
				return lhs.first < rhs.first;
			});
			if (it != m_backToggleTable.end() && it->first == targetChar.first) {
				editLine->DeleteText();
				RefString newText(&it->second, 1);
				editLine->UpdateText(newText, TextRangeType_RawInput);
				isUpdated = true;
			}
		}
		if (!isUpdated) {
			editLine->SetCaret(caretOffset, 0);
		}
	}

	virtual ~JaLiteralConvert() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE2(ILiteralConvert, JaLiteralConvert);
} // Ribbon
