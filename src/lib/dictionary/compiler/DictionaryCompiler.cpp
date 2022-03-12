#include "pch.h"
#include "../DictionaryFormat.h"
#include "../DictionaryReader.h"
#include "LangModelCalc.h"
#include "LexiconReader.h"
#include "DictionaryConstructor.h"
#include "LinearKmeans.h"
#include "PredictionCache.h"
namespace Ribbon { namespace Dictionary {

struct DictionaryCompiler :
	public std::enable_shared_from_this<DictionaryCompiler>,
	public IProgramMain,
	public IObject
{
	PosContainer m_posContainer;
	WordContainer m_wordContainer;
	PhraseContainer m_phraseContainer;
	PredCacheContainer m_predCacheContainer;

	int ProgramMain(int, char16_t const* const*) override
	{
		try
		{
			std::shared_ptr<ISetting> setting = Platform->GetSettings();

			std::shared_ptr<ILangModelCalculater> langModelCalculater = FACTORYCREATE(LangModelCalculater);

			// read from dictionary
			std::shared_ptr<ILexiconReader> lexiconReader = FACTORYCREATE(LexiconReader);
#if 0
			lexiconReader->ReadSources(std::dynamic_pointer_cast<ILexiconRegister>(langModelCalculater).get(), 3, -1);
#else
			lexiconReader->ReadSources(std::dynamic_pointer_cast<ILexiconRegister>(langModelCalculater).get(), 2, 100000);
#endif

			lexiconReader->ReadPhraseList(std::dynamic_pointer_cast<ILexiconRegister>(langModelCalculater).get(), -1);

			// calculate N-gram
			langModelCalculater->CalculateNgramProbability(m_wordContainer, m_posContainer, m_phraseContainer);

			// create prediction cache
			std::shared_ptr<IPredictionCacheMaker> predCache = FACTORYCREATE(PredictionCacheMaker);
			predCache->Build(m_phraseContainer, m_predCacheContainer);

			// output to file
			std::shared_ptr<IDictionaryConstructor> dictionaryConstructor = FACTORYCREATE(DictionaryConstructor);
			dictionaryConstructor->BuildDictionary(m_wordContainer, m_posContainer, m_phraseContainer, m_predCacheContainer);

			GlobalThreadPool.Clear(); // before exiting, we need to kill sub threads.
			exit(0); // in order to skip heavy destructor, exiting program here.
		}
		catch (...)
		{
			GlobalThreadPool.Clear(); // before exiting, we need to kill sub threads.
			try { throw;  } CATCH_LOG();
		}
		return 0;
	}

	virtual ~DictionaryCompiler() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE2(IProgramMain, DictionaryCompiler);
} /* namespace Dictionary */ } /* namespace Ribbon */
