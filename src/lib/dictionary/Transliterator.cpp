#include "pch.h"
#include "Transliterator.h"
#include "DictionaryReader.h"
#include "Decoder.h"
#include "Complementer.h"

namespace Ribbon { namespace Transliteration {

struct Transliterator :
	public std::enable_shared_from_this<Transliterator>,
	public ITransliterator,
	public IObject
{
	Transliterator() {}
	virtual ~Transliterator() {}

	std::shared_ptr<Dictionary::IDictionaryReader> GetDictinoaryReader() override
	{
		if (!m_isInitialized) EnsureInitialized();

		return m_dictionaryReader;
	}

	std::shared_ptr<ILattice> StringToInputLattice(const char16_t* srcText) override
	{
		if (!m_isInitialized) EnsureInitialized();

		std::shared_ptr<ILattice> inputLattice = FACTORYCREATE(Lattice);
		for (int ich = 0; srcText[ich] != 0; ++ich)
		{
			std::shared_ptr<IPrimitive> primitive = FACTORYCREATE(Primitive);
			primitive->Reading(RefString(srcText + ich, 1));
			primitive->TopEndFrame(MakeTopEndFrame(ich, ich + 1));
			inputLattice->Add(primitive);
		}
		return inputLattice;
	}

	RefString SimpleStringConversion(const char16_t* srcText) override
	{
		if (!m_isInitialized) EnsureInitialized();

		std::shared_ptr<IPhraseList> phraseList = RunDecoder(StringToInputLattice(srcText).get());

		return phraseList->Phrase(0)->Display();
	}

	RefString SimpleStringPrediction(const char16_t* srcText) override
	{
		if (!m_isInitialized) EnsureInitialized();

		std::shared_ptr<IPhraseList> phraseList = RunComplementer(StringToInputLattice(srcText).get());

		return phraseList->Phrase(0)->Display();
	}

	std::shared_ptr<IPhraseList> Query(ILattice* lattice) override
	{
		if (!m_isInitialized) EnsureInitialized();

		std::shared_ptr<IPhraseList> complementList = RunComplementer(lattice);
		std::shared_ptr<IPhraseList> decodedList = RunDecoder(lattice);

		std::shared_ptr<IPhraseList> finalList = FACTORYCREATE(PhraseList);
		std::set<RefString> displayCache;

		// Unique display
		int predictionCount = complementList->PhraseCount();
		for (int i = 0; i < predictionCount; ++i) {
			RefString displayText = complementList->Phrase(i)->Display();
			THROW_IF_FALSE(!!displayText);

			auto insertRes = displayCache.insert(displayText);
			if (insertRes.second) {
				finalList->Push(complementList->Phrase(i));
			}
		}

		int appendCount = decodedList->PhraseCount();
		for (int i = 0; i < appendCount; ++i) {
			RefString displayText = decodedList->Phrase(i)->Display();
			THROW_IF_FALSE(!!displayText);

			auto insertRes = displayCache.insert(displayText);
			if (insertRes.second) {
				finalList->Push(decodedList->Phrase(i));
			}
		}

		return finalList;
	}

private:
	std::shared_ptr<Dictionary::IDictionaryReader>  m_dictionaryReader;
	bool m_isInitialized = false;

	void EnsureInitialized()
	{
		if (m_isInitialized) return;

		std::shared_ptr<ISetting> setting = Platform->GetSettings();
		const char* dictionariesSection = "Dictionaries";
		std::string  dictionaryFile  = setting->GetExpandedString(dictionariesSection, "SystemDictionary");
		m_dictionaryReader = Dictionary::FACTORYCREATE(DictionaryReader);
		m_dictionaryReader->Open(dictionaryFile.c_str());

		m_isInitialized = true;
	}

	std::shared_ptr<IPhraseList> RunDecoder(ILattice* inputLattice)
	{
		auto decoder = Transliteration::FACTORYCREATE(Decoder);

		DECODERARAM param;
		param.dictionary = m_dictionaryReader.get();

		return decoder->Decode(param, inputLattice);
	}

	std::shared_ptr<IPhraseList> RunComplementer(ILattice* inputLattice)
	{
		auto complementer = FACTORYCREATE(Complementer);

		COMPLEMENTPARAM param;
		param.dictionaryReader = m_dictionaryReader.get();
		param.maxPredictionItems = -1;
		param.determinedFrame = -1;

		return complementer->Complement(param, inputLattice);
	}

	//
	Transliterator (const Transliterator&) = delete;
	Transliterator& operator = (const Transliterator&) = delete;
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(Transliterator);
} /* namespace Transliteration */ } /* namespace Ribbon */
