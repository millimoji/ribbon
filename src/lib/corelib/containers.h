#pragma once
#ifndef _RIBBON_CONTAINERS_H_
#define _RIBBON_CONTAINERS_H_
#include "refstring.h"

namespace Ribbon {

struct UUID;
struct IObject;

template <class TTOP, class TEND>
inline std::tuple<uint16_t, uint16_t> MakeTopEndFrame(TTOP top, TEND end)
{
	return std::tuple<uint16_t, uint16_t>(static_cast<uint16_t>(top), static_cast<uint16_t>(end));
}
inline int GetTopFrame(const std::tuple<uint16_t, uint16_t>& topEndFrame) { return static_cast<int>(std::get<0>(topEndFrame)); }
inline int GetEndFrame(const std::tuple<uint16_t, uint16_t>& topEndFrame) { return static_cast<int>(std::get<1>(topEndFrame)); }

struct ISerializeStream
{
	virtual void Write(const void* data, int dataSize) = 0;
	virtual void Read(void* data, int dataSize) const = 0;
};

enum class ContainerID
{
	ID_Supplement    = 1,
	ID_Primitive     = 2,
	ID_Phrase        = 3,
	ID_PhraseList    = 4,
	ID_Lattice       = 5,
};

struct ISeliarizable
{
/*
	virtual ContainerID ID() const = 0;
	virtual void Serialize(ISerializeStream* stream) const = 0;
	virtual void Deserialize(ISerializeStream* stream) = 0;
*/
};

struct IDebugLogger
{
	virtual void Dump(FILE* pf, const std::function<const char16_t*(uint16_t)>& PosToText, int headSpace = 0) const = 0;
};

// TODO: change key to string simly to fit with JSON encoder
struct ISupplement
{
	virtual int BlobSize(const UUID& uuid) const = 0;
	virtual int GetBlob(const UUID& uuid, void* buffer, int bufsiz) const = 0;
	virtual const void* GetBlobRef(const UUID& uuid, int* dataSiz) const = 0;
	virtual void SetBlob(const UUID& uuid, const void* buffer, int bufsiz) = 0;
	virtual std::shared_ptr<IObject> GetIObject(const UUID&) const = 0;
	virtual void SetIObject(const UUID&, const std::shared_ptr<IObject>&) = 0;

	template <class T>
	T GetData(const UUID& uuid)
	{
		T data;
		GetBlob(uuid, reinterpret_cast<void*>(&data), static_cast<int>(sizeof(T)));
		return data;
	}
	template <class T>
	void SetData(const UUID& uuid, const T& data)
	{
		SetBlob(uuid, reinterpret_cast<const void*>(&data), static_cast<int>(sizeof(T)));
	}
	RefString GetString(const UUID& uuid)
	{
		int bufSiz;
		const void* bufTop = GetBlobRef(uuid, &bufSiz);
		if (bufTop != nullptr) return RefString(reinterpret_cast<const char16_t*>(bufTop), bufSiz / sizeof(char16_t));
		return RefString();
	}
	void SetString(const UUID& uuid, const char16_t* text)
	{
		size_t textLen = textlen(text);
		SetBlob(uuid, reinterpret_cast<const void*>(text), static_cast<int>(textLen * sizeof(char16_t)));
	}

	template <class T>
	std::shared_ptr<T> GetObject(const UUID& uuid)
	{
		return std::dynamic_pointer_cast<T>(GetIObject(uuid));
	}
	template <class T>
	void SetObject(const UUID& uuid, const std::shared_ptr<T>& obj)
	{
		SetIObject(uuid, std::dynamic_pointer_cast<IObject>(obj));
	}

};

struct IPrimitive
{
	virtual RefString Display() const = 0;
	virtual RefString Reading() const = 0;
	virtual uint16_t Class() const = 0;
	virtual uint32_t BitFlags() const = 0;
	virtual std::shared_ptr<ISupplement> Supplement() const = 0;
	virtual std::tuple<uint16_t, uint16_t> TopEndFrame() const = 0;
	int TopFrame() { return static_cast<int>(std::get<0>(TopEndFrame())); }
	int EndFrame() { return static_cast<int>(std::get<1>(TopEndFrame())); }

	virtual RefString Display(const RefString&) = 0;
	virtual RefString Reading(const RefString&) = 0;
	virtual uint16_t Class(uint16_t) = 0;
	virtual uint32_t BitFlags(uint32_t) = 0;
	virtual std::shared_ptr<ISupplement> GetOrCreateSupplement() = 0;
	virtual std::tuple<uint16_t, uint16_t> TopEndFrame(const std::tuple<uint16_t, uint16_t>& frames) = 0;

