#pragma once
#ifndef _RIBBON_FLEXBIN_H_
#define _RIBBON_FLEXBIN_H_
namespace Ribbon {

struct IFlexibleBinStream
{
	virtual uint32_t Write(const uint8_t* ptr, uint32_t size) = 0;
};
	
struct IFlexibleBinBase
{
	virtual const uint8_t* Read(const uint8_t* readPtr, const uint8_t* endPtr) = 0;
	virtual uint32_t Write(IFlexibleBinStream*) = 0;
};

const uint32_t FLEXBIN_TYPE_INT32 = 0;	// UTF8 format
const uint32_t FLEXBIN_TYPE_PAIR = 1;	// 2 item array, element of hash
const uint32_t FLEXBIN_TYPE_ARRAY = 2;	// count + item
const uint32_t FLEXBIN_TYPE_BLOB = 3;	// size + blob
const uint32_t FLEXBIN_TYPE_MASK = 3;

const uint32_t FLEXBIN_FIELD_GENERIC = 0;

struct FlexBinUtil
{
	// SHIFT macro
#define RS(v,m,l) (((uint32_t)(v) & (m))<<(l))
#define LS(v,r,m,o) (uint8_t)((((uint32_t)(v)>>(r))&(m))|(o))

	static const uint8_t* ReadUint32(const uint8_t* src, const uint8_t* endPtr, uint32_t* val) {
		if (src[0] < 0x80) { // < 0x80
			THROW_IF_FALSE((src + 1) <= endPtr);
			*val = (uint32_t)*src;
			return src + 1;
		}
		if ((src[0] & 0xC0) == 0x80) { // < 0x2000
			THROW_IF_FALSE((src + 2) <= endPtr && (src[1] & 0x80) != 0);
			*val = RS(src[0], 0x3F, 7) + RS(src[1], 0x7F, 0);
			return src + 2;
		}
		if ((src[0] & 0xE0) == 0xC0) { // < 0x8_0000
			THROW_IF_FALSE((src + 3) <= endPtr && (src[1] & 0x80) != 0 && (src[2] & 0x80) != 0);
			*val = RS(src[0], 0x1F, 14) + RS(src[1], 0x7F, 7) + RS(src[2], 0x7F, 0);
			return src + 3;
		}
		if ((src[0] & 0xF0) == 0xE0) { // < 0x200_0000
			THROW_IF_FALSE((src + 4) <= endPtr && (src[1] & 0x80) != 0 && (src[2] & 0x80) != 0 && (src[3] & 0x80) != 0);
			*val = RS(src[0], 0xF, 21) + RS(src[1], 0x7F, 14) + RS(src[2], 0x7F, 7) + RS(src[3], 0x7F, 0);
			return src + 4;
		}
		THROW_IF_FALSE((src + 5) <= endPtr && (src[1] & 0x80) != 0 && (src[2] & 0x80) != 0 && (src[3] & 0x80) != 0 && (src[4] & 0x80) != 0);
		*val = RS(src[0], 0xF, 28) + RS(src[1], 0x7F, 21) + RS(src[2], 0x7F, 14) + RS(src[3], 0x7F, 7) + RS(src[4], 0x7F, 0);
		return src + 5;
	}
	static const uint8_t* ReadInt32(const uint8_t* src, const uint8_t* endPtr, int32_t* val) {
		return ReadUint32(src, endPtr, reinterpret_cast<uint32_t*>(val));
	}
	static uint32_t WriteUint32(uint32_t val, IFlexibleBinStream* stream)
	{
		uint8_t dst[8];
		if (val < 0x80) {
			dst[0] = (uint8_t)val;
			if (stream) stream->Write(dst, 1);
			return 1;
		}
		if (val < 0x2000) {
			dst[0] = LS(val, 7, 0x3F, 0x80);
			dst[1] = LS(val, 0, 0x7F, 0x80);
			if (stream) stream->Write(dst, 2);
			return 2;
		}
		if (val < 0x80000) {
			dst[0] = LS(val, 14, 0x1F, 0xC0);
			dst[1] = LS(val, 7, 0x7F, 0x80);
			dst[2] = LS(val, 0, 0x7F, 0x80);
			if (stream) stream->Write(dst, 3);
			return 3;
		}
		if (val < 0x2000000) {
			dst[0] = LS(val, 21, 0xF, 0xE0);
			dst[1] = LS(val, 14, 0x7F, 0x80);
			dst[2] = LS(val, 7, 0x7F, 0x80);
			dst[3] = LS(val, 0, 0x7F, 0x80);
			if (stream) stream->Write(dst, 4);
			return 4;
		}
		dst[0] = LS(val, 28, 0xF, 0xF0);
		dst[1] = LS(val, 21, 0x7F, 0x80);
		dst[2] = LS(val, 14, 0x7F, 0x80);
		dst[3] = LS(val, 7, 0x7F, 0x80);
		dst[4] = LS(val, 0, 0x7F, 0x80);
		if (stream) stream->Write(dst, 5);
		return 5;
	}
	static uint32_t WriteInt32(int32_t val, IFlexibleBinStream* stream) {
		return WriteUint32(static_cast<uint32_t>(val), stream);
	}
#undef RS
#undef LS
	static uint32_t WriteTime(uint32_t fieldId, IFlexibleBinStream* stream) {
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_ARRAY);
		time_t curTimeByTime = time(nullptr);
		uint64_t curTime = static_cast<uint64_t>(curTimeByTime);
		uint32_t writeSize = 0;
		writeSize += WriteUint32(fieldId, stream);
		writeSize += WriteUint32(2, stream);
		writeSize += WriteUint32(FLEXBIN_TYPE_INT32, stream);
		writeSize += WriteUint32(static_cast<uint32_t>(curTime), stream);
		writeSize += WriteUint32(FLEXBIN_TYPE_INT32, stream);
		writeSize += WriteUint32(static_cast<uint32_t>(curTime >> 32), stream);
		return writeSize;
	}
	static const uint8_t* ReadTime(const uint8_t* ptr, const uint8_t* endPtr, time_t* fileTime) {
		uint32_t fieldId;
		ptr = ReadUint32(ptr, endPtr, &fieldId);
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_ARRAY);
		uint32_t fieldCount;
		ptr = ReadUint32(ptr, endPtr, &fieldCount);
		THROW_IF_FALSE(fieldCount == 2);
		ptr = ReadUint32(ptr, endPtr, &fieldId);

