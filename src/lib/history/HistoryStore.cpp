#include "pch.h"
#include "HistoryStruct.h"
#include "HistoryStore.h"
#include "dictionary/DictionaryReader.h"

namespace Ribbon { namespace History {

class HistoryClassList :
	public std::enable_shared_from_this<HistoryClassList>,
	public IHistoryClassList,
	public IObject
{
	struct ComparePtrByName {
		bool operator () (const HistoryClass* lhs, const HistoryClass* rhs) {
			return lhs->className < rhs->className; } };
	struct CompareByID {
		bool operator () (const HistoryClass& lhs, const HistoryClass& rhs) {
			return lhs.classId < rhs.classId; } };

	std::vector<HistoryClass> m_classList;
	std::vector<HistoryClass*> m_fileToDict;
	HistoryClass* m_bos;
	HistoryClass* m_eos;
	HistoryClass* m_unk;
	uint32_t m_usingMark;

public:
	// Prepare for reading file
	void Initialize(Dictionary::IDictionaryReader* dictionaryReader) override
	{
		Dictionary::PosNameReader posReader = dictionaryReader->CreatePosNameReader();

		uint16_t bosID = posReader.GetBOS();
		uint16_t eosID = posReader.GetEOS();
		uint16_t unkID = posReader.GetUNK();

		m_unk = nullptr;
		RefString unkClassName;
		posReader.EnumeratePosName([&](const char16_t* className, uint16_t classId) {
			m_classList.emplace_back(HistoryClass(RefString(className), classId));
		});
		m_classList.emplace_back(HistoryClass(RefString(u"[UNK]"), unkID));

		std::sort(m_classList.begin(), m_classList.end(), CompareByID());

		for (auto& classItem : m_classList) {
			if (classItem.classId == bosID) {
				m_bos = &classItem;
			}
			else if (classItem.classId == eosID) {
				m_eos = &classItem;
			}
			else if (classItem.classId == unkID) {
				m_unk = &classItem;
			}
		}
		THROW_IF_FALSE(m_unk != nullptr && m_bos != nullptr && m_eos != nullptr);
	}
	void UpdateUsingMark(uint32_t usingMark) override
	{
		m_usingMark = usingMark;
	}
	HistoryClass* GetFromClassId(uint16_t classId) override
	{
		auto lowerBound = std::lower_bound(m_classList.begin(), m_classList.end(),
			HistoryClass(RefString(), classId), CompareByID());
		if (lowerBound != m_classList.end() && lowerBound->classId == classId) {
			return &*lowerBound;
		}
		return m_unk;
	}
	HistoryClass* GetFromFileIndex(uint16_t fileIndex) override
	{
		THROW_IF_FALSE(fileIndex < m_fileToDict.size());
		return m_fileToDict[fileIndex];
	}
	HistoryClass* GetBOS() override { return m_bos; }
	HistoryClass* GetEOS() override { return m_eos; }
	HistoryClass* GetUNK() override { return m_unk; }

	// IFlexibleBinBase
	const uint8_t* Read(const uint8_t* readPtr, const uint8_t* endPtr) override
	{
		uint32_t fieldTypeId;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldTypeId);
		THROW_IF_FALSE(fieldTypeId == FLEXBIN_FIELDID_CLASSLIST);
		uint32_t countOfRecord;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &countOfRecord);

		std::vector<HistoryClass*> nameOrderList;
		nameOrderList.reserve((m_classList.size()));
		for (HistoryClass& histClass : m_classList) {
			nameOrderList.emplace_back(&histClass);
		}
		std::sort(nameOrderList.begin(), nameOrderList.end(), ComparePtrByName());
		m_fileToDict.clear();
		m_fileToDict.reserve(countOfRecord);

