#include "pch.h"
#include "EditLine.h"
#include "IEditContext.h"
#include "InputModel.h"
#include "KeyEventProcessor.h"
#include "SymEmojiList.h"
#include "TransliterationAdapter.h"
#include "dictionary/Transliterator.h"
#include "dictionary/DictionaryReader.h"
#include "history/HistoryManager.h"
#include "history/HistoryReuser.h"
#include "history/HistoryStore.h"

namespace Ribbon {

class EnglishPredictionHelper
{
private:
	std::shared_ptr<ILiteralUtility> m_englishUtility;
	std::shared_ptr<History::IHistoryManager> m_historyMangerEng;

	std::shared_ptr<IPhrase> m_contextWords;
	RefString m_predictedWord;
	bool m_isNextWordPrediction = false;
	bool m_keepTypingWhilePrediciton = false;
	bool m_showingEnglishPrediction = false;
public:
	EnglishPredictionHelper(const std::shared_ptr<ILiteralUtility>& englishUtility, const std::shared_ptr<History::IHistoryManager>& historyManagerEng) :
		m_englishUtility(englishUtility), m_historyMangerEng(historyManagerEng)
	{}

	void HandleKeyDown(const std::shared_ptr<KeyEvent>& keyEvent, IEditContext* editContext)
	{
		m_isNextWordPrediction = false;
		m_contextWords.reset();

		if ((editContext->GetKeyEventProcessor()->ModifierState() & IME_ON) != 0) {
			return; // just in case
		}

		auto contextString = editContext->GetContextString();
		if (contextString.length() <= 0) {
			return;
		}

		const char16_t* labelText = keyEvent->label.u16ptr();
		m_contextWords = m_englishUtility->WordBreak(contextString, labelText[0]);

		m_isNextWordPrediction = false;
		if (m_contextWords->PrimitiveCount() == 0 || m_englishUtility->IsNextWordPrediction(contextString, labelText[0])) {
			m_isNextWordPrediction = true;
		}

		StoreInputWord(keyEvent, editContext);
		PenaltyEvaluatePrediction(keyEvent, editContext);

		std::shared_ptr<IPhraseList> predictionList = m_isNextWordPrediction ? BuildNextWordPrediction() : BuildTypeAheadPrediction();

		ProcessPredictionPhraseList(predictionList, editContext);
	}

private:
	void StoreInputWord(const std::shared_ptr<KeyEvent>& keyEvent, IEditContext* editContext)
	{
		auto contextString = editContext->GetContextString();
		const char16_t* labelText = keyEvent->label.u16ptr();

		// put history if the word seperator is inserted.
		if (m_englishUtility->NeedToStoreHistory(contextString, labelText[0])) {
			// put history
			m_historyMangerEng->GetReuser()->RegisterLastWordWithOpenPhrases(m_contextWords);
		}
	}

	void PenaltyEvaluatePrediction(const std::shared_ptr<KeyEvent>& keyEvent, IEditContext* editContext)
	{
		auto contextString = editContext->GetContextString();
		const char16_t* labelText = keyEvent->label.u16ptr();

		// compre predicted word and typed word
		if (labelText[0] == u' ' && m_keepTypingWhilePrediciton) {
			if (m_contextWords->PrimitiveCount() > 0) {
				auto typedLastWord = m_contextWords->Primitive(m_contextWords->PrimitiveCount() - 1);
				if (typedLastWord->Display() == m_predictedWord) {
					// predicted word is full typed manually
					auto phraseTmp = FACTORYCREATE(Phrase);
					phraseTmp->Push(typedLastWord);
					m_historyMangerEng->GetReuser()->GetReinforce()->SetPenalty(phraseTmp, 0);
				}
			}
		}
	}

	std::shared_ptr<IPhraseList> BuildNextWordPrediction()
	{
		m_historyMangerEng->GetReuser()->SetContext(m_contextWords);
		return m_historyMangerEng->GetReuser()->Query(nullptr);
	}

	std::shared_ptr<IPhraseList> BuildTypeAheadPrediction()
	{
		auto wordCount = m_contextWords->PrimitiveCount();
		auto contextPhrase = FACTORYCREATE(Phrase);
		for (int i = 0; i < (wordCount - 1); ++i) {
			contextPhrase->Push(m_contextWords->Primitive(i));
		}
		auto wordHeading = m_contextWords->Primitive(wordCount - 1);
		wordHeading->TopEndFrame(std::make_tuple((uint16_t)0, (uint16_t)1));
		auto queryLattice = FACTORYCREATE(Lattice);
		queryLattice->Add(wordHeading);

		m_historyMangerEng->GetReuser()->SetContext(contextPhrase);
		return m_historyMangerEng->GetReuser()->Query(queryLattice);
	}