		THROW_IF_FALSE(fieldId == FLEXBIN_TYPE_INT32);
		uint32_t lowTime;
		ptr = ReadUint32(ptr, endPtr, &lowTime);

		ptr = ReadUint32(ptr, endPtr, &fieldId);
		THROW_IF_FALSE(fieldId == FLEXBIN_TYPE_INT32);
		uint32_t hiTime;
		ptr = ReadUint32(ptr, endPtr, &hiTime);

		*fileTime = static_cast<time_t>((static_cast<uint64_t>(hiTime) << 32) + lowTime);
		return ptr;
	}

	static const uint8_t* RefStringFromBlob(const uint8_t* ptr, const uint8_t* endPtr, RefString& text)
	{
		uint32_t fieldId;
		ptr = ReadUint32(ptr, endPtr, &fieldId);
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_BLOB);
		uint32_t blobSize;
		ptr = ReadUint32(ptr, endPtr, &blobSize);
		THROW_IF_FALSE((ptr + blobSize) <= endPtr);
		THROW_IF_FALSE((blobSize & 1) == 0);

		text = RefString(reinterpret_cast<const char16_t*>(ptr), blobSize >> 1);
		return ptr + blobSize;
	}
	static uint32_t RefStringToBlob(uint32_t fieldId, const RefString& text, IFlexibleBinStream* stream)
	{
		uint32_t writeSize = 0;
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_BLOB);
		writeSize += WriteUint32(fieldId, stream);

		uint32_t countOf = static_cast<uint32_t>(text.length() * 2);
		writeSize += WriteUint32(countOf, stream);

		if (stream) stream->Write(reinterpret_cast<const uint8_t*>(text.u16ptr()), countOf);
		writeSize += countOf;

		return writeSize;
	}
	static uint32_t PeekType(const uint8_t* ptr, const uint8_t* endPtr) {
		uint32_t fieldType;
		ReadUint32(ptr, endPtr, &fieldType);
		return fieldType;
	}
};

struct FLEXBIN_ELEMENT {
	uint32_t valType;
	union U {
		int32_t intVal;
		struct ARRAY {
			FLEXBIN_ELEMENT *array;
			uint32_t size;
		} arrayVal;
		struct BLOB {
			const uint8_t* blob;
			uint32_t size;
		} blobVal;
	} u;

	FLEXBIN_ELEMENT() : FLEXBIN_ELEMENT(0, 0) {}

