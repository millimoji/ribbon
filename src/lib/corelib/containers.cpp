#include "pch.h"

namespace Ribbon {

static UUIDIndexer s_uuidIndexer;

struct Supplement :
	public std::enable_shared_from_this<Supplement>,
	public ISupplement,
	public ISeliarizable,
	public IDebugLogger,
	public IObject
{
	// ISupplement
	int BlobSize(const UUID& uuid) const override
	{
		auto shortIndex = s_uuidIndexer.GetIndexOnly(uuid);
		if (shortIndex == 0) return 0;
		auto findPodRes = m_plainData.find(shortIndex);
		if (findPodRes != m_plainData.end()) return findPodRes->second.GetDataSize();
		auto findObjRes = m_objectData.find(shortIndex);
		if (findObjRes != m_objectData.end()) return static_cast<int>(sizeof(std::shared_ptr<IObject>));
		return 0;
	}
	int GetBlob(const UUID& uuid, void* buf, int bufSiz) const override
	{
		auto shortIndex = s_uuidIndexer.GetIndexOnly(uuid);
		if (shortIndex == 0) return 0;
		auto findPodRes = m_plainData.find(shortIndex);
		if (findPodRes == m_plainData.end()) return 0;
		int dataSize = findPodRes->second.GetDataSize();
		if (bufSiz < dataSize) return -1; // not enough buffer, TODO: throw
		memcpy(buf, findPodRes->second.GetData(), static_cast<size_t>(dataSize));
		return dataSize;
	}
	const void* GetBlobRef(const UUID& uuid, int* dataSiz) const override
	{
		auto shortIndex = s_uuidIndexer.GetIndexOnly(uuid);
		if (shortIndex == 0) return nullptr;
		auto findPodRes = m_plainData.find(shortIndex);
		if (findPodRes == m_plainData.end()) return nullptr;
		*dataSiz = findPodRes->second.GetDataSize();
		return findPodRes->second.GetData();
	}
	void SetBlob(const UUID& uuid, const void* buf, int bufSiz) override
	{
		auto shortIndex = s_uuidIndexer.GetIndexOrAdd(uuid);
		auto insRes = m_plainData.insert(std::make_pair(shortIndex, SizeDataPair()));
		insRes.first->second = SizeDataPair(buf, bufSiz);
	}
	std::shared_ptr<IObject> GetIObject(const UUID& uuid) const override
	{
		auto shortIndex = s_uuidIndexer.GetIndexOnly(uuid);
		if (shortIndex == 0) return std::shared_ptr<IObject>(nullptr);
		auto findObjRes = m_objectData.find(shortIndex);
		if (findObjRes == m_objectData.end()) return std::shared_ptr<IObject>(nullptr);
		return findObjRes->second;
	}
	void SetIObject(const UUID& uuid, const std::shared_ptr<IObject>& obj) override
	{
		auto shortIndex = s_uuidIndexer.GetIndexOrAdd(uuid);
		auto insRes = m_objectData.insert(std::make_pair(shortIndex, obj));
		if (!insRes.second) insRes.first->second = obj;
	}
	// IDebugLoogger
	void Dump(FILE* , const std::function<const char16_t*(uint16_t)>& , int ) const override { }

	// internal
	std::map<uint32_t, SizeDataPair> m_plainData;
	std::map<uint32_t, std::shared_ptr<IObject>> m_objectData;

	Supplement() {}
	virtual ~Supplement() {}
	IOBJECT_COMMON_METHODS
};

struct Primitive :
	public std::enable_shared_from_this<Primitive>,
	public IPrimitive,
	public ISeliarizable,
	public IDebugLogger,
	public IObject
{
	// IPrimitive
	RefString Display() const override { return m_display; }
	RefString Reading() const override { return m_reading; }
	uint16_t Class() const override { return m_class; }
	uint32_t BitFlags() const override { return m_bitFlags; }
	std::shared_ptr<ISupplement> Supplement() const override { return std::shared_ptr<ISupplement>(nullptr); }
	std::tuple<uint16_t, uint16_t> TopEndFrame() const override { return m_topEndFrame; }

	RefString Display(const RefString& display) override { return m_display = display; }
	RefString Reading(const RefString& reading) override { return m_reading = reading; }
	uint16_t Class(uint16_t _class) override { return m_class = _class; }
	uint32_t BitFlags(uint32_t bitFlags) override { return m_bitFlags = bitFlags; }
	std::shared_ptr<ISupplement> GetOrCreateSupplement() override { return std::shared_ptr<ISupplement>(nullptr); }
	std::tuple<uint16_t, uint16_t> TopEndFrame(const std::tuple<uint16_t, uint16_t>& topEndFrame) override { return m_topEndFrame = topEndFrame; }

protected:
	std::shared_ptr<ISupplement> m_supplement;
	RefString m_display;
	RefString m_reading;
	std::tuple<uint16_t, uint16_t> m_topEndFrame;
	uint32_t m_bitFlags = 0;
	uint16_t m_class = 0;

public:
	// IDebugLoogger
	void Dump(FILE* pf, const std::function<const char16_t* (uint16_t)>& PosToText, int headSpace) const override
	{
		std::string leadingSpace;
		leadingSpace.resize((size_t)headSpace, L'\x20');

		fprintf(pf, "%s%s/%s/%s\n", leadingSpace.c_str(),
			Display().u8str().c_str(), Reading().u8str().c_str(),
			to_utf8(PosToText(Class())).c_str());
	}

	Primitive() {}
	virtual ~Primitive() {}
	IOBJECT_COMMON_METHODS
};

std::shared_ptr<IPrimitive> IPrimitive::CreatePrimitive(const char16_t* disp, const char16_t* read, uint16_t classId, uint32_t bitFlags)
{
	auto prim = FACTORYCREATE(Primitive);
	prim->Display(RefString(disp));
	prim->Reading(RefString(read));
	prim->Class(classId);
	prim->BitFlags(bitFlags);
	return prim;
}

struct Phrase :
	public std::enable_shared_from_this<Phrase>,
	public IPhrase,
	public ISeliarizable,
	public IDebugLogger,
	public IObject
{
	// IPhrase
	int PrimitiveCount() const override { return static_cast<int>(m_phrase.size()); }
	std::shared_ptr<IPrimitive> Primitive(int index) const override { return m_phrase[(size_t)index]; }
	std::shared_ptr<ISupplement> Supplement() const override { return m_supplement; }

	void Clear() override { m_phrase.clear(); m_supplement.reset(); m_isDirty = true; }
	void Push(const std::shared_ptr<IPrimitive>& primitive) override { m_phrase.push_back(primitive); m_isDirty = true; }
	void ReverseList() override { std::reverse(m_phrase.begin(), m_phrase.end()); m_isDirty = true; }
	std::shared_ptr<ISupplement> GetOrCreateSupplement() override {
		if (m_supplement) return m_supplement;
		m_supplement = FACTORYCREATE(Supplement);
		return m_supplement;
	}

	// IPrimitive
	RefString Display() const override {
		if (m_isDirty) UpdateDisplayAndReading();
		return m_cachedDisplay;
	}
	RefString Reading() const override {
		if (m_isDirty) UpdateDisplayAndReading();
		return m_cachedReading;
	}
	uint16_t Class() const override {
		if (m_phrase.size() > 0) return m_phrase.front()->Class();
		return 0;
	}
	uint32_t BitFlags() const override {
		if (m_phrase.size() == 0) return 0;
		return (m_phrase.front()->BitFlags() & PRIMITIVE_BIT_HEADING_SPACE) |
			(m_phrase.back()->BitFlags() & PRIMITIVE_BIT_TAILING_SPACE);
	}
	std::tuple<uint16_t, uint16_t> TopEndFrame() const override { return m_topEndFrame; }

	RefString Display(const RefString&) override { THROW_IF_FALSE(false); return m_cachedDisplay; }
	RefString Reading(const RefString&) override { THROW_IF_FALSE(false); return m_cachedReading; }
	uint16_t Class(uint16_t) override { THROW_IF_FALSE(false); return 0; }
	uint32_t BitFlags(uint32_t) override { THROW_IF_FALSE(false); return BitFlags(); }
	std::tuple<uint16_t, uint16_t> TopEndFrame(const std::tuple<uint16_t, uint16_t>& frames) override { return m_topEndFrame = frames; }

private:
	std::vector<std::shared_ptr<IPrimitive>> m_phrase;
	std::shared_ptr<ISupplement> m_supplement;
	mutable RefString m_cachedDisplay;
	mutable RefString m_cachedReading;
	std::tuple<uint16_t, uint16_t> m_topEndFrame;
	mutable bool m_isDirty;

	void UpdateDisplayAndReading() const {
		std::u16string concatDisplay, concatReading;
		for (size_t idx = 0; idx < m_phrase.size(); ++idx) {
			if (idx > 0 && idx < m_phrase.size()) {
				if ( /*(m_phrase[idx - 1]->BitFlags() & PRIMITIVE_BIT_TAILING_SPACE) != 0 &&*/
					(m_phrase[idx]->BitFlags() & PRIMITIVE_BIT_HEADING_SPACE) != 0) {
					concatDisplay += u' ';
					concatReading += u' ';
				}
			}
			concatDisplay += m_phrase[idx]->Display().u16ptr();
			concatReading += m_phrase[idx]->Reading().u16ptr();
		}
		m_cachedDisplay = RefString(concatDisplay);
		m_cachedReading = RefString(concatReading);
		m_isDirty = false;
	}

public:
	// IDebugLoogger
	void Dump(FILE* pf, const std::function<const char16_t*(uint16_t)>& PosToText, int headSpace) const override
	{
		std::string result;
		for (size_t idx = 0; idx < m_phrase.size(); ++idx) {
			if (idx > 0) {
				result += '/';
			}
			char buf[256 * 2];
			sprintf(buf, "%s,%s", m_phrase[idx]->Display().u8str().c_str(), to_utf8(PosToText(m_phrase[idx]->Class())).c_str());
			result += buf;
		}
		std::string leadingSpace;
		leadingSpace.resize((size_t)headSpace, '\x20');
		fprintf(pf, "%s%s\n", leadingSpace.c_str(), result.c_str());
	}

	Phrase() {}
	virtual ~Phrase() {}
	IOBJECT_COMMON_METHODS
};

struct PhraseList :
	public std::enable_shared_from_this<PhraseList>,
	public IPhraseList,
	public ISeliarizable,
	public IDebugLogger,
	public IObject
{
	uint32_t BitFlags() const override { return m_bitFlags; }
	int PhraseCount() const override { return static_cast<int>(m_list.size()); }
	std::shared_ptr<IPhrase> Phrase(int index) const override { return m_list[(size_t)index]; }

	void Clear() override { m_list.clear(); }
	void BitFlags(uint32_t bitFlags) override { m_bitFlags = bitFlags; }
	void Push(const std::shared_ptr<IPhrase>& primitive) override { m_list.push_back(primitive); }

private:
	std::vector<std::shared_ptr<IPhrase>> m_list;
	uint32_t m_bitFlags = 0;

public:
	// IDebugLoogger
	void Dump(FILE* pf, const std::function<const char16_t*(uint16_t)>& PosToText, int headSpace) const override
	{
		for (size_t idx = 0; idx < m_list.size(); ++idx) {
			auto logger = std::dynamic_pointer_cast<IDebugLogger>(m_list[idx]);
			if (logger) logger->Dump(pf, PosToText, headSpace);
		}
	}

	PhraseList() {}
	virtual ~PhraseList() {}
	IOBJECT_COMMON_METHODS
};

struct Lattice :
	public std::enable_shared_from_this<Lattice>,
	public ILattice,
	public ISeliarizable,
	public IDebugLogger,
	public IObject
{
	int FrameCount() const override
	{
		return static_cast<int>(m_topLink.size());
	}
	int TopLinkCount(int frameIndex) const override
	{
		return static_cast<int>(m_topLink[(size_t)frameIndex].size());
	}
	std::shared_ptr<IPrimitive> TopLink(int frameIndex, int linkIndex) const override
	{
		return m_topLink[(size_t)frameIndex][(size_t)linkIndex];
	}
	int EndLinkCount(int frameIndex) const override
	{
		return static_cast<int>(m_endLink[(size_t)frameIndex].size());
	}
	std::shared_ptr<IPrimitive> EndLink(int frameIndex, int linkIndex) const override
	{
		return m_endLink[(size_t)frameIndex][(size_t)linkIndex];
	}
	void Clear() override
	{
		m_topLink.clear();
		m_endLink.clear();
	}
	void Add(const std::shared_ptr<IPrimitive>& primitive) override
	{
		auto frames = primitive->TopEndFrame();
		uint16_t topFrame = std::get<0>(frames);
		uint16_t endFrame = std::get<1>(frames);

		if (m_endLink.size() < (size_t)(endFrame + 1)) {
			m_topLink.resize((size_t)(endFrame + 1));
			m_endLink.resize((size_t)(endFrame + 1));
		}

		m_topLink[topFrame].push_back(primitive);
		m_endLink[endFrame].push_back(primitive);
	}

private:
	std::vector<std::vector<std::shared_ptr<IPrimitive>>> m_topLink;
	std::vector<std::vector<std::shared_ptr<IPrimitive>>> m_endLink;

public:
	// IDebugLoogger
	void Dump(FILE* pf, const std::function<const char16_t*(uint16_t)>& PosToText, int headSpace) const override
	{
		for (size_t frameIdx = 0; frameIdx < m_topLink.size(); ++frameIdx) {
			auto& PrimitiveList(m_topLink[frameIdx]);
			for (size_t PrimitiveIdx = 0; PrimitiveIdx < PrimitiveList.size(); ++PrimitiveIdx) {
				auto logger = std::dynamic_pointer_cast<IDebugLogger>(PrimitiveList[PrimitiveIdx]);
				if (logger) logger->Dump(pf, PosToText, static_cast<int>(frameIdx) + headSpace);
			}
		}
	}

	Lattice() {}
	virtual ~Lattice() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(Supplement);
FACTORYDEFINE(Primitive);
FACTORYDEFINE(Phrase);
FACTORYDEFINE(PhraseList);
FACTORYDEFINE(Lattice);
} // Ribbon