		for (uint32_t i = 0; i < countOfRecord; ++i) {
			RefString className;
			readPtr = FlexBinUtil::RefStringFromBlob(readPtr, endPtr, className);

			HistoryClass searchHistClass(className, 0);
			auto lowerBound = std::lower_bound(nameOrderList.begin(), nameOrderList.end(),
				&searchHistClass, ComparePtrByName());

			if (lowerBound != nameOrderList.end() && (*lowerBound)->className == className) {
				m_fileToDict.emplace_back(*lowerBound);
			}
			else {
				// TODO: more better matching logic
				m_fileToDict.emplace_back(m_unk);
			}
		}
		return readPtr;
	}
	uint32_t Write(IFlexibleBinStream* stream) override
	{
		// count write target
		uint32_t writeCount = 0;
		for (const auto& histItem : m_classList) {
			if (histItem.usingMark == m_usingMark) {
				++writeCount;
			}
		}

		uint16_t fileIndex = 0;
		uint32_t writeSize = 0;
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_CLASSLIST, stream);
		writeSize += FlexBinUtil::WriteUint32(writeCount, stream);
		for (auto& histItem : m_classList) {
			if (histItem.usingMark == m_usingMark) {
				writeSize += FlexBinUtil::RefStringToBlob(FLEXBIN_TYPE_BLOB, histItem.className, stream);
				histItem.fileIndex = fileIndex;
				++fileIndex;
			}
		}
		return writeSize;
	}

	virtual ~HistoryClassList() {}
	IOBJECT_COMMON_METHODS
};