	FLEXBIN_ELEMENT(uint32_t fieldId, int32_t intVal) {
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_INT32);
		valType = fieldId;
		u.intVal = intVal;
	}
	FLEXBIN_ELEMENT(uint32_t fieldId, FLEXBIN_ELEMENT&& first, FLEXBIN_ELEMENT&& second) {
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_PAIR);
		valType = fieldId;
		u.arrayVal.size = 2;
		u.arrayVal.array = new FLEXBIN_ELEMENT[2];
		u.arrayVal.array[0] = std::move(first);
		u.arrayVal.array[1] = std::move(second);
	}
	FLEXBIN_ELEMENT(uint32_t fieldId, uint32_t size, FLEXBIN_ELEMENT*&& dataArray) {
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_ARRAY);
		valType = fieldId;
		u.arrayVal.size = size;
		u.arrayVal.array = dataArray;
		dataArray = nullptr;
	}
	FLEXBIN_ELEMENT(uint32_t fieldId, uint32_t size, const uint8_t*&& blobPtr) {
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_BLOB);
		valType = fieldId;
		u.blobVal.size = size;
		u.blobVal.blob = blobPtr;
		blobPtr = nullptr;
	}
	FLEXBIN_ELEMENT(uint32_t fieldId, const char16_t* text) {
		THROW_IF_FALSE((fieldId & FLEXBIN_TYPE_MASK) == FLEXBIN_TYPE_BLOB);
		size_t textLen = textlen(text);
		uint8_t *copiedPtr = new uint8_t[textLen * 2];
		memcpy(copiedPtr, text, textLen * 2);
		valType = fieldId;
		u.blobVal.size = static_cast<uint32_t>(textLen) * 2;
		u.blobVal.blob = copiedPtr;
	}
	~FLEXBIN_ELEMENT() {
		Reset();
	}

	void Reset()
	{
		switch (valType & FLEXBIN_TYPE_MASK) {
		case FLEXBIN_TYPE_INT32:
			break;
		case FLEXBIN_TYPE_PAIR:
		case FLEXBIN_TYPE_ARRAY:
			delete[] u.arrayVal.array;
			break;
		case FLEXBIN_TYPE_BLOB:
			delete[] u.blobVal.blob;
			break;
		}
		valType = FLEXBIN_TYPE_INT32;
		u.intVal = 0;
	}

	// move context
	void Move(FLEXBIN_ELEMENT&& src)
	{
		Reset();
		valType = src.valType;
		switch (valType & FLEXBIN_TYPE_MASK) {
		case FLEXBIN_TYPE_INT32:
			u.intVal = src.u.intVal;
			break;
		case FLEXBIN_TYPE_PAIR:
		case FLEXBIN_TYPE_ARRAY:
			u.arrayVal.size = src.u.arrayVal.size;
			u.arrayVal.array = src.u.arrayVal.array;
			src.u.arrayVal.size = 0;
			src.u.arrayVal.array = nullptr;
			break;
		case FLEXBIN_TYPE_BLOB:
			u.arrayVal.size = src.u.arrayVal.size;
			u.arrayVal.array = src.u.arrayVal.array;
			src.u.arrayVal.size = 0;
			src.u.arrayVal.array = nullptr;
			break;
		}
		src.valType = FLEXBIN_TYPE_INT32;
		src.u.intVal = 0;
	}

	// move context
	FLEXBIN_ELEMENT(FLEXBIN_ELEMENT&& src)
	{
		Move(std::move(src));
	}
	FLEXBIN_ELEMENT& operator = (FLEXBIN_ELEMENT&& src)
	{
		Move(std::move(src));
		return *this;
	}

	// compare
	bool operator == (const FLEXBIN_ELEMENT& src) {
		if (valType != src.valType) return false;
		switch (valType & FLEXBIN_TYPE_MASK) {
		case FLEXBIN_TYPE_INT32:
			return u.intVal == src.u.intVal;
		case FLEXBIN_TYPE_PAIR:
			THROW_IF_FALSE(u.arrayVal.size == 2 && src.u.arrayVal.size == 2);
			return u.arrayVal.array[0] == src.u.arrayVal.array[0] &&
				u.arrayVal.array[1] == src.u.arrayVal.array[1];
		case FLEXBIN_TYPE_ARRAY:
			if (u.arrayVal.size != src.u.arrayVal.size) return false;
			for (uint32_t i = 0; i < u.arrayVal.size; ++i) {
				bool isEqual = (u.arrayVal.array[i] == src.u.arrayVal.array[i]);
				if (!isEqual) return false;
			}
			return true;
		case FLEXBIN_TYPE_BLOB:
			if (u.blobVal.size != src.u.blobVal.size) return false;
			if (u.blobVal.size == 0) return true;
			if (memcmp(u.blobVal.blob, src.u.blobVal.blob, u.blobVal.size) != 0) return false;
			return true;
		default:
			THROW_IF_FALSE(false);
		}
		return false;
	}
	bool operator != (const FLEXBIN_ELEMENT& src) {
		return !(*this == src);
	}

	// read write
	const uint8_t* ReadData(const uint8_t* ptr, const uint8_t* endPtr)
	{
		Reset();

		uint32_t srcValType;
		ptr = FlexBinUtil::ReadUint32(ptr, endPtr, &srcValType);
		switch (srcValType & FLEXBIN_TYPE_MASK) {
		case FLEXBIN_TYPE_INT32: {
			valType = srcValType;
			ptr = FlexBinUtil::ReadInt32(ptr, endPtr, &u.intVal);
			return ptr;
		}
		case FLEXBIN_TYPE_PAIR: {
			valType = srcValType;
			u.arrayVal.size = 2;
			u.arrayVal.array = new FLEXBIN_ELEMENT[2];
			ptr = u.arrayVal.array[0].ReadData(ptr, endPtr);
			ptr = u.arrayVal.array[1].ReadData(ptr, endPtr);
			return ptr;
		}
		case FLEXBIN_TYPE_ARRAY: {
			uint32_t countOfItem;
			ptr = FlexBinUtil::ReadUint32(ptr, endPtr, &countOfItem);
			valType = srcValType;
			u.arrayVal.size = countOfItem;
			u.arrayVal.array = new FLEXBIN_ELEMENT[countOfItem];
			for (uint32_t i = 0; i < countOfItem; ++i) {
				ptr = u.arrayVal.array[i].ReadData(ptr, endPtr);
			}
			return ptr;
		}
		case FLEXBIN_TYPE_BLOB: {
			uint32_t sizeOfBlob;
			ptr = FlexBinUtil::ReadUint32(ptr, endPtr, &sizeOfBlob);
			THROW_IF_FALSE((ptr + sizeOfBlob) <= endPtr);
			uint8_t* blobBuf = new uint8_t[sizeOfBlob];
			memcpy(blobBuf, ptr, sizeOfBlob);
			ptr += sizeOfBlob;
			valType = srcValType;
			u.blobVal.size = sizeOfBlob;
			u.blobVal.blob = blobBuf;
			return ptr;
		}
		default:
			THROW_IF_FALSE(false); // unknown type
		}
		return ptr;
	}
	uint32_t WriteData(IFlexibleBinStream* stream)
	{
		uint32_t writeSize = 0;
		writeSize += FlexBinUtil::WriteUint32(valType, stream);
		switch (valType & FLEXBIN_TYPE_MASK) {
		case FLEXBIN_TYPE_INT32: {
			writeSize += FlexBinUtil::WriteInt32(u.intVal, stream);
			return writeSize;
		}
		case FLEXBIN_TYPE_PAIR: {
			THROW_IF_FALSE(u.arrayVal.size == 2);
			writeSize += u.arrayVal.array[0].WriteData(stream);
			writeSize += u.arrayVal.array[1].WriteData(stream);
			return writeSize;
		}
		case FLEXBIN_TYPE_ARRAY: {
			writeSize += FlexBinUtil::WriteUint32(u.arrayVal.size, stream);
			for (uint32_t i = 0; i < u.arrayVal.size; ++i) {
				writeSize += u.arrayVal.array[i].WriteData(stream);
			}
			return writeSize;
		}
		case FLEXBIN_TYPE_BLOB: {
			writeSize += FlexBinUtil::WriteUint32(u.blobVal.size, stream);
			if (stream) stream->Write(u.blobVal.blob, u.blobVal.size);
			writeSize += u.blobVal.size;
			return writeSize;
		}
		default:
			THROW_IF_FALSE(false); // unknown type
		}
		return writeSize;
	}

	// deleted copy context
	FLEXBIN_ELEMENT(const FLEXBIN_ELEMENT&) = delete;
	FLEXBIN_ELEMENT& operator = (const FLEXBIN_ELEMENT&) = delete;
};

} // Ribbon
#endif //__RIBBON_FLEXBIN_H_
