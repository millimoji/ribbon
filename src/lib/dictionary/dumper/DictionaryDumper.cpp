#include "pch.h"
#include "../DictionaryFormat.h"
#include "../DictionaryReader.h"

namespace Ribbon { namespace Dictionary {

struct DictionaryDumper :
	public std::enable_shared_from_this<DictionaryDumper>,
	public IProgramMain,
	public IObject
{
	std::shared_ptr<ISetting> m_setting;

	FILE *pfUnigram = nullptr;
	FILE *pfBigram = nullptr;
	FILE *pfPosBigram = nullptr;
	FILE *pfPrediction = nullptr;
	FILE *pfPredCache = nullptr;

	std::shared_ptr<IDictionaryReader> m_dictionaryReader;
	PosNameReader posNameReader;

	const float* m_posInsideTable;
	const float* m_posPhraseTable;
	const float* m_backOffTable;
	const float* m_bigramTable;
	const float* m_posBigramTable;

	int ProgramMain(int, char16_t const* const*) override
	{
		int returnVal = 0;
		try
		{
			m_setting = Platform->GetSettings();
			const char* dictionariesSection = "Dictionaries";
			std::string dictionaryFile = m_setting->GetExpandedString(dictionariesSection, "SystemDictionary");

			m_dictionaryReader = FACTORYCREATE(DictionaryReader);
			m_dictionaryReader->Open(dictionaryFile.c_str());
			posNameReader = m_dictionaryReader->CreatePosNameReader();

			m_posInsideTable = m_dictionaryReader->GetPosInsideTable();
			m_posPhraseTable = m_dictionaryReader->GetPosPhraseTable();
			m_backOffTable = m_dictionaryReader->GetBackOffTable();
			m_bigramTable = m_dictionaryReader->GetWordBigramTable();
			m_posBigramTable = m_dictionaryReader->GetPosBigramTable();

			DumpPorbablityTable();

			{
				auto scopeExit = ScopeExit([&]() {
					if (pfUnigram) fclose(pfUnigram);
					if (pfBigram) fclose(pfBigram);
					if (pfPosBigram) fclose(pfPosBigram);
					if (pfPrediction) fclose(pfPrediction);
					if (pfPredCache) fclose(pfPredCache);
				});

				if ((pfUnigram = fopen(m_setting->GetString(dictionariesSection, "DumpUnigram").c_str(), "wt")) != nullptr &&
					(pfBigram = fopen(m_setting->GetString(dictionariesSection, "DumpBigram").c_str(), "wt")) != nullptr &&
					(pfPosBigram = fopen(m_setting->GetString(dictionariesSection, "DumpPosBigram").c_str(), "wt")) != nullptr &&
					(pfPrediction = fopen(m_setting->GetString(dictionariesSection, "DumpPrediction").c_str(), "wt")) != nullptr &&
					(pfPredCache = fopen(m_setting->GetString(dictionariesSection, "DumpPredCache").c_str(), "wt")) != nullptr)
				{
					RecursivelyIndexTrace(m_dictionaryReader->CreateIndexTracer(), maxReadingLetter, true);

					RecursivelyIndexTrace(m_dictionaryReader->GetPosWordNgramTop(), 0x100, true);

					RecursivelyIndexTrace(m_dictionaryReader->GetPosPosNgramTop(), 0x100, false);
				}
			}
		}
		CATCH_LOG();
		return returnVal;
	}

	void DumpPorbablityTable()
	{
		FILE* pfProb;
		if ((pfProb = fopen(m_setting->GetExpandedString("Dictionaries", "DumpProbability").c_str(), "wt")) == nullptr)
		{
			return;
		}
		auto scopeExit = ScopeExit([&]() { fclose(pfProb); });

		struct X { static void WriteFloatList(FILE*pfProb, const float* ptr) {
			for (int i = 0; i < 256; i += 8)
			{
				fprintf(pfProb, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
				ptr[i + 0], ptr[i + 1], ptr[i + 2], ptr[i + 3], ptr[i + 4], ptr[i + 5], ptr[i + 6], ptr[i + 7]);
			}
		} };

		fprintf(pfProb, "PosInside\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetPosInsideTable());

		fprintf(pfProb, "PosPhrase\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetPosPhraseTable());

		fprintf(pfProb, "PosBigram\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetPosBigramTable());

		fprintf(pfProb, "PosTrigram\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetPosTrigramTable());

		fprintf(pfProb, "WordBigram\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetWordBigramTable());

		fprintf(pfProb, "WordTrigram\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetWordTrigramTable());

		fprintf(pfProb, "BackOff\n");
		X::WriteFloatList(pfProb, m_dictionaryReader->GetBackOffTable());
	}

	void RecursivelyIndexTrace(const IndexTracer& iTracer, uint32_t maxCharCode, bool isWordNgram)
	{
		for (uint32_t ch = 0x1; ch < maxCharCode; ++ch)
		{
			IndexTracer childIdx(iTracer);

			if (childIdx.GoChild((uint8_t)ch))
			{
				VocabTracer vocabTracer;
				if (childIdx.TryGetVocab(vocabTracer, true))
				{
					DumpVocab(vocabTracer, childIdx, isWordNgram);
				}

				if (!childIdx.OnLeaf())
				{
					RecursivelyIndexTrace(childIdx, maxCharCode, isWordNgram);
				}
			}
		}
	}
	void DumpVocab(VocabTracer& vocabTracer, const IndexTracer& indexTracer, bool isWordNgram)
	{
		char16_t reading[maxIndexLen];
		indexTracer.BuildLetter(reading, COUNT_OF(reading));

		DECODEDVOCAB decodedVocab;
		while (vocabTracer.Expand(decodedVocab)) {
			switch (decodedVocab.type) {
			case DECODEDVOCAB::Type::PredCache:
				for (int i = 0; i < decodedVocab.u.PredCache.predCouunt; ++i)
				{
					int predIdx = decodedVocab.u.PredCache.predIdx[i];
					VocabTracer vocabPhrase = m_dictionaryReader->CreateVocabTracer(predIdx);

					DECODEDVOCAB decodedPhrase;
					vocabPhrase.Expand(decodedPhrase);

					if (decodedPhrase.type == DECODEDVOCAB::Type::Composite)
					{
						std::u16string phraseText;
						CreatePhraseStringFromVocab(decodedPhrase, phraseText);
						fprintf(pfPredCache, "%s\t%s\n", to_utf8(reading).c_str(), to_utf8(phraseText).c_str());
					}
					else
					{
						THROW_IF_FALSE(decodedPhrase.type == DECODEDVOCAB::Type::Word);
						std::u16string wordText;
						CreateWordStringFromDecodedVocab(decodedPhrase, vocabPhrase.GetLetterIndex(), wordText);
						fprintf(pfPredCache, "%s\t%s\n", to_utf8(reading).c_str(), to_utf8(wordText).c_str());
					}
				}
				break;
			case DECODEDVOCAB::Type::Composite:
				{
					std::u16string phraseText;
					CreatePhraseStringFromVocab(decodedVocab, phraseText);
					fprintf(pfPrediction, "%s\n", to_utf8(phraseText).c_str());
				}
				break;
			case DECODEDVOCAB::Type::Word:
				{
					std::u16string wordText;
					CreateWordStringFromDecodedVocab(decodedVocab, indexTracer.GetLeafNodeIndex(), wordText);

					if (isWordNgram)
					{
						fprintf(pfUnigram, "%s\t%f\t%f\t%f\n", to_utf8(wordText).c_str(),
							m_posInsideTable[decodedVocab.u.Word.posInside],
							decodedVocab.u.Word.posBackOff == 0 ? 0.0 : m_backOffTable[decodedVocab.u.Word.posBackOff],
							decodedVocab.u.Word.predScore == 0 ? 0.0 : m_posPhraseTable[decodedVocab.u.Word.predScore]);
					}

					if (decodedVocab.u.Word.bigramSize > 0)
					{
						WriteBigramData(wordText, decodedVocab, isWordNgram);
					}
				}
				break;

			case DECODEDVOCAB::Type::Display:
			case DECODEDVOCAB::Type::Termination:
			default:
				break;
			}
		}
	}

	void WriteBigramData(const std::u16string& wordText, const DECODEDVOCAB& decodedVocab, bool isWordNgram)
	{
		THROW_IF_FALSE(decodedVocab.type == DECODEDVOCAB::Type::Word);

		BigramReader bigramReader = m_dictionaryReader->CreateBigramReader(decodedVocab.u.Word.bigramTop, decodedVocab.u.Word.bigramSize);

		bigramReader.Enumerate([&](int wordId, uint8_t probBigram) -> bool {
			VocabTracer vocab2nd = m_dictionaryReader->CreateVocabTracer(wordId);
			DECODEDVOCAB decoded2nd; vocab2nd.Expand(decoded2nd);

			std::u16string word2ndText;
			CreateWordStringFromDecodedVocab(decoded2nd, vocab2nd.GetLetterIndex(), word2ndText);

			if (isWordNgram)
			{
				fprintf(pfBigram, "%s\t%s\t%f\n",
					to_utf8(wordText).c_str(),
					to_utf8(word2ndText).c_str(),
					m_bigramTable[probBigram]);
			}
			else
			{
				fprintf(pfPosBigram, "%s\t%s\t%f\n",
					to_utf8(wordText).c_str(),
					to_utf8(word2ndText).c_str(),
					m_posBigramTable[probBigram]);
			}
			return true;
		});
	}

	void CreateWordStringFromVocab(VocabTracer& vocabTracer, std::u16string& wordString)
	{
		THROW_IF_FALSE(vocabTracer.VocabType() == VOCAB_ID_WORD);

		DECODEDVOCAB decoded;
		THROW_IF_FALSE(vocabTracer.Expand(decoded));

		CreateWordStringFromDecodedVocab(decoded, vocabTracer.GetLetterIndex(), wordString);
	}

	void CreateWordStringFromDecodedVocab(DECODEDVOCAB& decoded, int readingIndex, std::u16string& wordString)
	{
		THROW_IF_FALSE(decoded.type == DECODEDVOCAB::Type::Word);

		IndexTracer displayIdx(m_dictionaryReader->CreateIndexTracer(decoded.u.Word.display));
		char16_t displayString[maxIndexLen];
		displayIdx.BuildLetter(displayString, COUNT_OF(displayString));

		IndexTracer readingIdx(m_dictionaryReader->CreateIndexTracer(readingIndex));
		char16_t readingString[maxIndexLen];
		readingIdx.BuildLetter(readingString, COUNT_OF(readingString));

		const char16_t* posName = posNameReader.GetPosName(decoded.u.Word.pos);

		char wordStringWork[maxIndexLen * 4];
		if (displayString[0] == PREFIX_POS_IN_POSNGRAM || displayString[0] == PREFIX_POS_IN_WORDNGRAM)
		{
			sprintf(wordStringWork, "*/*/%s", to_utf8(posName).c_str());
		}
		else
		{
			sprintf(wordStringWork, "%s/%s/%s", to_utf8(displayString).c_str(), to_utf8(readingString).c_str(), to_utf8(posName).c_str());
		}
		wordString = to_utf16(wordStringWork);
	}

	void CreatePhraseStringFromVocab(DECODEDVOCAB& decoded, std::u16string& phraseText)
	{
		THROW_IF_FALSE(decoded.type == DECODEDVOCAB::Type::Composite);

		for (int i = 0; i < decoded.u.Composite.wordCount; ++i)
		{
			VocabTracer vocabWord = m_dictionaryReader->CreateVocabTracer(decoded.u.Composite.wordIdx[i]);

			std::u16string wordText;
			CreateWordStringFromVocab(vocabWord, wordText);
		
			(phraseText += wordText) += u'\t';
		}

		char scoreBuf[128];
		sprintf(scoreBuf, "%f", m_posPhraseTable[decoded.u.Composite.predScore]);
		phraseText += to_utf16(scoreBuf);
	}

	DictionaryDumper() {}
	virtual ~DictionaryDumper() {}
	DictionaryDumper(const DictionaryDumper&) = delete;
	DictionaryDumper& operator = (const DictionaryDumper&) = delete;
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE2(IProgramMain, DictionaryDumper);
} /* namespace Dictionary */ } /* namespace Ribbon */
