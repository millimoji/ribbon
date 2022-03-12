#include "pch.h"
#include "LinearKmeans.h"
namespace Ribbon { namespace Dictionary {

struct LinearKmeans :
	public std::enable_shared_from_this<LinearKmeans>,
	public ILinearKmeans,
	public IObject
{
	static const size_t countOfClusters = 255;

	LinearKmeans() :
		m_center(countOfClusters + 1),
		m_maxValue(countOfClusters + 1)
	{}
	virtual ~LinearKmeans() {}

	void AddValue(double f) override
	{
		double logged = 0.0;
		if (f > 0.0)
		{
			logged = log(f);
			logged = std::max(std::min(logged, (double)FLT_MAX), (double)-FLT_MAX);
		}
		m_src.push_back(logged);
	}

	void Calculate() override
	{
		Initialize();

		if (m_src.size() < countOfClusters)
		{
			NotEnoughData();
		}
		else
		{
			const int maxIteration = 300;
			for (int i = 0; i < maxIteration; ++i)
			{
				double chg = Iteration();
				if (chg < FLT_MIN)
				{
					break;
				}
			}
		}
		MakeSearchTable();
	}

	void GetTable(float buffer[256]) const override
	{
		memcpy(buffer, &m_center[0], sizeof(float[256]));
	}

	uint8_t GetIndex(double val) const override
	{
		double logged = 0.0;
		if (val > 0.0)
		{
			logged = log(val);
			logged = std::max(std::min(logged, (double)FLT_MAX), (double)-FLT_MAX);
		}
		return (uint8_t)(std::upper_bound(m_maxValue.begin(), m_maxValue.end(), (float)logged) - m_maxValue.begin());
	}

	void SaveSource(const char16_t* fileName) const override
	{
		FILE *pf = nullptr;
		if ((pf = fopen(to_utf8(fileName).c_str(), "wb")) != nullptr)
		{
			auto scopeExit = ScopeExit([&]() { fclose(pf); });
			int count = (int)m_src.size();
			fwrite(&count, sizeof(int), 1, pf);
			if (count > 0)
			{
				fwrite(&m_src[0], sizeof(m_src[0]), (size_t)count, pf);
			}
		}
	}

	void LoadSource(const char16_t* fileName) override
	{
		FILE *pf = nullptr;
		if ((pf = fopen(to_utf8(fileName).c_str(), "rb")) != nullptr)
		{
			auto scopeExit = ScopeExit([&]() { fclose(pf); });
			int count = 0;
			fread(&count, sizeof(int), 1, pf);
			m_src.resize((size_t)count);
			if (count > 0)
			{
				fread(&m_src[0], sizeof(m_src[0]), (size_t)count, pf);
			}
		}
	}

private:
	std::vector<double> m_src;
	std::vector<float> m_center;
	std::vector<float> m_maxValue;

	void Initialize()
	{
		std::sort(m_src.begin(), m_src.end());

		if (m_src.size() > 0)
		{
			double minVal = m_src.front();
			double maxVal = m_src.back();

			for (size_t cluster = 0; cluster < countOfClusters; ++cluster)
			{
				m_center[cluster] = (float)((maxVal - minVal) * (cluster + 1) / (countOfClusters + 1) + minVal);
			}
		}
		m_center[countOfClusters] = FLT_MAX;
	}

	void NotEnoughData()
	{
		if (m_src.size() == 0)
		{
			memset(&m_center[0], 0, sizeof(m_center[0]) * (countOfClusters + 1));
		}
		else if (m_src.size() < countOfClusters)
		{
			size_t i = 0;
			for (; i < m_src.size(); ++i)
			{
				m_center[i] = (float)m_src[i];
			}
			for (; i < countOfClusters; ++i)
			{
				m_center[i] = m_center[i - 1];
			}
		}
	}

	double Iteration()
	{
		double changeFromLast = 0.0;
		std::vector<float> newCenters(countOfClusters + 1, FLT_MAX);
		size_t readingClusterIdx = 0;
		size_t writingClusterIdx = 0;

		int countOfValues = 0;
		double sum = 0.0;
		size_t bestCluster = (size_t)-1;

		for (size_t srcIdx = 0; srcIdx < m_src.size(); ++srcIdx)
		{
			double distance = -FLT_MAX;
			double lastDistance = -FLT_MAX;

			for (size_t tryCluster = readingClusterIdx; tryCluster < countOfClusters; ++tryCluster)
			{
				double currentDistance = m_center[tryCluster] - m_src[srcIdx];

				if (std::abs(distance) >= std::abs(currentDistance))
				{
					distance = currentDistance;
					bestCluster = tryCluster;
				}
				else if (lastDistance > 0 && currentDistance > 0)
				{
					break;
				}
				lastDistance = currentDistance;
			}
			if (bestCluster != readingClusterIdx)
			{
				newCenters[writingClusterIdx] = (float)(sum / (double)countOfValues);

				double diff = newCenters[writingClusterIdx] - m_center[writingClusterIdx];
				changeFromLast += diff * diff;

				readingClusterIdx = bestCluster;
				writingClusterIdx++;
				countOfValues = 1;
				sum = m_src[srcIdx];
			}
			else
			{
				countOfValues++;
				sum += m_src[srcIdx];
			}
		}
		newCenters[writingClusterIdx] = (float)(sum / (double)countOfValues);
		double diff = newCenters[writingClusterIdx] - m_center[writingClusterIdx];
		changeFromLast += diff * diff;
		writingClusterIdx++;

		// there are emtpy spaces
		if (writingClusterIdx < countOfClusters)
		{
			if (changeFromLast < 0.0001)
			{
				size_t countOfNoData = countOfClusters - writingClusterIdx;
				size_t changeIdx = (size_t)std::max(((int)writingClusterIdx - (int)countOfNoData) / 2, 0);
				for (size_t i = 0; i < countOfNoData && i < writingClusterIdx; ++i)
				{
					float orgVal = newCenters[changeIdx + i];
					float negDiff = (changeIdx + i) == 0 ? (orgVal / 10000.0f) : ((orgVal - newCenters[changeIdx + i - 1]) / 10000.0f);
					newCenters[changeIdx + i] = orgVal - negDiff;

					float posDiff = (changeIdx + i + 1) >= countOfClusters ? (orgVal / 10000.0f) : ((newCenters[changeIdx + i + 1] - orgVal) / 10000.0f);
					newCenters[writingClusterIdx + i] = orgVal + posDiff;
				}
				for (size_t i = (writingClusterIdx + writingClusterIdx); i < countOfClusters; ++i)
				{
					newCenters[i] = (float)m_src[0];
				}
			}
			else
			{
				for (size_t i = writingClusterIdx; i < countOfClusters; ++i)
				{
					newCenters[i] = (float)m_src.back();
				}

			}
			std::sort(newCenters.begin(), newCenters.begin() + countOfClusters);
			changeFromLast = FLT_MAX; // force retry
		}
		newCenters[countOfClusters] = FLT_MAX;
		m_center.swap(newCenters);

		return changeFromLast;
	}
	void MakeSearchTable()
	{
		m_maxValue[0] = -FLT_MAX;
		for (size_t i = 0; i < (countOfClusters - 1); ++i)
		{
			m_maxValue[i + 1] = (m_center[i] + m_center[i + 1]) / 2.0f;
		}
		m_maxValue[countOfClusters] = FLT_MAX;

		m_center.insert(m_center.begin(), -FLT_MAX);
		m_center.resize(countOfClusters + 1);
	}


	LinearKmeans(const LinearKmeans&) = delete;
	LinearKmeans(LinearKmeans&&) = delete;
	LinearKmeans& operator = (const LinearKmeans&) = delete;
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(LinearKmeans);
} /* namespace Dictionary */ } /* namespace Ribbon */
