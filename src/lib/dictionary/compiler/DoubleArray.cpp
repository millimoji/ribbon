#include "pch.h"
#include "DoubleArray.h"
namespace Ribbon { namespace Dictionary {

struct WARRAYELEMENT
{
	int			base;
	uint32_t	check;

	WARRAYELEMENT() : base(0), check((uint32_t)-1) {}
};

struct FREELEMENT
{
	int			prevFree;
	int			nextFree;
	bool		isUsed;
	bool		isBase;

	FREELEMENT() : prevFree(-1), nextFree(-1), isUsed(false), isBase(false) {}
};

struct TARGETRANGE
{
	uint32_t	topRow;
	uint32_t	endRow;
	int			charIndex;
	int			parentIndex;

	TARGETRANGE(size_t top, size_t end, int chIdx) : topRow((uint32_t)top), endRow((uint32_t)end), charIndex(chIdx), parentIndex(0) {}
	TARGETRANGE(const TARGETRANGE& src) :
		topRow(src.topRow), endRow(src.endRow), charIndex(src.charIndex), parentIndex(src.parentIndex)
	{}
	TARGETRANGE& operator = (const TARGETRANGE& src) {
		topRow = src.topRow; endRow = src.endRow; charIndex = src.charIndex; parentIndex = src.parentIndex;
		return *this;
	}
	uint32_t length() const
	{
		return endRow - topRow;
	}
	bool operator < (const TARGETRANGE& rhs) const
	{
		if (length() != rhs.length())
		{
			return length() < rhs.length();
		}
		return topRow < rhs.topRow;
	}
};


struct WArrayMaker :
	public std::enable_shared_from_this<WArrayMaker>,
	public IObject,
	public IWArrayMaker
{
	WArrayMaker() {}
	virtual ~WArrayMaker() {}

	void BuildFlat(std::vector<const uint8_t*>& keys, bool isUniqueBase) override
	{
		Initialize(isUniqueBase);
		BuildWArray(keys);
	}
	void UpdateBaseValue(const std::function<int(int, int)>& callBack) override
	{
		for (size_t i = 0; i < m_wArray.size(); ++i)
		{
			int base = m_wArray[i].base;
			if (base >= 0)
			{
				continue;
			}
			base = callBack((int)i, -base - 1);
			m_wArray[i].base = -(base + 1);
		}
	}

	void GetBaseCheck3232(std::vector<BaseCheck3232>& wArray) const override
	{
		wArray.resize(m_wArray.size());
		for (size_t i = 0; i < m_wArray.size(); ++i)
		{
			wArray[i].base = (uint32_t)m_wArray[i].base;
			wArray[i].check = m_wArray[i].check;
		}
	}

private:
	std::vector<WARRAYELEMENT> m_wArray;
	std::deque<FREELEMENT> m_freeList;
	size_t		m_freeTop;
	size_t		m_freeEnd;
	size_t		m_maxBase;
	bool		m_isUniqueBase;

	void Set16(unsigned char *pb, int val)
	{
		pb[0] = (unsigned char)val;
		pb[1] = (unsigned char)(val >> 8);
	}
	int Get16(const unsigned char *pb)
	{
		return (int)(char)pb[1] * 0x100 + pb[0];
	}
	void Set24(unsigned char *pb, int val)
	{
		pb[0] = (unsigned char)val;
		pb[1] = (unsigned char)(val >> 8);
		pb[2] = (unsigned char)(val >> 16);
	}

	void Make168F(std::vector<std::array<uint8_t, 3>>& desiredWArray)
	{
		desiredWArray.resize(m_wArray.size());
		for (size_t i = 0; i < m_wArray.size(); ++i)
		{
			std::array<unsigned char, 3>& entry(desiredWArray[i]);

			Set16(entry.data(), m_wArray[i].base);
			desiredWArray[i][2] = (uint8_t)((m_wArray[i].check == (uint32_t)-1) ? 0xFF : (i - m_wArray[m_wArray[i].check].base));
		}
	}
	void Make2424B(std::vector<std::array<unsigned char, 6>>& desiredWArray)
	{
		desiredWArray.resize(m_wArray.size());
		for (size_t i = 0; i < m_wArray.size(); ++i)
		{
			std::array<unsigned char, 6>& entry(desiredWArray[i]);

			Set24(entry.data(), m_wArray[i].base);
			Set24(entry.data() + 3, (int)m_wArray[i].check);
		}
	}

	void Initialize(bool isUniqueBase)
	{
		m_wArray.resize(1);
		m_freeList.resize(1);
		m_freeList[0].prevFree = -1;
		m_freeList[0].nextFree = -1;
		m_freeTop = 0;
		m_freeEnd = 0;
		m_maxBase = 0;
		m_isUniqueBase = isUniqueBase;
	}

	void Resize(size_t newSize)
	{
		if (m_freeList.size() >= (newSize + 1))
		{
			return;
		}
		size_t oldSize = m_freeList.size();

		// increase buffer
		m_wArray.resize(newSize + 1);
		m_freeList.resize(newSize + 1);

		// write linking information
		for (size_t i = oldSize; i < (newSize + 1); ++i)
		{
			m_freeList[i].prevFree = (int)(i - 1);
			m_freeList[i].nextFree = (int)(i + 1);
		}

		m_freeList[m_freeEnd].nextFree = (int)oldSize;
		m_freeList[oldSize].prevFree = (int)m_freeEnd;
		m_freeList[newSize].nextFree = -1;
		m_freeEnd = newSize;
	}

	int GetNextFreeIndex(int index)
	{
		// Getting next free index. If argument is -1, this returns top of free entries.
		if (index < 0)
		{
			return (int)m_freeTop;
		}
		if (m_freeList[(size_t)index].nextFree < 0)
		{
			size_t iEndIndex = m_freeList.size();
			Resize(iEndIndex);
			return (int)iEndIndex;
		}

		// TODO: assert
		//assert(!freeList[index].isUsed);
		return m_freeList[(size_t)index].nextFree;
	}

	void MarkUsed(size_t index)
	{
		// mark the entry as 'used'

		// resize if index is over than current size.
		if ((index + 1) >= m_freeList.size())
		{
			Resize(index + 1);
		}

		// update linking information
		int orgPrev = m_freeList[index].prevFree;
		int orgNext = m_freeList[index].nextFree;

		if (orgPrev >= 0)
		{
			m_freeList[(size_t)orgPrev].nextFree = orgNext;
		}
		if (orgNext >= 0)
		{
			m_freeList[(size_t)orgNext].prevFree = orgPrev;
		}

		if (index == m_freeTop)
		{
			m_freeTop = (size_t)orgNext;
		}
		if (index == m_freeEnd)
		{
			m_freeEnd = (size_t)orgPrev;
		}

		// mark 'used'
		m_freeList[index].isUsed = true;
	}

	void MarkBase(size_t index)
	{
		// resize if index is over than current size.
		if (index >= m_freeList.size())
		{
			Resize(index + 1);
		}

		// mark the entry is 'used for base index'
		m_freeList[index].isBase = true;

		m_maxBase = std::max(m_maxBase, index);
	}

	bool IsUsed(size_t index)
	{
		return index < m_freeList.size() && m_freeList[index].isUsed;
	}

	bool IsBase(size_t index)
	{
		return index < m_freeList.size() && m_freeList[index].isBase;
	}

	void CalcurateFreeRatio(int &nTotal, int &nEmpty)
	{
		// getting double array status, index size and count of unused entries.
		nTotal = (int)m_freeList.size();

		nEmpty = 0;
		std::for_each(m_freeList.begin(), m_freeList.end(), [&](FREELEMENT& oFree)
		{
			if (!oFree.isUsed) ++nEmpty;
		});
	}

	void BuildWArray(std::vector<const unsigned char*>& keys)
	{
		std::set<TARGETRANGE> remainedRange;
		std::sort(keys.begin(), keys.end(),
			[](const unsigned char* lhs, const unsigned char* rhs) -> bool
		{
			return lhs != rhs && strcmp((const char*)lhs, (const char*)rhs) < 0;
		});
		remainedRange.insert(TARGETRANGE(0, keys.size(), 0));

		MarkUsed(0);
		MarkBase(0);
		m_wArray[0].check = 0;

		while (remainedRange.size() > 0)
		{
			auto itTop = remainedRange.begin();
			auto targetRange(*itTop);
			remainedRange.erase(itTop);

			// search 1st index to store this entry.
			int topFreeIndex = GetNextFreeIndex(-1);

			// slide trial base until the base >= 0
			size_t firstEntryIndex = keys[targetRange.topRow][targetRange.charIndex];
			while (topFreeIndex < (int)firstEntryIndex)
			{
				Resize(firstEntryIndex);
				topFreeIndex = GetNextFreeIndex(topFreeIndex);
			}
			size_t baseIndex = (size_t)topFreeIndex - firstEntryIndex;

			// check this base is available
			while (baseIndex < m_wArray.size())
			{
				// check unique base rule
				if (baseIndex == 0 || (m_isUniqueBase && IsBase(baseIndex)))
				{
					topFreeIndex = GetNextFreeIndex(topFreeIndex);
					baseIndex = topFreeIndex - firstEntryIndex;
					continue;
				}

				// check all entries are acceptable
				size_t entryIndex;
				for (entryIndex = targetRange.topRow; entryIndex < targetRange.endRow; ++entryIndex)
				{
					size_t targetPoint = keys[(size_t)entryIndex][targetRange.charIndex] + baseIndex;
					if (targetPoint >= m_wArray.size())
					{
						goto L_BigBreak;
					}
					if (IsUsed(targetPoint))
					{
						break;
					}
				}
				if (entryIndex < targetRange.endRow)
				{
					topFreeIndex = GetNextFreeIndex(topFreeIndex);
					baseIndex = topFreeIndex - firstEntryIndex;
					continue;
				}
				break;
			}

		L_BigBreak:
			MarkBase(baseIndex);
			Resize(baseIndex + 0xFF);

			m_wArray[(size_t)targetRange.parentIndex].base = (int)baseIndex; // update parent base

			// write data
			for (size_t entryTop = targetRange.topRow, entryEnd = 0;
				entryTop < targetRange.endRow; entryTop = entryEnd)
			{
				unsigned char targetChar = keys[entryTop][targetRange.charIndex];

				for (entryEnd = entryTop + 1; entryEnd < (uint32_t)targetRange.endRow; ++entryEnd)
				{
					if (targetChar != keys[entryEnd][targetRange.charIndex])
					{
						break;
					}
				}

				MarkUsed(baseIndex + targetChar);
				m_wArray[(size_t)(baseIndex + targetChar)].check = (uint32_t)targetRange.parentIndex;

				if ((entryTop + 1) == entryEnd &&
					(targetChar == 0 || keys[(size_t)entryTop][targetRange.charIndex + 1] == 0))
				{
					m_wArray[(size_t)(baseIndex + targetChar)].base = -((int)entryTop + 1);
					continue;
				}

				TARGETRANGE nextRange(entryTop, entryEnd, targetRange.charIndex + 1);
				nextRange.parentIndex = (int)(baseIndex + targetChar);
				remainedRange.insert(nextRange);
			}
		}
		// padding tail of array
		Resize(m_maxBase + 0xFF);

		int nTotal, nCountFree;
		CalcurateFreeRatio(nTotal, nCountFree);

		Platform->Printf("// Total: %d, Free: %d, %.02f%% waisted\n", nTotal, nCountFree, (double)nCountFree / (double)nTotal * 100.0);
	}

	WArrayMaker(const WArrayMaker&) = delete;
	WArrayMaker(WArrayMaker&&) = delete;
	WArrayMaker& operator = (const WArrayMaker&) = delete;
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(WArrayMaker);
} /* namespace Dictionary */ } /* namespace Ribbon */