	void ProcessPredictionPhraseList(const std::shared_ptr<IPhraseList>& predictionList, IEditContext* editContext)
	{
		// only use the first word in the first candidate
		std::shared_ptr<IPhraseList> finalCandidate;
		m_predictedWord.clear();
		if (predictionList->PhraseCount() > 0) {
			auto phrase = predictionList->Phrase(0);
			if (phrase->PrimitiveCount() > 0) { // TODO:
				auto firstPrim = phrase->Primitive(0);
				if (firstPrim->Display().length() > 0) {
					auto candidatePhrase = FACTORYCREATE(Phrase);
					candidatePhrase->Push(firstPrim);
					finalCandidate = FACTORYCREATE(PhraseList);
					finalCandidate->Push(candidatePhrase);

					// TODO
					m_predictedWord = firstPrim->Display();
					if (m_isNextWordPrediction) {
						m_keepTypingWhilePrediciton = true;
					}
				}
			}
		}
		if (m_isNextWordPrediction && finalCandidate) {
			finalCandidate->BitFlags(PHRASELIST_BIT_NEXTWORD);
		}
		editContext->SetCandidates(finalCandidate);
		m_showingEnglishPrediction = true;
	}
};


struct TransliterationAdapter :
	public std::enable_shared_from_this<TransliterationAdapter>,
	public ITransliterationAdapter,
	public IObject
{
	std::shared_ptr<Transliteration::ITransliterator> m_transliterator;
	std::shared_ptr<History::IHistoryManager> m_historyMangerEng;
	std::shared_ptr<ISymbolEmojiList> m_symbolEmojiList;
	std::shared_ptr<ILiteralUtility> m_englishUtility;
	bool m_showingSymbolEmojiList = false;

	EnglishPredictionHelper m_englishPredictionHelper;

	TransliterationAdapter() :
		m_historyMangerEng(FACTORYCREATENS(History, HistoryManager)),
		m_englishUtility(FACTORYCREATE(EnglishUtility)),
		m_englishPredictionHelper(m_englishUtility, m_historyMangerEng)
	{
		m_transliterator = FACTORYCREATENS(Transliteration, Transliterator);
		m_symbolEmojiList = FACTORYCREATE(SymbolEmojiList);
		m_historyMangerEng->Initialize(m_transliterator->GetDictinoaryReader().get(), "en.", -1);
		m_historyMangerEng->Load(); // TODO: do sub thread
	}

	// ITransliterationAdapter

	// ITaskProcessor
	void ProcessTask(TaskType task, const std::shared_ptr<BaseEvent>& baseEvent, IEditContext* editContext) override
	{
		switch (task) {
		case TaskType::KillFocus:
			TaskHandler_KillFocus(baseEvent, editContext);
			break;
		case TaskType::IMEOffInput:
			TaskHandler_IMEOffInput(baseEvent, editContext);
			break;
		case TaskType::CandidateUI:
			TaskHandler_CandidateUI(baseEvent, editContext);
			break;
		case TaskType::UpdateCandidate:
			TaskHandler_UpdateCandidate(baseEvent, editContext);
			break;
		case TaskType::CloseCandidate:
			TaskHandler_CloseCandidate(baseEvent, editContext);
			break;
		}
	}

	// Task Handler

	void TaskHandler_IMEOffInput(const std::shared_ptr<BaseEvent>& baseEvent, IEditContext* editContext)
	{
		if (baseEvent && baseEvent->eventType == BaseEvent::EventType::KeyDown) {
			std::shared_ptr<KeyEvent> keyEvent = std::static_pointer_cast<KeyEvent>(baseEvent);
			m_englishPredictionHelper.HandleKeyDown(keyEvent, editContext);
		}
	}

	void TaskHandler_CandidateUI(const std::shared_ptr<BaseEvent>& baseEvent, IEditContext* editContext)
	{
		THROW_IF_FALSE(baseEvent);
		switch (baseEvent->eventType) {
		case BaseEvent::EventType::CandidateChosen: {
			auto candidateEvent = std::static_pointer_cast<CandidateEvent>(baseEvent);
			int index = candidateEvent->candidateIndex;
			auto candidateList = editContext->GetDisplayedCandidateList();
			if (index >= 0 && index < candidateList->PhraseCount()) {
				// TODO: insert space, if English prediction
				editContext->SetInjectText(candidateList->Phrase(index).get());
				editContext->GetEditLine()->Clear();
				if (m_showingSymbolEmojiList) {
					;
				}
				else {
					editContext->QueueTask(TaskType::CloseCandidate, std::shared_ptr<BaseEvent>());
				}
			}
		}
		break;

		case BaseEvent::EventType::ShowSymbolEmoji: {
			auto candidateEvent = std::static_pointer_cast<CandidateEvent>(baseEvent);
			SymbolEmojiCategory category = static_cast<SymbolEmojiCategory>(candidateEvent->symEmojiCategory);
			auto emojiClass = m_transliterator->GetDictinoaryReader()->CreatePosNameReader().GetEmojiClass();
			editContext->SetCandidates(m_symbolEmojiList->GetList(category, emojiClass));
			m_showingSymbolEmojiList = true;
			//m_showingEnglishPrediction = false;
		}
		break;
		}
		
	}
	void TaskHandler_UpdateCandidate(const std::shared_ptr<BaseEvent>& /*baseEvent*/, IEditContext* editContext)
	{
		auto inputLattice = editContext->GetEditLine()->CreateInputLattice();
		auto phraseList = m_transliterator->Query(inputLattice.get());

		editContext->SetCandidates(phraseList);
		m_showingSymbolEmojiList = false;
	}
	void TaskHandler_CloseCandidate(const std::shared_ptr<BaseEvent>& /*baseEvent*/, IEditContext* editContext)
	{
		editContext->SetCandidates(std::shared_ptr<IPhraseList>());
		m_showingSymbolEmojiList = false;
	}

	void TaskHandler_KillFocus(const std::shared_ptr<BaseEvent>& /*baseEvent*/, IEditContext* editContext)
	{
		editContext->SetCandidates(std::shared_ptr<IPhraseList>());
		m_historyMangerEng->Save();
	}

	virtual ~TransliterationAdapter() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(TransliterationAdapter);
} /* namespace Ribbon */