struct HistoryPrimList :
	public std::enable_shared_from_this<HistoryPrimList>,
	public IHistoryPrimList,
	public IObject
{
	const char16_t* const BOS_TEXT = u"\u000E";
	const char16_t* const EOS_TEXT = u"\u000F";

	std::deque<HistoryPrimitive> m_primList;
	std::unique_ptr<TrieNode> m_rootTrie;
	std::weak_ptr<IHistoryClassList> m_histClassList;
	HistoryPrimitive* m_bos = nullptr;
	HistoryPrimitive* m_eos = nullptr;

	HistoryPrimList()
	{
	}
	void Initialize(IHistoryClassList* histClassList) override
	{
		m_rootTrie = std::make_unique<TrieNode>(u'\0', 0U);
		m_bos = AddOrFindWord(RefString(BOS_TEXT), RefString(BOS_TEXT), histClassList->GetBOS(), 0, true);
		m_eos = AddOrFindWord(RefString(EOS_TEXT), RefString(EOS_TEXT), histClassList->GetEOS(), 0, true);
	}

	void PrepareBeforeLoading(IHistoryClassList* histClassList) override
	{
		m_histClassList = GetSharedPtr<IHistoryClassList>(histClassList);
	}

	void PrepareBeforeSaving(uint32_t usingMark) override
	{
		// update using mark to class before saving
		for (auto& histPrim : m_primList) {
			if (!histPrim.IsDeleted()) {
				histPrim.classPtr->usingMark = usingMark;
			}
		}
	}

	HistoryPrimitive* GetFromFileIndex(uint32_t fileIndex) override
	{
		THROW_IF_FALSE(fileIndex < m_primList.size());
		return &m_primList[fileIndex];
	}

	std::vector<const HistoryPrimitive*> AllPrimitives() override
	{
		std::vector<const HistoryPrimitive*> retVal;
		retVal.reserve(m_primList.size());
		for (const auto& prim : m_primList) {
			retVal.emplace_back(&prim);
		}
		return retVal;
	}

	TrieNode* GetTrieNode() override
	{
		return m_rootTrie.get();
	}

	// IFlexibleBinBase
	const uint8_t* Read(const uint8_t* readPtr, const uint8_t* endPtr) override
	{
		m_primList.clear();
		m_rootTrie = std::make_unique<TrieNode>(u'\0', 0U);
		std::shared_ptr<IHistoryClassList> histClassList = m_histClassList.lock();

		uint32_t fieldTypeId;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldTypeId);
		THROW_IF_FALSE(fieldTypeId == FLEXBIN_FIELDID_PRIMLIST);
		uint32_t countOfRecord;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &countOfRecord);

		for (uint32_t i = 0; i < countOfRecord; ++i) {
			readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldTypeId);
			THROW_IF_FALSE(fieldTypeId == FLEXBIN_FIELDID_PRIMPROPS);
			uint32_t countOfProps;
			readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &countOfProps);

			RefString displayText, readingText;
			HistoryClass* classPtr = nullptr;
			uint32_t primBitFlags = 0;
			uint32_t primUsedTime = 0;
			for (uint32_t prop = 0; prop < countOfProps; ++prop)
			{
				const uint8_t* nextPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldTypeId);
				switch (fieldTypeId)
				{
				case FLEXBIN_FIELDID_PRIMDISPLAY:
					readPtr = FlexBinUtil::RefStringFromBlob(readPtr, endPtr, displayText);
					break;
				case FLEXBIN_FIELDID_PRIMREADING:
					readPtr = FlexBinUtil::RefStringFromBlob(readPtr, endPtr, readingText);
					break;
				case FLEXBIN_FIELDID_PRIMBITFLAGS: {
					readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &primBitFlags);
					break;
				}
				case FLEXBIN_FIELDID_PRIMCLASS: {
					uint32_t tmp;
					readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &tmp);
					classPtr = histClassList->GetFromFileIndex(static_cast<uint16_t>(tmp));
					break;
				}
				case FLEXBIN_FIELDID_PRIMTIME:
					readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &primUsedTime);
					break;
				default: {
					FLEXBIN_ELEMENT elem;
					readPtr = elem.ReadData(readPtr, endPtr); // read and release
				}
				}
			}
			THROW_IF_FALSE(classPtr != nullptr);
			THROW_IF_FALSE(primUsedTime != 0);
			THROW_IF_FALSE(displayText.length() > 0);
			THROW_IF_FALSE(readingText.length() > 0);

			HistoryPrimitive* histPrim = AddOrFindWord(displayText, readingText, classPtr, primBitFlags, true);
			histPrim->usedTime = primUsedTime;
			histPrim->bitFlags = primBitFlags;
		}
		m_bos = AddOrFindWord(RefString(BOS_TEXT), RefString(BOS_TEXT), histClassList->GetBOS(), 0, true);
		m_eos = AddOrFindWord(RefString(EOS_TEXT), RefString(EOS_TEXT), histClassList->GetEOS(), 0, true);
		return readPtr;
	}

	uint32_t Write(IFlexibleBinStream* stream) override
	{
		// count active workds
		uint32_t activeCount = 0;
		for (auto& prim : m_primList) {
			if (!prim.IsDeleted()) {
				activeCount++;
			}
		}

		uint32_t writeSize = 0;
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_PRIMLIST, stream);
		writeSize += FlexBinUtil::WriteUint32(activeCount, stream);

		uint32_t writePrimCount = 0;
		for (auto& prim : m_primList) {
			if (prim.IsDeleted()) {
				continue;
			}
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_PRIMPROPS, stream);
			writeSize += FlexBinUtil::WriteUint32(5, stream); // count of fields
			writeSize += FlexBinUtil::RefStringToBlob(FLEXBIN_FIELDID_PRIMDISPLAY, prim.display->text, stream);
			writeSize += FlexBinUtil::RefStringToBlob(FLEXBIN_FIELDID_PRIMREADING, prim.reading->text, stream);
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_PRIMBITFLAGS, stream);
			writeSize += FlexBinUtil::WriteUint32(prim.bitFlags, stream);
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_PRIMCLASS, stream);
			writeSize += FlexBinUtil::WriteUint32(prim.classPtr->fileIndex, stream);
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_PRIMTIME, stream);
			writeSize += FlexBinUtil::WriteUint32(prim.usedTime, stream);

			prim.fileIndex = writePrimCount;
			++writePrimCount;
		}
		THROW_IF_FALSE(writePrimCount == activeCount);
		return writeSize;
	}

	HistoryPrimitive* AddOrFindWord(const RefString& displayText, const RefString& readingText, HistoryClass* classPtr, uint32_t bitFlags, bool addIfNotFound) override
	{
		if (displayText == readingText) {
			std::shared_ptr<HistoryText> historyTextDisplay = m_rootTrie->Insert(displayText);
			for (const auto& existingPrim : historyTextDisplay->primitives) {
				if (existingPrim->reading == existingPrim->display && existingPrim->classPtr == classPtr) {
					existingPrim->UpdateUsedTime();
					return existingPrim;
				}
			}
			if (!addIfNotFound) {
				return nullptr;
			}
			m_primList.emplace_back();
			HistoryPrimitive& histPrim(m_primList.back());
			historyTextDisplay->primitives.emplace_back(&histPrim);
			histPrim.display = histPrim.reading = historyTextDisplay.get();
			histPrim.classPtr = classPtr;
			histPrim.bitFlags = bitFlags;
			histPrim.UpdateUsedTime();
			return &histPrim;
		}

		std::shared_ptr<HistoryText> historyTextDisplay = m_rootTrie->Insert(displayText);
		std::shared_ptr<HistoryText> historyTextReading = m_rootTrie->Insert(readingText);

		for (const auto& existingPrim : historyTextDisplay->primitives) {
			if (existingPrim->reading == historyTextReading.get() && existingPrim->classPtr == classPtr) {
				existingPrim->UpdateUsedTime();
				return existingPrim;
			}
		}
		if (!addIfNotFound) {
			return nullptr;
		}

		m_primList.emplace_back();
		HistoryPrimitive& histPrim(m_primList.back());
		historyTextDisplay->primitives.emplace_back(&histPrim);
		historyTextReading->primitives.emplace_back(&histPrim);
		histPrim.classPtr = classPtr;
		histPrim.display = historyTextDisplay.get();
		histPrim.reading = historyTextReading.get();
		histPrim.bitFlags = bitFlags;
		histPrim.UpdateUsedTime();
		return &histPrim;
	}

	HistoryPrimitive* GetBOS() override { return m_bos; }
	HistoryPrimitive* GetEOS() override { return m_eos; }

	HistoryPrimList(const HistoryPrimList&) = delete;
	HistoryPrimList& operator = (const HistoryPrimList&) = delete;
	virtual ~HistoryPrimList() {}
	IOBJECT_COMMON_METHODS
};

