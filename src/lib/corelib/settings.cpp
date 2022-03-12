#include "pch.h"

namespace Ribbon {

struct FileSetting :
	public std::enable_shared_from_this<FileSetting>,
	public ISetting,
	public IObject
{
	FileSetting() {}
	virtual ~FileSetting()
	{
		if (m_isWritable && m_isDirty) {
			try {
				FlushToFile();
			}
			CATCH_LOG();
		}
	}

	virtual void SetFilename(const char* fileName, bool isWritable) override
	{
		m_fileName = fileName;
		m_configData.clear();
		m_readAlready = false;
		m_isWritable = isWritable;
	}

	void CascadeReadonlySetting(const std::shared_ptr<ISetting>& setting) override
	{
		if (!m_cascadedSetting) {
			m_cascadedSetting = setting;
		}
		else {
			m_cascadedSetting->CascadeReadonlySetting(setting);
		}
	}

	bool HasKey(const char* section, const char* key) const override
	{
		if (!m_readAlready) EnsureSettingRead();

		auto sectionFindRes = m_configData.find(section);
		if (sectionFindRes == m_configData.end()) {
			return m_cascadedSetting ? m_cascadedSetting->HasKey(section, key) : false;
		}
		auto keyFindRes = sectionFindRes->second.find(key);
		if (keyFindRes == sectionFindRes->second.end()) {
			return m_cascadedSetting ? m_cascadedSetting->HasKey(section, key) : false;
		}
		return true;
	}

	const std::string& GetString(const char* section, const char* key) const override
	{
		if (!m_readAlready) EnsureSettingRead();

		auto sectionFindRes = m_configData.find(section);
		if (sectionFindRes == m_configData.end()) {
			return m_cascadedSetting ? m_cascadedSetting->GetString(section, key) : s_emptyString;
		}
		auto keyFindRes = sectionFindRes->second.find(key);
		if (keyFindRes == sectionFindRes->second.end()) {
			return m_cascadedSetting ? m_cascadedSetting->GetString(section, key) : s_emptyString;
		}
		return keyFindRes->second;
	}

	std::string GetExpandedString(const char* section, const char* key) const override
	{
		if (!m_readAlready) EnsureSettingRead();

		auto sectionFindRes = m_configData.find(section);
		if (sectionFindRes == m_configData.end()) {
			return m_cascadedSetting ? m_cascadedSetting->GetExpandedString(section, key) : s_emptyString;
		}
		auto keyFindRes = sectionFindRes->second.find(key);
		if (keyFindRes == sectionFindRes->second.end()) {
			return m_cascadedSetting ? m_cascadedSetting->GetExpandedString(section, key) : s_emptyString;
		}

		const auto& foundValue = keyFindRes->second;
		size_t targetPos = 0;
		if ((targetPos = foundValue.find("%BinPath%")) != foundValue.npos) {
			auto binPath = Platform->GetPathByType(IPlatform::PathType::BinPath);
			std::string result = foundValue.substr(0, targetPos);
			result += binPath;
			result += foundValue.substr(targetPos + strlen("%BinPath%"));
			return result;
		}
		if ((targetPos = foundValue.find("%UserPath%")) != foundValue.npos) {
			auto binPath = Platform->GetPathByType(IPlatform::PathType::UserPath);
			std::string result = foundValue.substr(0, targetPos);
			result += binPath;
			result += foundValue.substr(targetPos + strlen("%UserPath%"));
			return result;
		}

		return keyFindRes->second;
	}

	bool SetString(const char* section, const char* key, const char* value) override
	{
		if (!m_readAlready) EnsureSettingRead();
		if (m_isWritable) {
			SetStringInternal(section, key, value);
			m_isDirty = true;
			return true;
		}
		return m_cascadedSetting ? m_cascadedSetting->SetString(section, key, value) : false;
	}

protected:
	static std::string s_emptyString;
	std::string m_fileName;
	std::shared_ptr<ISetting> m_cascadedSetting;
	mutable std::set<std::string> m_stringPool;
	mutable std::map<const char*, std::map<const char*, std::string, textCompare<char>>, textCompare<char>> m_configData;
	mutable bool m_readAlready = false;
	mutable bool m_isWritable = false;
	bool m_isDirty = false;

	void EnsureSettingRead() const
	{
		if (m_readAlready) return;
		m_readAlready = true;
		const_cast<FileSetting*>(this)->UpdateDataFromFile();
	}

	void UpdateDataFromFile()
	{
		std::string currentSection("default");
		m_configData.clear();

		try {
			ReadLineCallback(m_fileName.c_str(), [&](const char* key, const char* value) -> bool {
				if (value == nullptr) {
					// change section
					currentSection = key;
				}
				else {
					SetStringInternal(currentSection.c_str(), key, value);
				}
				return true;
			});
		}
		catch (...) {
			// ignore simple reading failure
		}
	}

	void SetStringInternal(const char* section, const char* key, const char* value)
	{
		auto sectionInsert = m_stringPool.insert(std::string(section));
		std::pair<const char*, std::map<const char*, std::string, textCompare<char>>> insertPair
			= std::make_pair(sectionInsert.first->c_str(), std::map<const char*, std::string, textCompare<char>>());
		auto categoryInsert = m_configData.insert(insertPair);

		auto keyInsert = m_stringPool.insert(std::string(key));
		auto keyValueInsert = categoryInsert.first->second.insert(std::make_pair(keyInsert.first->c_str(), std::string()));
		keyValueInsert.first->second = value;
	}

	static inline bool IsLineBreak(char ch) { return ch == '\r' || ch == '\n'; }
	static inline bool IsCommentChar(char ch) { return ch == ';'; }
	static inline bool IsMultiLineChar(char ch) { return ch == '\\'; }
	static inline bool IsWhiteSpace(char ch) { return ch == ' ' || ch == '\t'; }
	static inline bool IsSecsion(const std::string& text) { return text.length() > 2 && text.front() == '[' && text.back() == ']'; }

	bool ReadLineCallback(const char* fileName, const std::function<bool (const char*, const char*)>& callback) const
	{
		auto mmapFile = Platform->OpenMemoryMappedFile(fileName);
		auto fileSize = mmapFile->GetFileSize();
		auto topAddr = reinterpret_cast<const char*>(mmapFile->GetAddress());
		auto endAddr = topAddr + fileSize;

		if (topAddr == nullptr) {
			return true;
		}
		// skip UTF8 BOM if exists
		{
			const unsigned char* addr = reinterpret_cast<const unsigned char*>(topAddr);
			if (addr[0] == 0xEF && addr[1] == 0xBB && addr[2] == 0xBF) {
				topAddr += 3;
			}
		}

		std::string multiLine;

		while (topAddr < endAddr) {
			char workBuf[2048];

			// Get Line
			{
				auto curAddr = topAddr;
				for (; (curAddr < endAddr) && !IsLineBreak(*curAddr); ++curAddr) ;
				THROW_IF_FALSE((curAddr - topAddr + 1) < COUNT_OF(workBuf));
				memcpy(workBuf, topAddr, static_cast<size_t>(curAddr - topAddr));
				workBuf[curAddr - topAddr] = 0;

				auto nextAddr = curAddr;
				for (; (nextAddr < endAddr) && IsLineBreak(*nextAddr); ++nextAddr);
				topAddr = nextAddr;
			}

			// Chop comment
			char* endLineAddr = workBuf;
			for (; *endLineAddr != 0 && !IsCommentChar(*endLineAddr); ++endLineAddr);
			*endLineAddr = 0;

			// multiline
			bool isMultiLine = false;
			if (endLineAddr > workBuf && IsMultiLineChar(endLineAddr[-1])) {
				isMultiLine = true;
				--endLineAddr;
				*endLineAddr = 0;
			}

			// Chop tail white space
			for (; (endLineAddr > workBuf) && IsWhiteSpace(endLineAddr[-1]); --endLineAddr);
			*endLineAddr = 0;

			// Chop heading white space
			char* topLineAddr = workBuf;
			for (; (topLineAddr < endLineAddr) && IsWhiteSpace(*topLineAddr); ++topLineAddr);

			// multi line case, appding connect
			multiLine += std::string(topLineAddr, static_cast<size_t>(endLineAddr - topLineAddr));
			if (isMultiLine) {
				continue;
			}

			// is Section?
			if (IsSecsion(multiLine)) {
				callback(multiLine.substr(1, multiLine.length() - 2).c_str(), nullptr);
			}
			else
			{
				auto equalPos = multiLine.find('=');
				if (equalPos != std::string::npos) {
					callback(multiLine.substr(0, equalPos).c_str(), multiLine.substr(equalPos + 1).c_str());
				}
			}
			multiLine.clear();
		}
		return true;
	}


	void FlushToFile()
	{
		if (!m_isDirty) {
			return;
		}
		m_isDirty = false;

		FILE *pf(nullptr);
		THROW_IF_FALSE((pf = fopen(m_fileName.c_str(), "w")) != nullptr);
		auto scopeExit = ScopeExit([&]() { fclose(pf); });

		for (auto& section : m_configData) {
			if (section.second.size() == 0) {
				continue;
			}
			fprintf(pf, "[%s]\n", section.first);
			for (auto& kvp : section.second) {
				fprintf(pf, "%s=%s\n", kvp.first, kvp.second.c_str());
			}
		}
	}

	IOBJECT_COMMON_METHODS
};

std::string FileSetting::s_emptyString;

FACTORYDEFINE2(ISetting, FileSetting);
} // Ribbon