	//
	static std::shared_ptr<IPrimitive> CreatePrimitive(const char16_t* disp, const char16_t* read, uint16_t classId, uint32_t bitFlags);
};

const uint32_t PRIMITIVE_BIT_HEADING_SPACE = 0x01;
const uint32_t PRIMITIVE_BIT_TAILING_SPACE = 0x02;

struct IPhrase : public IPrimitive
{
	virtual int PrimitiveCount() const = 0;
	virtual std::shared_ptr<IPrimitive> Primitive(int index) const = 0;
	//virtual std::shared_ptr<ISupplement> Supplement() const = 0;

	virtual void Clear() = 0;
	virtual void Push(const std::shared_ptr<IPrimitive>& primitive) = 0;
	virtual void ReverseList() = 0;
	//virtual std::shared_ptr<ISupplement> GetOrCreateSupplement() = 0;
};

const uint32_t PHRASELIST_BIT_NEXTWORD = 0x1;

struct IPhraseList
{
	virtual uint32_t BitFlags() const = 0;
	virtual int PhraseCount() const = 0;
	virtual std::shared_ptr<IPhrase> Phrase(int index) const = 0;

	virtual void Clear() = 0;
	virtual void BitFlags(uint32_t) = 0;
	virtual void Push(const std::shared_ptr<IPhrase>& phrase) = 0;
};

struct ILattice
{
	virtual int FrameCount() const = 0;
	virtual int TopLinkCount(int frameIndex) const = 0;
	virtual std::shared_ptr<IPrimitive> TopLink(int frameIndex, int linkIndex) const = 0;
	virtual int EndLinkCount(int frameIndex) const = 0;
	virtual std::shared_ptr<IPrimitive> EndLink(int frameIndex, int linkIndex) const = 0;

	virtual void Clear() = 0;
	virtual void Add(const std::shared_ptr<IPrimitive>& primitive) = 0;
};

struct SizeDataPair
{
	union {
		void* dataPtr;
		uint32_t dataBuf[3];
	} m_u;
	uint32_t m_dataSiz;

	SizeDataPair() {
		m_u.dataPtr = nullptr;
		m_dataSiz = 0;
	}
	SizeDataPair(const void* dataPtr, int dataSiz) {
		if (dataSiz > static_cast<int>(sizeof(uint32_t[3]))) {
			m_u.dataPtr = malloc(static_cast<size_t>(dataSiz)); // TODO: throw
			memcpy(m_u.dataPtr, dataPtr, static_cast<size_t>(dataSiz));
		} else {
			memcpy(m_u.dataBuf, dataPtr, static_cast<size_t>(dataSiz));
		}
		m_dataSiz = static_cast<uint32_t>(dataSiz);
	}
	~SizeDataPair() {
		if (m_dataSiz > static_cast<int>(sizeof(uint32_t[3]))) free(m_u.dataPtr);
	}
	// move context
	SizeDataPair(SizeDataPair&& src) {
		memcpy(m_u.dataBuf, src.m_u.dataBuf, sizeof(m_u.dataBuf));
		m_dataSiz = src.m_dataSiz;
		src.m_u.dataPtr = nullptr;
		src.m_dataSiz = 0;
	}
	SizeDataPair& operator = (SizeDataPair&& src) {
		if (m_dataSiz > static_cast<int>(sizeof(uint32_t[3]))) free(m_u.dataPtr);
		memcpy(m_u.dataBuf, src.m_u.dataBuf, sizeof(m_u.dataBuf));
		m_dataSiz = src.m_dataSiz;
		src.m_u.dataPtr = nullptr;
		src.m_dataSiz = 0;
		return *this;
	}
	const void* GetData() const {
		if (m_dataSiz == 0) return nullptr;
		if (m_dataSiz > static_cast<int>(sizeof(uint32_t[3]))) return m_u.dataPtr;
		return reinterpret_cast<const void*>(m_u.dataBuf);
	}
	int GetDataSize() const { return static_cast<int>(m_dataSiz); }

	// disable copy context
	SizeDataPair(const SizeDataPair&) = delete;
	SizeDataPair& operator = (const SizeDataPair&) = delete;
};

FACTORYEXTERN(Supplement);
FACTORYEXTERN(Primitive);
FACTORYEXTERN(Phrase);
FACTORYEXTERN(PhraseList);
FACTORYEXTERN(Lattice);
} // Ribbon
#endif // _RIBBON_CONTAINERS_H_
