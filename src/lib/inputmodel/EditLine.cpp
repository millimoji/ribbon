#include "pch.h"
#include "EditLine.h"

namespace Ribbon {

struct TextRange :
	public std::enable_shared_from_this<TextRange>,
	public ITextRange
{
	std::vector<std::shared_ptr<TextRange>> m_subRange;
	RefString m_visualText;
	uint32_t m_rangeType;

	RefString VisualText() const override
	{
		return m_visualText;
	}
	int VisualLength() const override
	{
		return static_cast<int>(m_visualText.length()); // TODO: care for Surrogate pair
	}
	uint32_t RangeType() const override
	{
		return m_rangeType;
	}
	int SubRangeCount() const override
	{
		return (int)m_subRange.size();
	}
	std::shared_ptr<ITextRange> SubRange(int rangeIndex) const override
	{
		THROW_IF_FALSE((size_t)rangeIndex < m_subRange.size());
		return std::dynamic_pointer_cast<ITextRange>(m_subRange[(size_t)rangeIndex]);
	}

	virtual ~TextRange() {}
};

struct LiteralProvider :
	public std::enable_shared_from_this<LiteralProvider>,
	public ILiteralProvider,
	public IObject
{
	std::shared_ptr<IEditLine> m_editLine;
	int m_expectedCaretPos;
	int m_expectedCaretLen;

	LiteralProvider(IEditLine* editLine)
	{
		m_editLine = GetSharedPtr<IEditLine>(editLine);
		auto posAndLen = m_editLine->GetCaret();
		m_expectedCaretPos = std::get<0>(posAndLen);
		m_expectedCaretLen = std::get<1>(posAndLen);
	}

	RefString GetSourceBeforeCaret() override
	{
		std::u16string plainText;
		auto posAndLen = m_editLine->GetCaret();
		int curPos = 0;
		while (curPos < std::get<0>(posAndLen)) {
			std::shared_ptr<ITextRange> textRange = std::get<0>(m_editLine->GetRange(curPos));
			if (!textRange) {
				break;
			}
			if ((textRange->RangeType() & TextRangeType_AttributeMask) == TextRangeType_PhoneticSource) {
				plainText += textRange->VisualText().u16ptr();
			}
			else {
				plainText += std::u16string((size_t)textRange->VisualLength(), u'\uffff');
			}
			curPos = curPos + textRange->VisualLength();
		}
		return RefString(plainText);
	}
	void Apply(int fromPos, int length, RefString newText) override
	{
		m_editLine->SetCaret(fromPos, length);
		m_editLine->UpdateText(newText, TextRangeType_PhoneticText);

		int orgCaretEndPosition = m_expectedCaretPos + m_expectedCaretLen;
		if (fromPos < m_expectedCaretPos) {
			m_expectedCaretPos = std::max(0, m_expectedCaretPos - length);
			m_expectedCaretPos += (int)newText.length();
			m_expectedCaretLen = std::max(0, orgCaretEndPosition - m_expectedCaretPos);
		}
	}
	void Finalize() override
	{
		m_editLine->SetCaret(m_expectedCaretPos, 0);
	}

	virtual ~LiteralProvider() {}
	LiteralProvider() = delete;
	LiteralProvider(const LiteralProvider&) = delete;
	LiteralProvider& operator = (const LiteralProvider&) = delete;
	IOBJECT_COMMON_METHODS
};

struct INDEXANDOFFSET
{
	int rangeIndex;
	int rangeOffset;