struct HistoryNGramTree :
	public std::enable_shared_from_this<HistoryNGramTree>,
	public IHistoryNGramTree,
	public IObject
{
	const uint32_t maxStrong = 4;

	std::unique_ptr<NGramNode> m_rootNode;
	std::vector<HistoryPrimitive*> m_previousPrims;
	std::weak_ptr<IHistoryPrimList> m_primList;
	IHistoryPrimList* m_primListLocked = nullptr;

	HistoryNGramTree()
	{
		m_rootNode = std::make_unique<NGramNode>();
	}
	void PrepareBeforeLoading(IHistoryPrimList* primList) override
	{
		m_primList = GetSharedPtr<IHistoryPrimList>(primList);
	}
	void InsertSequence(HistoryPrimitive** primList, uint32_t primCount, bool isStrong) override
	{
		NGramNode* leafNode = m_rootNode->Insert(primList, primCount);
		leafNode->usedCount++;
		leafNode->strongCount = std::min(leafNode->strongCount + (isStrong ? 1 : 0), maxStrong);
		leafNode->UpdateUsedTime();
	}
	void InsertReverseSequence(HistoryPrimitive** primList, uint32_t primCount, bool isStrong) override
	{
		NGramNode* leafNode = m_rootNode->InsertReverse(primList, primCount);
		leafNode->usedCount++;
		leafNode->strongCount = std::min(leafNode->strongCount + (isStrong ? 1 : 0), maxStrong);
		leafNode->UpdateUsedTime();
	}

	NGramNode* GetNGramNode() override { return m_rootNode.get(); }

	// IFlexibleBinBase
	const uint8_t* Read(const uint8_t* readPtr, const uint8_t* endPtr) override
	{
		std::shared_ptr<IHistoryPrimList> primListLocked = m_primList.lock();
		m_primListLocked = primListLocked.get();
		auto scopeExit = ScopeExit([&]() { m_primListLocked = nullptr; });

		m_rootNode = std::make_unique<NGramNode>();

		return ReadRecursive(readPtr, endPtr, *m_rootNode, 0);
	}
	const uint8_t* ReadRecursive(const uint8_t* readPtr, const uint8_t* endPtr, NGramNode& node, int level)
	{
		uint32_t rootType;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &rootType);
		THROW_IF_FALSE(rootType == FLEXBIN_FIELDID_NGRAMNODE);
		uint32_t fieldCount;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldCount);

		for (uint32_t i = 0; i < fieldCount; ++i)
		{
			uint32_t fieldType;
            uint32_t valTmp;
			const uint8_t* nextPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldType);
			switch (fieldType)
			{
			case FLEXBIN_FIELDID_NGRAMPRIM:
				readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &valTmp);
				node.primitive = (level == 0) ? nullptr : m_primListLocked->GetFromFileIndex(valTmp);
				break;
			case FLEXBIN_FIELDID_NGRAMCOUNT:
				readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &valTmp);
				node.usedCount = valTmp;
				break;
			case FLEXBIN_FIELDID_NGRAMSTRONG:
				readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &valTmp);
				node.strongCount = valTmp;
				break;
			case FLEXBIN_FIELDID_NGRAMTIME:
				readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &valTmp);
				node.usedTime = valTmp;
				break;
			case FLEXBIN_FIELDID_NGRAMCHILDREN:
				readPtr = FlexBinUtil::ReadUint32(nextPtr, endPtr, &valTmp);
				node.children.clear();
				node.children.resize(valTmp);
				for (uint32_t iChild = 0; iChild < valTmp; ++iChild) {
					readPtr = ReadRecursive(readPtr, endPtr, node.children[iChild], level + 1);
				}
				break;
			default: {
					FLEXBIN_ELEMENT elem;
					readPtr = elem.ReadData(readPtr, endPtr); // read and release
				}
				break;
			}
		}
		std::sort(node.children.begin(), node.children.end(), NGramNode::ComparePrimitivePtr());
		return readPtr;
	}

	uint32_t Write(IFlexibleBinStream* stream) override
	{
		return WriteRecursive(m_rootNode.get(), stream, 0);
	}
	uint32_t WriteRecursive(NGramNode* node, IFlexibleBinStream* stream, int level)
	{
		uint32_t activeChildren = 0;
		for (const auto& childNode : node->children) {
			if (!childNode.primitive->IsDeleted()) {
				++activeChildren;
			}
		}

		uint32_t writeSize = 0;
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_NGRAMNODE, stream);
		writeSize += FlexBinUtil::WriteUint32(5, stream);
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_NGRAMPRIM, stream);
		writeSize += FlexBinUtil::WriteUint32((level == 0) ? 0 : node->primitive->fileIndex, stream);
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_NGRAMCOUNT, stream);
		writeSize += FlexBinUtil::WriteUint32(node->usedCount, stream);
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_NGRAMSTRONG, stream);
		writeSize += FlexBinUtil::WriteUint32(node->strongCount, stream);
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_NGRAMTIME, stream);
		writeSize += FlexBinUtil::WriteUint32(node->usedTime, stream);
		writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_NGRAMCHILDREN, stream);
		writeSize += FlexBinUtil::WriteUint32(activeChildren, stream);

		uint32_t writtenNodes = 0;
		for (auto& childNode : node->children) {
			if (childNode.primitive->IsDeleted()) {
				continue;
			}
			writeSize += WriteRecursive(&childNode, stream, level + 1);
			++writtenNodes;
		}
		THROW_IF_FALSE(activeChildren == writtenNodes);
		return writeSize;
	}

	HistoryNGramTree(const HistoryNGramTree&) = delete;
	HistoryNGramTree& operator = (const HistoryNGramTree&) = delete;
	virtual ~HistoryNGramTree() {}
	IOBJECT_COMMON_METHODS
};


