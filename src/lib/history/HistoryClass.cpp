#include "pch.h"
#include "dictionary/DictionaryReader.h"
#include "HistoryStruct.h"
#include "HistoryClass.h"

namespace Ribbon::History {

struct ClassList :
	public std::enable_shared_from_this<ClassList>,
	public IClassList,
	public IObject
{
	struct CLASSITEM {
		RefString className;
		uint16_t classID;
		uint16_t fileIndex;

		CLASSITEM() : className(), classID(0), fileIndex(0) {}
		CLASSITEM(RefString _n, uint16_t _id, uint16_t _idx) : className(_n), classID(_id), fileIndex(_idx) {}
		CLASSITEM(const CLASSITEM& src) : className(src.className), classID(src.classID), fileIndex(src.fileIndex) {}
		CLASSITEM& operator = (const CLASSITEM& src) {
			className = src.className; classID = src.classID; fileIndex = src.fileIndex;
			return *this;
		}
	};

	struct CompareByName {
		bool operator () (const CLASSITEM& lhs, const CLASSITEM& rhs) {
			return lhs.className < rhs.className;
		}
	};
	struct CompareByID {
		bool operator () (const CLASSITEM& lhs, const CLASSITEM& rhs) {
			return lhs.classID < rhs.classID;
		}
	};

	std::vector<CLASSITEM> m_classList;
	CLASSITEM m_unk;
	std::vector<uint16_t> m_fileToDict;

	ClassList() {}

	// IFlexibleBinBase
	const uint8_t* Read(const uint8_t* readPtr, const uint8_t* endPtr) override
	{
		auto nameOrderList = m_classList; // deep copy
		std::sort(nameOrderList.begin(), nameOrderList.end(), CompareByName());

		uint32_t fieldTypeId;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &fieldTypeId);
		THROW_IF_FALSE(fieldTypeId == FLEXBIN_FIELDID_CLASSLIST);
		uint32_t countOfRecord;
		readPtr = FlexBinUtil::ReadUint32(readPtr, endPtr, &countOfRecord);
		for (uint32_t i = 0; i < countOfRecord; ++i) {
			RefString className;
			readPtr = FlexBinUtil::RefStringFromBlob(readPtr, endPtr, className);
			
			auto lowerBound = std::lower_bound(nameOrderList.begin(), nameOrderList.end(),
				CLASSITEM(className, 0, 0), CompareByName());

			if (lowerBound != nameOrderList.end() && lowerBound->className == className) {
				m_fileToDict.emplace_back(lowerBound->classID);
				lowerBound->fileIndex = (uint16_t)i;
			}
			else {
				// TODO: more better matching logic
				m_fileToDict.emplace_back(m_unk.classID);
			}
		}
		return readPtr;
	}
	uint32_t Write(IFlexibleBinStream*) override
	{
		return 0;
	}

	// IClassList
	void Initialize(Ribbon::Dictionary::IDictionaryReader* dictionaryReader) override
	{
		Dictionary::PosNameReader posReader = dictionaryReader->CreatePosNameReader();
		posReader.EnumeratePosName([this](const char16_t* className, const uint16_t classId) {
			m_classList.emplace_back(CLASSITEM(RefString(className), classId, 0));
		});
		std::sort(m_classList.begin(), m_classList.end(), CompareByID());
	}
	virtual uint16_t GetDictionaryClassId(uint32_t classIdxInFile) override
	{
		return m_fileToDict[classIdxInFile - 1];
	}


	virtual ~ClassList() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(ClassList);
} /* namespace Ribbon::History */
