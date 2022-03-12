#include "pch.h"

namespace Ribbon {

namespace RefStringInternal {

static const size_t STRINGBLOCKSIZE = 0x4000;

struct StringBuffer
{
	uint64_t buf[STRINGBLOCKSIZE / sizeof(uint64_t)];
};

template <size_t ELEMENTSIZE>
union StringMemoryBuffer
{
	uint64_t buf[ELEMENTSIZE / sizeof(uint64_t)];
	StringMemoryBuffer* prev;

	static const int blockSize = ELEMENTSIZE;
	static const int countInBlock = static_cast<int>(STRINGBLOCKSIZE / ELEMENTSIZE);
	static const int maxLength = static_cast<int>((ELEMENTSIZE - sizeof(StringMemoryImage)) / sizeof(char16_t));
};

struct StringMemoryManager
{
	static StringMemoryManager* singleton; // Life time is manged by reference counter.

	static void Ensure()
	{
		if (!singleton) {
			singleton = new StringMemoryManager();
		}
	}
	static void Clean()
	{
		if (singleton) {
			delete singleton;
		}
		singleton = nullptr;
	}

	std::deque<std::unique_ptr<StringBuffer>> m_alloc; // just for free
	StringMemoryBuffer<16>*  m_last16ptr = nullptr;
	StringMemoryBuffer<32>*  m_last32ptr = nullptr;
	StringMemoryBuffer<64>*  m_last64ptr = nullptr;
	StringMemoryBuffer<128>* m_last128ptr = nullptr;
	StringMemoryBuffer<256>* m_last256ptr = nullptr;
	std::mutex m_lock;
	std::atomic_int m_activeInstanceCount;

	StringMemoryManager() :
		m_activeInstanceCount(0)
	{}
	~StringMemoryManager() {}

	template <class T>
	StringMemoryImage* AllocateForEachSize(T*& lastPtr)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		if (lastPtr == nullptr) {
			m_alloc.emplace_back(new StringBuffer);
			T* fillTop = reinterpret_cast<T*>(m_alloc.back()->buf);
			fillTop[0].prev = nullptr;
			for (int i = 1; i < T::countInBlock; ++i) {
				fillTop[i].prev = &fillTop[i - 1];
			}
			lastPtr = &fillTop[T::countInBlock - 1];
		}
		StringMemoryImage* ptr = reinterpret_cast<StringMemoryImage*>(lastPtr);
		lastPtr = lastPtr->prev;
		++m_activeInstanceCount;
		return ptr;
	}

	template <class T>
	void ReleaseForEachSize(StringMemoryImage* ptr, T*& lastPtr)
	{
		{
			std::lock_guard<std::mutex> lock(m_lock);
			T* rawPtr = reinterpret_cast<T*>(ptr);
			rawPtr->prev = lastPtr;
			lastPtr = rawPtr;
		}
		if (--m_activeInstanceCount == 0) {
			StringMemoryManager::Clean();
		}
	}

	StringMemoryImage* Allocate(size_t length)
	{
		StringMemoryImage* ptr;
		if (length <= StringMemoryBuffer<16>::maxLength) {
			ptr = AllocateForEachSize<StringMemoryBuffer<16>>(m_last16ptr);
		}
		else if (length <= StringMemoryBuffer<32>::maxLength) {
			ptr = AllocateForEachSize<StringMemoryBuffer<32>>(m_last32ptr);
		}
		else if (length <= StringMemoryBuffer<64>::maxLength) {
			ptr = AllocateForEachSize<StringMemoryBuffer<64>>(m_last64ptr);
		}
		else if (length <= StringMemoryBuffer<128>::maxLength) {
			ptr = AllocateForEachSize<StringMemoryBuffer<128>>(m_last128ptr);
		}
		else if (length <= StringMemoryBuffer<256>::maxLength) {
			ptr = AllocateForEachSize<StringMemoryBuffer<256>>(m_last256ptr);
		}
		else {
			size_t allocSize = sizeof(StringMemoryImage) + length * sizeof(char16_t);
			ptr = reinterpret_cast<StringMemoryImage*>(malloc(allocSize));
			++m_activeInstanceCount;
		}
		ptr->m_refCount = 1;
		ptr->m_length = static_cast<uint16_t>(length);
		ptr->m_str[0] = 0;
		return ptr;
	}

	void Release(StringMemoryImage* ptr)
	{
		if (ptr->m_length <= StringMemoryBuffer<16>::maxLength) {
			ReleaseForEachSize<StringMemoryBuffer<16>>(ptr, m_last16ptr);
		}
		else if (ptr->m_length <= StringMemoryBuffer<32>::maxLength) {
			ReleaseForEachSize<StringMemoryBuffer<32>>(ptr, m_last32ptr);
		}
		else if (ptr->m_length <= StringMemoryBuffer<64>::maxLength) {
			ReleaseForEachSize<StringMemoryBuffer<64>>(ptr, m_last64ptr);
		}
		else if (ptr->m_length <= StringMemoryBuffer<128>::maxLength) {
			ReleaseForEachSize<StringMemoryBuffer<128>>(ptr, m_last128ptr);
		}
		else if (ptr->m_length <= StringMemoryBuffer<256>::maxLength) {
			ReleaseForEachSize<StringMemoryBuffer<256>>(ptr, m_last256ptr);
		}
		else {
			free(ptr);
			if (--m_activeInstanceCount == 0) {
				StringMemoryManager::Clean();
			}
		}
	}

	StringMemoryManager(const StringMemoryManager&) = delete;
	StringMemoryManager& operator = (const StringMemoryManager&) = delete;
	StringMemoryManager(StringMemoryManager&&) = delete;
};

StringMemoryManager* StringMemoryManager::singleton;

uint32_t StringMemoryImage::DecrementReference()
{
	uint32_t refCount = --m_refCount;
	if (refCount == 0) {
		RefStringInternal::StringMemoryManager::singleton->Release(this);
	}
	return refCount;
}

} // namespace RefStringInternal


char16_t RefString::Length0Text[] = u"";

RefString::RefString(const char16_t* src, size_t length) :
	m_ptr(nullptr)
{
	length = std::min(length, static_cast<size_t>(0x7ffe));
	if (length > 0) {
		RefStringInternal::StringMemoryManager::Ensure();
		m_ptr = RefStringInternal::StringMemoryManager::singleton->Allocate(static_cast<uint16_t>(length));
		memcpy(m_ptr->m_str, src, length * sizeof(char16_t));
		m_ptr->m_str[length] = 0;
	}
}

/*static*/ uint32_t RefString::GetActiveInstanceCount()
{
	if (RefStringInternal::StringMemoryManager::singleton == nullptr) {
		return 0;
	}
	return (uint32_t)RefStringInternal::StringMemoryManager::singleton->m_activeInstanceCount;
}

} // Ribbon