struct HistoryFile :
	public std::enable_shared_from_this<HistoryFile>,
	public IHistoryFile,
	public IObject
{
	static const uint32_t headerMark = 'R' + ('B' << 8) + ('N' << 16) + ('U' << 24);

	std::string m_filePath;
	std::string m_fileName;
	std::string m_fileNameTmp;
	std::string m_filePrefix;
	int maxWords;
	uint32_t m_usingMark;
	time_t m_savedTime;

	HistoryFile() {
		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		m_filePath = setting->GetExpandedString("UserHistory", "HistoryFilePath");
		m_fileName = setting->GetExpandedString("UserHistory", "HistoryFile");
		m_fileNameTmp = setting->GetExpandedString("UserHistory", "HistoryFileTmp");
		m_usingMark = static_cast<uint32_t>(time(nullptr));
	}

	void Initialize(const char* filePrefix) override {
		m_filePrefix = filePrefix;
	}

	void Load(IHistoryClassList* classList, IHistoryPrimList* primList, IHistoryNGramTree* nGramTree, IEnglishReinforce* engRein) override {
		if (m_filePath.length() == 0) {
			return;
		}
		
		const auto& fullFileName = _CreateFileName(false);
		std::shared_ptr<IMemoryMappedFile> mmap;
		try {
			mmap = Platform->OpenMemoryMappedFile(fullFileName.c_str());
		}
		catch (...) {
			return;
		}
		if (!mmap) {
			return;
		}
		const uint8_t* topAddr = reinterpret_cast<const uint8_t*>(mmap->GetAddress());
		const uint8_t* endAddr = topAddr + mmap->GetFileSize();

		primList->PrepareBeforeLoading(classList);
		nGramTree->PrepareBeforeLoading(primList);

		const uint8_t* readingPtr = topAddr;

		uint32_t fieldId;
		readingPtr = FlexBinUtil::ReadUint32(readingPtr, endAddr, &fieldId);
		THROW_IF_FALSE(fieldId == FLEXBIN_TYPE_ARRAY);
		uint32_t fieldCount;
		readingPtr = FlexBinUtil::ReadUint32(readingPtr, endAddr, &fieldCount);
		THROW_IF_FALSE(fieldCount == 5 || fieldCount == 6);
		readingPtr = FlexBinUtil::ReadUint32(readingPtr, endAddr, &fieldId);
		THROW_IF_FALSE(fieldId == FLEXBIN_FIELDID_HEADERMARK);
		uint32_t header;
		readingPtr = FlexBinUtil::ReadUint32(readingPtr, endAddr, &header);
		THROW_IF_FALSE(headerMark == header);

		time_t savedTime;
		readingPtr = FlexBinUtil::ReadTime(readingPtr, endAddr, &savedTime);
		THROW_IF_FALSE(savedTime <= time(nullptr));
		m_savedTime = savedTime;

		readingPtr = classList->Read(readingPtr, endAddr);
		readingPtr = primList->Read(readingPtr, endAddr);
		readingPtr = nGramTree->Read(readingPtr, endAddr);
		if (fieldCount >= 6)
		{
			readingPtr = engRein->Read(readingPtr, endAddr);
		}

		THROW_IF_FALSE(readingPtr == endAddr);
	}

	void Save(IHistoryClassList* classList, IHistoryPrimList* primList, IHistoryNGramTree* nGramTree, IEnglishReinforce* engRein) override {
		if (m_filePath.length() == 0) {
			return;
		}

		++m_usingMark;
		primList->PrepareBeforeSaving(m_usingMark);
		classList->UpdateUsingMark(m_usingMark);

		const auto& fileNameTmp = _CreateFileName(true);
		const auto& fileNameNew = _CreateFileName(false);

		remove(fileNameTmp.c_str());
		FILE* fp = fopen(fileNameTmp.c_str(), "wb");
		THROW_IF_FALSE(fp != nullptr);
		{
			struct WriteStream : public IFlexibleBinStream {
				WriteStream(FILE* fp) : m_fp(fp) {}
				virtual ~WriteStream() { fclose(m_fp); }
				uint32_t Write(const uint8_t* ptr, uint32_t size) override {
					return static_cast<uint32_t>(fwrite(ptr, 1, size, m_fp));
				}
				FILE* m_fp;
			} writeStream(fp);

			uint32_t writeSize = 0;
			constexpr uint32_t fieldCount = 6;
			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_TYPE_ARRAY, &writeStream);
			writeSize += FlexBinUtil::WriteUint32(fieldCount, &writeStream);

			writeSize += FlexBinUtil::WriteUint32(FLEXBIN_FIELDID_HEADERMARK, &writeStream);
			writeSize += FlexBinUtil::WriteUint32(headerMark, &writeStream);

			writeSize += FlexBinUtil::WriteTime(FLEXBIN_FIELDID_DATETIME, &writeStream);
			m_savedTime = time(nullptr);

			writeSize += classList->Write(&writeStream);
			writeSize += primList->Write(&writeStream);
			writeSize += nGramTree->Write(&writeStream);
			writeSize += engRein->Write(&writeStream);
		}

		remove(fileNameNew.c_str());
		rename(fileNameTmp.c_str(), fileNameNew.c_str());
	}

	std::string _CreateFileName(bool isTemp)
	{
		if (m_filePath.length() == 0) {
			return std::string();
		}
		std::string fullFileName = m_filePath;
		fullFileName += m_filePrefix;
		fullFileName += (isTemp ? m_fileNameTmp : m_fileName);
		return fullFileName;
	}

	virtual ~HistoryFile() {}
	IOBJECT_COMMON_METHODS
};