	INDEXANDOFFSET(int index, int offset) : rangeIndex(index), rangeOffset(offset) {}
	INDEXANDOFFSET(size_t index, size_t offset) : rangeIndex((int)index), rangeOffset((int)offset) {}
};

struct EditLine :
	public std::enable_shared_from_this<EditLine>,
	public IEditLine,
	public IObject
{
	std::vector<std::shared_ptr<TextRange>> m_editLine;
	int m_caretIndex = 0;		// text range index
	int m_caretOffset = 0;		// offset from text range
	int m_caretLength = 0;
	bool m_isDirty = false;

	EditLine()
	{
	}

	// getter
	bool IsEmpty() const override
	{
		return m_editLine.empty();
	}
	RefString GetPlainText() const override
	{
		std::u16string plainText;
		for (size_t idx = 0; idx < m_editLine.size(); ++idx) {
			plainText += m_editLine[idx]->VisualText().u16ptr();
		}
		return RefString(plainText);
	}
	std::shared_ptr<IPhrase> GetSegmentedText() const override
	{
		int currentFrame = 0;
		std::shared_ptr<IPhrase> phrase = FACTORYCREATE(Phrase);
		for (size_t idx = 0; idx < m_editLine.size(); ++idx) {
			std::shared_ptr<IPrimitive> prim = FACTORYCREATE(Primitive);
			prim->Display(m_editLine[idx]->VisualText());
			prim->TopEndFrame(std::make_tuple((uint16_t)currentFrame, (uint16_t)(currentFrame + m_editLine[idx]->VisualLength())));
			phrase->Push(prim);
		}
		return phrase;
	}
	std::tuple<int, int> GetCaret() const override
	{
		int offsetFromTop = GetPositionFromIndex(INDEXANDOFFSET(m_caretIndex, m_caretOffset));
		return std::make_tuple(offsetFromTop, m_caretLength);
	}
	int GetVisualLength() const override
	{
		return GetPositionFromIndex(INDEXANDOFFSET(-1, 0));
	}
	std::tuple<std::shared_ptr<ITextRange>, int> GetRange(int pos) const override
	{
		INDEXANDOFFSET indexAndOffset = GetIndexFromPosition(pos);

		size_t textOffset = 0;
		for (size_t idx = 0; idx < (size_t)indexAndOffset.rangeIndex; ++idx) {
			textOffset += m_editLine[idx]->VisualText().length();
		}
		if ((size_t)indexAndOffset.rangeIndex < m_editLine.size()) {
			return std::make_tuple(
				m_editLine[(size_t)indexAndOffset.rangeIndex],
				(int)textOffset);
		}
		else {
			return std::make_tuple(std::shared_ptr<ITextRange>(nullptr), (int)textOffset);
		}
	}
	std::shared_ptr<ILattice> CreateInputLattice() const override
	{
		if (m_editLine.size() == 0) {
			return std::shared_ptr<ILattice>(nullptr);
		}

		std::shared_ptr<ILattice> lattice = FACTORYCREATE(Lattice);
		size_t offsetFromTop = 0;

		// TODO: partial roman
		for (size_t idx = 0; idx < m_editLine.size(); ++idx) {
			std::shared_ptr<IPrimitive> prim = FACTORYCREATE(Primitive);
			size_t nextOffsetFromTop = offsetFromTop + m_editLine[idx]->VisualText().length();

			prim->Reading(m_editLine[idx]->VisualText());
			prim->TopEndFrame(std::make_tuple((uint16_t)offsetFromTop, (uint16_t)nextOffsetFromTop));

			lattice->Add(prim);

			offsetFromTop = nextOffsetFromTop;
		}
		return lattice;
	}
	RefString GetCaretText() const override 
	{
		if (m_caretLength <= 0) {
			return RefString(); // empty
		}
		std::u16string resultText;
		int visualOffset = m_caretOffset;
		int visualLength = m_caretLength;
		for (int index = m_caretIndex; index < (int)m_editLine.size(); ++index) {
			auto text = m_editLine[(size_t)index]->VisualText();
			if ((int)text.length() < (visualOffset + visualLength)) {
				resultText += std::u16string(text.u16ptr() + visualOffset);
				visualOffset = 0;
				visualLength = visualLength - ((int)text.length() - visualOffset);
			}
			else {
				resultText += std::u16string(text.u16ptr() + visualOffset, (size_t)visualLength);
				break;
			}
		}
		return RefString(resultText);
	}

	// modifier
	void Clear() override
	{
		m_isDirty = true;
		m_editLine.clear();
		m_caretIndex = 0;
		m_caretOffset = 0;
		m_caretLength = 0;
	}
	virtual bool GetAndResetDirtyFlag() override
	{
		bool isDirty = m_isDirty;
		m_isDirty = false;
		return isDirty;
	}
	void SetCaret(int pos, int length) override
	{
		m_isDirty = true;
		if (pos < 0) {
			THROW_IF_FALSE(length == 0);
			m_caretIndex = (int)m_editLine.size();
			m_caretOffset = 0;
			m_caretLength = 0;
			return;
		}

		INDEXANDOFFSET beginIndexAndOffset = GetIndexFromPosition(pos);
		m_caretIndex = beginIndexAndOffset.rangeIndex;
		m_caretOffset = beginIndexAndOffset.rangeOffset;

		if (length == 0) {
			m_caretLength = 0;
			return;
		}

		INDEXANDOFFSET endIndexAndOffset = GetIndexFromPosition(pos + length);
		int newEndPos = GetPositionFromIndex(endIndexAndOffset);
		m_caretLength = newEndPos - pos;
	}
	void UpdateText(RefString text, uint32_t rangeType) override
	{
		m_isDirty = true;
		std::shared_ptr<TextRange> newRange = std::make_shared<TextRange>();
		newRange->m_rangeType = rangeType;
		newRange->m_visualText = text;

		if (m_caretOffset > 0) {
			BreakElement(m_caretIndex);
		}
		if (m_caretLength == 0) {
			// insert case
			m_editLine.insert(m_editLine.begin() + m_caretIndex, newRange);
			++m_caretIndex;
			return;
		}

		std::tuple<int, int> caretPosAndLen = GetCaret();
		int rangeEnd = std::get<0>(caretPosAndLen) + std::get<1>(caretPosAndLen);
		INDEXANDOFFSET indexAndOffset = GetIndexFromPosition(rangeEnd);
		if (indexAndOffset.rangeOffset != 0) {
			BreakElement(indexAndOffset.rangeIndex);
		}
		// update end offset
		indexAndOffset = GetIndexFromPosition(rangeEnd);
		THROW_IF_FALSE(indexAndOffset.rangeOffset == 0);

		for (int rangeIdx = m_caretIndex; rangeIdx < indexAndOffset.rangeIndex; ++rangeIdx) {
			newRange->m_subRange.push_back(m_editLine[(size_t)rangeIdx]);
		}
		m_editLine.erase(m_editLine.begin() + m_caretIndex, m_editLine.begin() + indexAndOffset.rangeIndex);
		m_editLine.insert(m_editLine.begin() + m_caretIndex, newRange);
		++m_caretIndex;
	}
	void DeleteText() override
	{
		m_isDirty = true;
		int length = m_caretLength;
		m_caretLength = 0;

		for (int i = 0; i < length; ++i) {
			DeleteChar();
		}
	}
	// Provide another getter
	std::shared_ptr<ILiteralProvider> GetLiteralProvider() override
	{
		std::shared_ptr<LiteralProvider> provider = std::make_shared<LiteralProvider>(this);
		return std::static_pointer_cast<ILiteralProvider>(provider);
	}

	// priavte
	void DeleteChar()
	{
		if (m_caretOffset != 0) {
			BreakElement(m_caretIndex);
		}
		if (m_editLine[(size_t)m_caretIndex]->VisualLength() > 1) {
			BreakElement(m_caretIndex);
		}

		THROW_IF_FALSE(m_caretOffset == 0 && m_editLine[(size_t)m_caretIndex]->VisualLength() == 1);

		m_editLine.erase(m_editLine.begin() + m_caretIndex);
	}
	INDEXANDOFFSET GetIndexFromPosition(int pos) const
	{
		if (pos < 0) { // bottom
			return INDEXANDOFFSET((int)m_editLine.size() + 1, 0);
		}
		if (pos == 0) {
			return INDEXANDOFFSET(0, 0);
		}
		size_t elementTop = 0;
		for (size_t idx = 0; idx < m_editLine.size(); ++idx) {
			size_t elementEnd = elementTop + m_editLine[idx]->VisualLength();
			if ((size_t)pos == elementEnd) {
				return INDEXANDOFFSET(idx + 1, (size_t)0);
			}
			if ((size_t)pos < elementEnd) {
				return INDEXANDOFFSET(idx, (size_t)pos - elementTop);
			}
			elementTop = elementEnd;
		}
		return INDEXANDOFFSET((int)m_editLine.size(), 0);
	}
	int GetPositionFromIndex(const INDEXANDOFFSET& indexAndOffset) const
	{
		int rangeIndex = indexAndOffset.rangeIndex;
		int rangeOffset = indexAndOffset.rangeOffset;
		if (rangeIndex < 0) {
			rangeIndex = (int)m_editLine.size();
			THROW_IF_FALSE(rangeOffset == 0);
		}
		int caretFromTop = 0;
		for (size_t idx = 0; idx < (size_t)rangeIndex; ++idx) {
			caretFromTop += (int)m_editLine[idx]->VisualText().length(); // TODO: Surrogate Pair
		}
		if (rangeOffset == 0) {
			return caretFromTop;
		}
		THROW_IF_FALSE((size_t)rangeOffset < m_editLine[(size_t)indexAndOffset.rangeIndex]->VisualText().length());
		return (int)caretFromTop + rangeOffset;
	}
	void BreakElement(int index)
	{
		if ((size_t)index >= m_editLine.size()) {
			return;
		}
		std::shared_ptr<TextRange> orgRange = m_editLine[(size_t)index];
		if (orgRange->VisualLength() == 1) {
			return;
		}
		m_editLine.erase(m_editLine.begin() + index);

		RefString srcText = orgRange->VisualText();
		for (int srcIndex = (int)srcText.length() - 1; srcIndex >= 0; --srcIndex) {
			std::shared_ptr<TextRange> newElement = std::make_shared<TextRange>();
			newElement->m_visualText = RefString(srcText.u16ptr() + srcIndex, 1);
			newElement->m_rangeType = (orgRange->RangeType() & TextRangeType_StateMask) | TextRangeType_Generated;
			m_editLine.insert(m_editLine.begin() + index, newElement);
		}
		if (m_caretIndex == index) {
			m_caretIndex = m_caretIndex + m_caretOffset;
			m_caretOffset = 0;
		}
		else if (m_caretIndex > index) {
			m_caretIndex = m_caretIndex + (int)srcText.length() - 1;
		}
	}
	virtual ~EditLine() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(EditLine);
} // Ribbon
