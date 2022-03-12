#include "pch.h"
#include "../DictionaryFormat.h"
#include "LexiconReader.h"
namespace Ribbon { namespace Dictionary {
#ifdef _WINDOWS
	// Note: This file assumes sizeof(wchar_t) == 2 and using 'w' functions for utf16

static const char* s_nGramFileNameKey[] = {
	nullptr, // 0
	"SourceUnigram",
	"SourceBigram",
	"SourceTrigram",
	"SourceN4gram",
	"SourceN5gram",
	"SourceN6gram",
	"SourceN7gram",
};

// const
static const char s_dictionariesSection[] = "Dictionaries";

struct LexiconReader :
	public std::enable_shared_from_this<LexiconReader>,
	public ILexiconReader,
	public IObject
{
	// member vars
	ILexiconRegister* m_lexiconRegister;
	int m_katzThreshold = 10;

	// methods
	LexiconReader() {}
	virtual ~LexiconReader() {}

	void ReadSources(ILexiconRegister* lexiconRegister, int nGram, int lowerBound) override
	{
		m_lexiconRegister = lexiconRegister;
		std::shared_ptr<ISetting> setting = Platform->GetSettings();

		m_katzThreshold = setting->GetInt(s_dictionariesSection, "KatzThreshold");

		ReadAlllDics(setting->GetString(s_dictionariesSection, "SourceLexicon").c_str());

		for (int i = 1; i <= nGram; ++i)
		{
			const auto& fileName = setting->GetString(s_dictionariesSection, s_nGramFileNameKey[i]);
			ReadNgramFile(fileName.c_str(), i, lowerBound);
		}
	}

	struct COLUMN {
		static const int DISPLAY = 0;
		static const int POS1 = 1;
		static const int POS2 = 2;
		static const int POS3 = 3;
		static const int POS4 = 4;
		static const int POS5 = 5;
		static const int POS6 = 6;
		static const int BASEDISP = 7;
		static const int READING = 8;
		static const int READING2 = 9;
		static const int MAX = 10;

		static const int IPA_UNKNUM_BEGIN = 1;
		static const int IPA_UNKNUM_LENGTH = 3;
	};

	void ReadAlllDics(const char* dicsFolder)
	{
		std::vector<char16_t*> elements(COLUMN::MAX);

		auto fileNames = Platform->EnumerateFiles(dicsFolder, "*.csv");
		for (auto& fileName : fileNames)
		{
			printf("%s: ", fileName.c_str());
			std::string fileNameBuf(dicsFolder);
			fileNameBuf += CHAR_PATH_DELIMITOR;
			fileNameBuf += fileName;

			int lineCount = 0;
			ReadLineCallback(fileNameBuf.c_str(), false, [&](char16_t* line) -> bool {
				lineCount++;

				WORDINFO wordInfo;
				std::u16string posstring;
				if (MakeWordInfo(wordInfo, line, posstring))
				{
					m_lexiconRegister->RegisterWord(&wordInfo);
				}
				else
				{
					printf("Failed to add [%ls]\n", reinterpret_cast<wchar_t*>(line));
				}
				return true; // continue
			});

			printf("%d\n", lineCount);
		}
	}

	void ReadNgramFile(const char* fileName, int nGram, int lowerBound)
	{
		printf("%d-gram: %s => ", nGram, fileName);

		std::vector<char16_t*> wordList(10);
		struct WORDINFOANDBUF
		{
			std::u16string posstring;
		};
		WORDINFOANDBUF wordInfoBuf[10];
		WORDINFO wordInfo[10];

		int lineCount = 0;
		ReadLineCallback(fileName, true, [&](char16_t* line) -> bool {
			lineCount++;

			SplitText(line, u"\t\r\n", wordList);

			char16_t* nextPtr;
			uint64_t count = wcstoull(
				reinterpret_cast<wchar_t*>(wordList[(size_t)nGram]),
				reinterpret_cast<wchar_t**>(&nextPtr), 10);

			if (lowerBound > 0 && count < static_cast<uint64_t>(count)) {
				return false;
			}

			bool isFailed = false;
			for (size_t i = 0; i < (size_t)nGram; ++i)
			{
				if (!MakeWordInfo(wordInfo[i], wordList[i], wordInfoBuf[i].posstring))
				{
					isFailed = true;
					break;
				}
			}
			if (!isFailed)
			{
				m_lexiconRegister->SetNgram(nGram, wordInfo, count);
			}
			return true; // continue
		});
		printf("%d lines\n", lineCount);
	}

	void ReadLineCallback(const char* fileName, bool isUnicode, const std::function<bool(char16_t*)>& callback)
	{
		const char* openMode = isUnicode ? "rt, ccs=UNICODE" : "rt";
		setlocale(LC_ALL, "japanese");

		FILE *pf(nullptr);
		if ((pf = fopen(fileName, openMode)) == nullptr)
		{
			return;
		}
		auto scopeExit = ScopeExit([&]() { fclose(pf); });

		for (;;)
		{
			char16_t readBuf[1024];
			if (fgetws(reinterpret_cast<wchar_t*>(readBuf), COUNT_OF(readBuf), pf) == nullptr)
			{
				return;
			}
			if (!callback(readBuf))
			{
				return;
			}
		}
	}

	static void SplitText(char16_t* src, const char16_t* separator, std::vector<char16_t*>& result)
	{
		result.clear();
		const wchar_t* sepAlias = reinterpret_cast<const wchar_t*>(separator);
		wchar_t* token = reinterpret_cast<wchar_t*>(src);
		wchar_t* context;

		for (token = wcstok_s(token, sepAlias, &context);
			token != nullptr;
			token = wcstok(nullptr, sepAlias, &context))
		{
			result.push_back(reinterpret_cast<char16_t*>(token));
		}
	}

	bool NormalizePOS(const std::vector<char16_t*>& elements, std::u16string& posResult)
	{
		if (textcmp(elements[COLUMN::POS1], TEXT_POS_JOSHI) == 0 ||
			textcmp(elements[COLUMN::POS1], TEXT_POS_JODOSHI) == 0)
		{
			OverwriteKatakanaToHiragana(elements[COLUMN::READING]);
			posResult = elements[COLUMN::READING];
		}
		else
		{
			posResult = u"*";
		}
		(posResult += u",") += elements[COLUMN::POS1];
		(posResult += u",") += elements[COLUMN::POS2];
		(posResult += u",") += elements[COLUMN::POS3];
		(posResult += u",") += elements[COLUMN::POS4];
		(posResult += u",") += elements[COLUMN::POS5];
		(posResult += u",") += elements[COLUMN::POS6];
		return true;
	}

	bool MakeWordInfo(WORDINFO& wordInfo, char16_t* srcText, std::u16string& posString)
	{
		std::vector<char16_t*> wordAttr(16);
		SplitText(srcText, u",\r\n", wordAttr);

		if (wordAttr.size() < COLUMN::MAX)
		{
			if (textcmp(srcText, TEXT_BOS) == 0)
			{
				wordInfo.disp = POSID_BOS_IN_WORDNGRAM;
				wordInfo.read = POSID_BOS_IN_WORDNGRAM;
				wordInfo.pos = TEXT_BOS;
				return true;
			}
			else if (textcmp(srcText, TEXT_EOS) == 0)
			{
				wordInfo.disp = POSID_EOS_IN_WORDNGRAM;
				wordInfo.read = POSID_EOS_IN_WORDNGRAM;
				wordInfo.pos = TEXT_EOS;
				return true;
			}
			// TODO: make unknown words, here.
			return false;
		}
		else if (wordAttr.size() > COLUMN::MAX)
		{
			// adjust column for IPA dic in mecab
			wordAttr.erase(wordAttr.begin() + COLUMN::IPA_UNKNUM_BEGIN,
				wordAttr.begin() + COLUMN::IPA_UNKNUM_BEGIN + COLUMN::IPA_UNKNUM_LENGTH);
		}

		wordInfo.disp = wordAttr[COLUMN::DISPLAY];
		wordInfo.read = wordAttr[COLUMN::READING];
		NormalizePOS(wordAttr, posString);
		wordInfo.pos = posString.c_str();

		// change reading to hiragana
		OverwriteKatakanaToHiragana(wordAttr[COLUMN::READING]);

		return true;
	}

	void OverwriteKatakanaToHiragana(char16_t* kanaText)
	{
		for (char16_t* pch = kanaText; *pch != 0; ++pch)
		{
			if (*pch >= 0x30A1 && *pch <= 0x30f6)
			{
				*pch = (char16_t)(*pch - (0x30a1 - 0x3041));
			}
		}
	}

	std::u16string WordInfoToString(const WORDINFO* wordInfo) override
	{
		if (wordInfo->pos[0] == u'[')
		{
			return std::u16string(wordInfo->pos);
		}

		size_t orgPosLen = textlen(wordInfo->pos);
		std::vector<char16_t> posWorkBuf(orgPosLen + 1);
		memcpy(posWorkBuf.data(), wordInfo->pos, (orgPosLen + 1) * sizeof(char16_t));

		std::vector<char16_t*> posFields;
		SplitText(posWorkBuf.data(), u",\t\r\n", posFields);

		std::u16string result;
		(result += wordInfo->disp) += u',';
		(result += posFields[COLUMN::POS1]) += u',';
		(result += posFields[COLUMN::POS2]) += u',';
		(result += posFields[COLUMN::POS3]) += u',';
		(result += posFields[COLUMN::POS4]) += u',';
		(result += posFields[COLUMN::POS5]) += u',';
		(result += posFields[COLUMN::POS6]) += u',';
		result += u"*,"; // BASEDISP
		(result += wordInfo->read) += u',';
		result += wordInfo->read;

		return result;
	}

	void ReadPhraseList(ILexiconRegister* lexiconRegister, int lowerBound) override
	{
		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		std::string phraseListFile = setting->GetString(s_dictionariesSection, "PhraseList");

		WORDINFO wordInfo[10];
		struct {
			std::u16string posstring;
		} wordInfoBuf[10];

		int lineCount = 0;
		ReadLineCallback(phraseListFile.c_str(), true, [&](char16_t* line) -> bool {
			++lineCount;

			std::vector<char16_t*> fieldList;
			SplitText(line, u"\t\r\n", fieldList);

			char16_t* nextPtr;
			uint64_t count = wcstoull(
				reinterpret_cast<wchar_t*>(fieldList.back()),
				reinterpret_cast<wchar_t**>(&nextPtr), 10);

			if (lowerBound > 0 && count < static_cast<uint64_t>(lowerBound))
			{
				return false;
			}

			// top is sample display, end is count.
			int nGram = static_cast<int>(fieldList.size()) - 2;

			bool isFailed = false;
			for (size_t i = 0; i < (size_t)nGram; ++i)
			{
				if (!MakeWordInfo(wordInfo[i], fieldList[i + 1], wordInfoBuf[i].posstring))
				{
					isFailed = true;
					break;
				}
			}
			if (!isFailed)
			{
				lexiconRegister->SetPhrase(nGram, wordInfo, count);
			}
			return true; // continue
		});
	}

	IOBJECT_COMMON_METHODS
};

#else
struct LexiconReader :
	public std::enable_shared_from_this<LexiconReader>,
	public ILexiconReader,
	public IObject
{
	IOBJECT_COMMON_METHODS
};
#endif // _WINDOWS
FACTORYDEFINE(LexiconReader);
} /* namespace Dictionary */ } /* namespace Ribbon */
