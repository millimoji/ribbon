#pragma once
#ifndef _RIBBON_UUID_H_
#define _RIBBON_UUID_H_
namespace Ribbon {
// rcl = Ribbon Core Lib

struct UUID
{
	uint32_t	_32;
	uint16_t	_16[2];
	uint8_t		_08[8];

	UUID() :
		_32(0), _16{ 0, 0 }, _08{ 0, 0, 0, 0, 0, 0, 0, 0 }
	{}

	UUID(uint32_t _32_0, uint16_t _16_0, uint16_t _16_1, uint8_t _08_0, uint8_t _08_1, uint8_t _08_2, uint8_t _08_3, uint8_t _08_4, uint8_t _08_5, uint8_t _08_6, uint8_t _08_7) :
		_32(_32_0), _16{_16_0, _16_1}, _08{_08_0, _08_1, _08_2, _08_3, _08_4, _08_5, _08_6, _08_7}
	{ }

	bool operator == (const UUID& rhs) const noexcept
	{
		return memcmp(this, &rhs, sizeof(UUID)) == 0;
	}
	bool operator != (const UUID& rhs) const noexcept
	{
		return !(*this == rhs);
	}
	bool operator < (const UUID& rhs) const noexcept
	{
		return memcmp(this, &rhs, sizeof(UUID)) < 0;
	}
};

const UUID UUID_NULL = UUID(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// string compare
struct UuidPtrComparer {
	bool operator() (const UUID*lhs, const UUID*rhs) const {
		return *lhs < *rhs;
	}
};

struct UUIDIndexer
{
	std::map<const UUID*, uint32_t, UuidPtrComparer> m_indexFromUUID;
	std::vector<UUID> m_uuidFromIndex;
	std::mutex m_lock;

	UUIDIndexer() {
		m_uuidFromIndex.emplace_back(UUID_NULL);
	}
	uint32_t GetIndexOnly(const UUID& uuid) { // Note: 0 is not found
		std::unique_lock<std::mutex> threadLocalLock(m_lock);
		auto findRes = m_indexFromUUID.find(&uuid);
		if (findRes == m_indexFromUUID.end()) return 0;
		return findRes->second;
	}
	uint32_t GetIndexOrAdd(const UUID& uuid) {
		std::unique_lock<std::mutex> threadLocalLock(m_lock);
		m_uuidFromIndex.push_back(uuid);
		auto insRes = m_indexFromUUID.insert(std::make_pair(&m_uuidFromIndex.back(), static_cast<uint32_t>(m_uuidFromIndex.size())));
		if (!insRes.second) { m_uuidFromIndex.pop_back(); }
		return insRes.first->second;
	}
	const UUID& GetUUID(uint32_t index) {
		std::unique_lock<std::mutex> threadLocalLock(m_lock);
		return m_uuidFromIndex[index];
	}

	//
	UUIDIndexer(const UUIDIndexer&) = delete;
	UUIDIndexer(UUIDIndexer&&) = delete;
	UUIDIndexer& operator = (const UUIDIndexer&) = delete;
};

} // Ribbon
#endif // _RIBBON_UUID_H_
