#pragma once
#ifndef _RIBBON_HISTORYSTRUCT_H_
#define _RIBBON_HISTORYSTRUCT_H_
namespace Ribbon { namespace History {

const uint32_t FLEXBIN_FIELDID_CLASSLIST = (0x1 << 2) + FLEXBIN_TYPE_ARRAY;
const uint32_t FLEXBIN_FIELDID_PRIMLIST = (0x2 << 2) + FLEXBIN_TYPE_ARRAY;
const uint32_t FLEXBIN_FIELDID_PRIMPROPS = (0x3 << 2) + FLEXBIN_TYPE_ARRAY;
const uint32_t FLEXBIN_FIELDID_PRIMDISPLAY = (0x4 << 2) + FLEXBIN_TYPE_BLOB;
const uint32_t FLEXBIN_FIELDID_PRIMREADING = (0x5 << 2) + FLEXBIN_TYPE_BLOB;
const uint32_t FLEXBIN_FIELDID_PRIMCLASS = (0x6 << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_PRIMTIME = (0x7 << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_NGRAMNODE = (0x8 << 2) + FLEXBIN_TYPE_ARRAY;
const uint32_t FLEXBIN_FIELDID_NGRAMPRIM = (0x9 << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_NGRAMCOUNT = (0xA << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_NGRAMSTRONG = (0xB << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_NGRAMTIME = (0xC << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_NGRAMCHILDREN = (0xD << 2) + FLEXBIN_TYPE_ARRAY;
const uint32_t FLEXBIN_FIELDID_HEADERMARK = (0xE << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_DATETIME = (0xF << 2) + FLEXBIN_TYPE_ARRAY;
const uint32_t FLEXBIN_FIELDID_PRIMBITFLAGS = (0x10 << 2) + FLEXBIN_TYPE_INT32;
const uint32_t FLEXBIN_FIELDID_ENGPREDREINF = (0x11 << 2) + FLEXBIN_TYPE_BLOB;

const uint32_t MAX_NGRAM_LENGTH = 8;

/*
	Element Management
	Insertion: add display/reading => update TrieNode => Get/Crete Element Gorup => Element
	Deletiopn: find Element => retrieve from ElementGroup => Update Trie if elementGroup is empty
	Save: Update usingMark in class, 
*/
struct HistoryPrimitive;

struct HistoryClass
{
	RefString className;
	uint32_t usingMark = 0;
	uint16_t fileIndex = 0;
	uint16_t classId = 0;

	HistoryClass() {}
	HistoryClass(const RefString& _cn, uint16_t _id) : className(_cn), classId(_id) {}
};

struct HistoryText
{
	std::vector<HistoryPrimitive*> primitives;
	RefString text;

	bool IsEmpty() {
		return primitives.size() == 0;
	}
	void Cleanup() {
		primitives.erase(std::remove_if(primitives.begin(), primitives.end(), [](const auto& prim) {
			return prim->IsDeleted();
		}), primitives.end());
	}

	HistoryText(const RefString& _text) : text(_text) {}
	HistoryText() = delete;
	HistoryText(const HistoryText&) = delete;
};

struct HistoryPrimitive
{
	HistoryText *reading = nullptr; // weak
	HistoryText *display = nullptr; // weak
	HistoryClass *classPtr = nullptr; // weak
	uint32_t bitFlags = 0;
	uint32_t usedTime = 0;
	uint32_t fileIndex = 0;;

	void MarkDeleted() { reading = display = nullptr; }
	bool IsDeleted() { return reading == nullptr; }
	void UpdateUsedTime() { usedTime = static_cast<uint32_t>(time(nullptr) >> 6); /* div by 64 */ }
	bool operator == (const HistoryPrimitive& src) const {
		return reading->text == src.reading->text && display->text == src.display->text && classPtr->className == src.classPtr->className;
	}
	bool operator != (const HistoryPrimitive& src) const {
		return !(*this == src);
	}
};

struct TrieNode
{
	char16_t nodeVal;
	uint16_t nodeLevel;
	std::vector<TrieNode> children;
	std::shared_ptr<HistoryText> historyText;

	TrieNode(char16_t val, uint32_t level) : nodeVal(val), nodeLevel(static_cast<uint16_t>(level)) {}
	TrieNode(TrieNode&& src) :
		nodeVal(src.nodeVal), nodeLevel(src.nodeLevel),
		children(std::move(src.children)), historyText(std::move(src.historyText)) {}
	TrieNode& operator = (TrieNode&& src) {
		nodeVal = src.nodeVal;
		nodeLevel = src.nodeLevel;
		children = std::move(src.children);
		historyText = std::move(src.historyText);
		return *this;
	}

	struct CompareByNodeVal {
		bool operator () (const TrieNode& lhs, const TrieNode& rhs) {
			return lhs.nodeVal < rhs.nodeVal; }
	};

	std::shared_ptr<HistoryText> Insert(const RefString& text) {
		if (!historyText && children.size() == 0) {
			return historyText = std::make_shared<HistoryText>(text);
		}
		if (historyText) {
			if (historyText->text == text) {
				return historyText;
			}
			if (historyText->text.u16ptr()[nodeLevel] != 0) { // historyText is tail string node
				MoveDownTailStringNode();
			}
		}
		if (text.u16ptr()[nodeLevel] == 0) {
			THROW_IF_FALSE(!historyText);
			return historyText = std::make_shared<HistoryText>(text);
		}
		char16_t addingChar = text.u16ptr()[nodeLevel];
		TrieNode* childNode = CreateChildNode(addingChar);
		return childNode->Insert(text);
	}
	std::shared_ptr<HistoryText> Find(const RefString& text) const {
		if (historyText && historyText->text == text) {
			return historyText;
		}
		if (children.size() == 0) {
			return std::shared_ptr<HistoryText>();
		}
		char16_t searcingChar = text.u16ptr()[nodeLevel];
		auto it = std::lower_bound(children.begin(), children.end(), TrieNode(searcingChar, nodeLevel + 1U), CompareByNodeVal());
		if (it != children.end() && it->nodeVal == searcingChar) {
			return it->Find(text);
		}
		return std::shared_ptr<HistoryText>();
	}
	void Cleanup() {
		children.erase(std::remove_if(children.begin(), children.end(), [](auto& childNode) {
			childNode.Cleanup();
			return childNode.children.size() == 0 && !childNode.historyText;
		}), children.end());

		if (children.size() == 1 && !historyText) {
			historyText = children.front().historyText;
			children.clear();
		}
		if (historyText) {
			historyText->Cleanup();
			if (historyText->IsEmpty()) {
				historyText.reset();
			}
		}
	}

	void MoveDownTailStringNode() {
		// check tail string condition
		THROW_IF_FALSE(children.size() == 0);
		THROW_IF_FALSE(historyText);
		char16_t nextChar = historyText->text.u16ptr()[nodeLevel];
		THROW_IF_FALSE(nextChar != 0);

		children.emplace_back(TrieNode(nextChar, nodeLevel + 1U));
		children.back().historyText = std::move(historyText);
	}
	TrieNode* CreateChildNode(char16_t insertingChar) {
		THROW_IF_FALSE(insertingChar != 0);
		auto it = std::lower_bound(children.begin(), children.end(), TrieNode(insertingChar, nodeLevel + 1U), CompareByNodeVal());
		if (it != children.end() && it->nodeVal == insertingChar) {
			return &*it;
		}
		return &*children.insert(it, TrieNode(insertingChar, nodeLevel + 1U));
	}
	TrieNode* FindChildNode(char16_t findingChar) {
		THROW_IF_FALSE(findingChar != 0);
		auto it = std::lower_bound(children.begin(), children.end(), TrieNode(findingChar, nodeLevel + 1U), CompareByNodeVal());
		if (it != children.end() && it->nodeVal == findingChar) {
			return &*it;
		}
		return nullptr;
	}

	TrieNode() = delete;
	TrieNode(const TrieNode&) = delete;
	TrieNode& operator = (TrieNode& src) = delete;
};

class TrieTracer
{
public:
	TrieTracer(const TrieNode* node) : m_node(node), m_level(node->nodeLevel), m_inTail(false) {}
	TrieTracer(const TrieTracer& src) : m_node(src.m_node), m_level(src.m_level), m_inTail(src.m_inTail) {}
	TrieTracer& operator = (const TrieTracer& src) { m_node = src.m_node; m_level = src.m_level; m_inTail = src.m_inTail;  return *this; }

	bool Down(char16_t target)
	{
		if (m_inTail) {
			if (m_node->historyText->text.u16ptr()[m_level] == target) {
				++m_level;
				return true;
			}
			return false;
		}
		if (m_node->children.size() == 0)
		{
			if (m_node->historyText && m_node->historyText->text.u16ptr()[m_level] == target) {
				m_inTail = true;
				++m_level;
				return true;
			}
			return false;
		}
		auto it = std::lower_bound(m_node->children.begin(), m_node->children.end(), TrieNode(target, m_node->nodeLevel + 1U), TrieNode::CompareByNodeVal());
		if (it != m_node->children.end() && it->nodeVal == target) {
			m_node = &*it;
			++m_level;
			return true;
		}
		return false;
	}
	std::shared_ptr<HistoryText> GetHistoryText() const
	{
		if (!m_node->historyText) {
			return std::shared_ptr<HistoryText>();
		}
		if (m_node->historyText->text.u16ptr()[m_level] == 0) {
			return m_node->historyText;
		}
		return std::shared_ptr<HistoryText>();
	}
	void TraceChildren(const std::function<void(HistoryText*)>& callback) const {
		if (m_node->historyText) {
			callback(m_node->historyText.get());
		}
		for (const auto& child : m_node->children) {
			TrieTracer tracer(&child);
			tracer.TraceChildren(callback);
		}
	}
	int TrieLevel() const { return m_level; }
	bool isInTail() const { return m_inTail; }

private:
	const TrieNode* m_node;
	int m_level;
	bool m_inTail;

	TrieTracer() = delete;
};

struct NGramNode
{
	std::vector<NGramNode> children;
	HistoryPrimitive* primitive = nullptr;
	uint32_t usedCount = 0;
	uint32_t strongCount = 0;
	uint32_t usedTime = 0;

	struct ComparePrimitivePtr {
		bool operator () (const NGramNode& lhs, const NGramNode& rhs) {
			return lhs.primitive < rhs.primitive;
		}
	};

	NGramNode() {}
	NGramNode(NGramNode&& src) :
		children(std::move(src.children)),
		primitive(src.primitive), usedCount(src.usedCount), strongCount(src.strongCount), usedTime(src.usedTime)
	{}
	NGramNode& operator = (NGramNode&& src) {
		children = std::move(src.children); 
		primitive = src.primitive; usedCount = src.usedCount; strongCount = src.strongCount; usedTime = src.usedTime; 
		return *this;
	}
	NGramNode(const NGramNode& src) = delete;
	NGramNode& operator = (const NGramNode& src) = delete;

	bool operator == (const NGramNode& src) const { // just for unit test
		if (!(primitive == src.primitive || *primitive == *src.primitive)) {
			return false;
		}
		if (usedCount != src.usedCount || strongCount != src.strongCount || usedTime != src.usedTime) {
			return false;
		}
		for (const auto& child : children) {
			bool compareOk = false;
			for (const auto& srcChild : src.children) {
				if (*child.primitive == *srcChild.primitive) {
					if (child == srcChild) {
						compareOk = true;
						break;
					}
				}
			}
			if (!compareOk) {
				return false;
			}
		}
		return true;
	}

	void UpdateUsedTime() { usedTime = static_cast<uint32_t>(time(nullptr) >> 6); /* div by 64 */ }
	NGramNode* Insert(HistoryPrimitive** primList, size_t primCount) {
		if (primCount == 0) {
			return this;
		}
		NGramNode tmpNode;
		tmpNode.primitive = primList[0];
		auto it = std::lower_bound(children.begin(), children.end(), tmpNode, ComparePrimitivePtr());
		if (it != children.end() && it->primitive == primList[0]) {
			return it->Insert(primList + 1, primCount - 1);
		}
		it = children.insert(it, std::move(tmpNode));
		return it->Insert(primList + 1, primCount - 1);
	}
	NGramNode* InsertReverse(HistoryPrimitive** primList, size_t primCount) {
		if (primCount == 0) {
			return this;
		}
		NGramNode tmpNode;
		tmpNode.primitive = primList[primCount - 1];
		auto it = std::lower_bound(children.begin(), children.end(), tmpNode, ComparePrimitivePtr());
		if (it != children.end() && it->primitive == tmpNode.primitive) {
			return it->InsertReverse(primList, primCount - 1);
		}
		it = children.insert(it, std::move(tmpNode));
		return it->InsertReverse(primList, primCount - 1);
	}
	const NGramNode* Find(const HistoryPrimitive* histPrim) const {
		NGramNode tmpNode;
		tmpNode.primitive = const_cast<HistoryPrimitive*>(histPrim);
		auto it = std::lower_bound(children.begin(), children.end(), tmpNode, ComparePrimitivePtr());
		if (it != children.end() && it->primitive == tmpNode.primitive) {
			return &*it;
		}
		return nullptr;
	}
};

} }
#endif // _RIBBON_HISTORYSTRUCT_H_