struct EnglishReinforce :
	public std::enable_shared_from_this<EnglishReinforce>,
	public IEnglishReinforce,
	public IObject
{
	static constexpr float learningRatioAlpha = 0.05f;

	float m_nextWordCharacter[26 + 10 + 1 + 1] = {};
	float m_candidateBias = 1.0f;
	float m_showBorder = 0.5f;

	static constexpr size_t readWriteBuf = 64;
	const uint8_t* Read(const uint8_t* readPtr, const uint8_t* endPtr) override {
		const uint8_t* ptr = readPtr;
		uint32_t fieldId;
		ptr = FlexBinUtil::ReadUint32(ptr, endPtr, &fieldId);
		THROW_IF_FALSE(fieldId == FLEXBIN_FIELDID_ENGPREDREINF);

		uint32_t blobSize;
		ptr = FlexBinUtil::ReadUint32(ptr, endPtr, &blobSize);
		THROW_IF_FALSE((ptr + blobSize) <= endPtr);
		THROW_IF_FALSE(blobSize == static_cast<uint32_t>((sizeof(float) * readWriteBuf)));
		const uint8_t* blobTop = ptr;

		memcpy(m_nextWordCharacter, ptr, sizeof(m_nextWordCharacter));
		ptr += sizeof(m_nextWordCharacter);

		memcpy(&m_candidateBias, ptr, sizeof(m_candidateBias));
		ptr += sizeof(m_candidateBias);

		memcpy(&m_showBorder, ptr, sizeof(m_showBorder));
		ptr += sizeof(m_showBorder);

		// overrite end ptr
		const uint8_t* nextPtr = blobTop + sizeof(float) * readWriteBuf;
		THROW_IF_FALSE(ptr <= nextPtr);

		return nextPtr;
	}
	uint32_t Write(IFlexibleBinStream* stream) override {
		uint32_t dataBuf[readWriteBuf] = {};
		memcpy(dataBuf, m_nextWordCharacter, sizeof(m_nextWordCharacter));
		memcpy(dataBuf + 38, &m_candidateBias, sizeof(m_candidateBias));
		memcpy(dataBuf + 39, &m_showBorder, sizeof(m_showBorder));

		uint32_t writeSize = 0;
		uint32_t fieldId = FLEXBIN_FIELDID_ENGPREDREINF;
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_BLOB);
		writeSize += FlexBinUtil::WriteUint32(fieldId, stream);

		uint32_t dataSize = sizeof(float) * readWriteBuf;
		writeSize += FlexBinUtil::WriteUint32(dataSize, stream);

		writeSize += stream->Write(reinterpret_cast<const uint8_t*>(dataBuf), sizeof(dataBuf));

		return writeSize;
	}

	int GetCharacterIndex(char16_t ch) {
		if (ch >= u'a' && ch <= u'z') return (ch - u'a');
		if (ch >= u'A' && ch <= u'Z') return (ch - u'A');
		if (ch >= u'0' && ch <= u'9') return (ch - u'0' + 26);
		if (ch == u' ') return 36;
		return 37;
	}
	void EnumCharacterInPrim(const std::shared_ptr<IPrimitive>& prim, int skipChar, const std::function<void(char16_t)>& fn) {
		const auto& disp = prim->Display();
		for (size_t i = 0; i < disp.length(); ++i) {
			if (i < static_cast<size_t>(skipChar)) {
				continue;
			}
			fn(disp[i]);
		}
	}
	bool IsShowCandidate(const std::shared_ptr<IPhrase> phrase, float candidateScore, int skipChar) override {
		const auto primCount = phrase->PrimitiveCount();
		float characterScore = 0.0f;
		for (int primIdx = 0; primIdx < std::min(primCount, 1); ++primIdx) {
			EnumCharacterInPrim(phrase->Primitive(primIdx), (primIdx == 0) ? skipChar : 0, [&](char16_t ch) {
				int idx = GetCharacterIndex(ch);
				characterScore += m_nextWordCharacter[idx];
			});
		}
		return (candidateScore * m_candidateBias + characterScore) >= m_showBorder;
	}
	// Monte Carlo method
	void Rewarded(const std::shared_ptr<IPhrase> phrase, int skipChar) override {
		const auto primCount = phrase->PrimitiveCount();
		for (int primIdx = 0; primIdx < std::min(primCount, 1); ++primIdx) {
			EnumCharacterInPrim(phrase->Primitive(primIdx), (primIdx == 0) ? skipChar : 0, [&](char16_t ch) {
				int idx = GetCharacterIndex(ch);
				m_nextWordCharacter[idx] += learningRatioAlpha;
			});
		}
		m_candidateBias += learningRatioAlpha;
		m_showBorder = std::max(0.1f, m_showBorder - learningRatioAlpha);
	}
	void SetPenalty(const std::shared_ptr<IPhrase> phrase, int skipChar) override {
		const auto primCount = phrase->PrimitiveCount();
		for (int primIdx = 0; primIdx < std::min(primCount, 1); ++primIdx) {
			EnumCharacterInPrim(phrase->Primitive(primIdx), (primIdx == 0) ? skipChar : 0, [&](char16_t ch) {
				int idx = GetCharacterIndex(ch);
				m_nextWordCharacter[idx] -= learningRatioAlpha;
			});
		}
		m_candidateBias = std::max(0.1f, m_candidateBias - learningRatioAlpha);
		m_showBorder += learningRatioAlpha;
	}
	virtual ~EnglishReinforce() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(HistoryClassList);
FACTORYDEFINE(HistoryPrimList);
FACTORYDEFINE(HistoryNGramTree);
FACTORYDEFINE(EnglishReinforce);
FACTORYDEFINE(HistoryFile);
} }/* namespace Ribbon::History */
