#include "pch.h"
#include "EditLine.h"
#include "EditFunctions.h"
#include "history\HistoryManager.h"
#include "InputModel.h"
#include "IEditContext.h"
#include "KeyEventProcessor.h"
#include "LiteralConvert.h"
#include "SymEmojiList.h"
#include "TransliterationAdapter.h"

namespace Ribbon {

std::shared_ptr<ITaskProcessor> CreateCompositionAdaptor();

struct InputModel :
	public std::enable_shared_from_this<InputModel>,
	public IInputModel,
	public IEditContext,
	public IObject
{
	std::shared_ptr<ISetting> m_settings;
	std::shared_ptr<IEditLine> m_editLine;
	std::shared_ptr<ILiteralConvert> m_literalConvert;
	std::shared_ptr<IKeyCodeHelper> m_keyCodeHelper;
	std::shared_ptr<IKeyEventProcessor> m_keyEventProcessor;
	std::shared_ptr<ITransliterationAdapter> m_transliterationAdaptor;
	std::vector<std::shared_ptr<ITaskProcessor>> m_taskProcessor;
	RefString m_contextString;

	// output data
	int m_passThrough;
	ToKeyboard m_toKeyboard = ToKeyboard::None;
	RefString m_insertText;
	std::shared_ptr<IPhrase> m_compositionText;
	std::shared_ptr<IPhraseList> m_candidateList;
	int m_caretInComposition = 0;

	InputModel()
	{
		m_settings = Platform->GetSettings();
		m_editLine = FACTORYCREATE(EditLine);
		m_literalConvert = FACTORYCREATE(JaLiteralConvert);
		m_keyCodeHelper = FACTORYCREATE(KeyCodeHelper);
		m_keyEventProcessor = FACTORYCREATE(KeyEventProcessor);
		m_transliterationAdaptor = FACTORYCREATE(TransliterationAdapter);

		m_taskProcessor.push_back(m_keyEventProcessor);
		m_taskProcessor.push_back(m_transliterationAdaptor);
		m_taskProcessor.push_back(CreateCompositionAdaptor());
	}

	// IInputModel - external tnterface 
	void EventHandler(const std::shared_ptr<BaseEvent>& baseEvent) noexcept override try
	{
		ResetVolatileState();

		QueueTask(TaskType::Preprocess, std::shared_ptr<BaseEvent>());
		QueueTask(TaskType::Postprocess, std::shared_ptr<BaseEvent>());

		switch (baseEvent->eventType) {
		case BaseEvent::EventType::KeyDown:
			QueueTask(TaskType::KeyDown, baseEvent);
			break;
		case BaseEvent::EventType::KeyUp:
			QueueTask(TaskType::KeyUp, baseEvent);
			break;
		case BaseEvent::EventType::CandidateChosen:
			QueueTask(TaskType::CandidateUI, baseEvent);
			break;
		case BaseEvent::EventType::KillFocus:
			QueueTask(TaskType::KillFocus, baseEvent);
			break;
		}

		StartEventProcessing();
	}
	catch (...) {}

	void UpdateContext(const char16_t* contextString) noexcept override try {
		m_contextString = contextString;
	} catch (...) {}

	// IInputModel - Getter
	std::shared_ptr<IPhrase> CompositionText() noexcept override try {
		return m_compositionText;
	} catch (...) { return FACTORYCREATE(Phrase); }

	std::shared_ptr<IPhraseList> CandidateList() noexcept override try {
		return m_candidateList;
	} catch (...) { return FACTORYCREATE(PhraseList); }

	RefString InsertingText() noexcept override try {
		RefString insertText = m_insertText;
		m_insertText = RefString();
		return insertText;
	} catch (...) { return RefString(); }

	int CarretPosition() noexcept override try {
		return m_caretInComposition;
	} catch (...) { return 0; }

	int GetPassThroughEvent() noexcept override try {
		return m_passThrough;
	} catch (...) { return 0; }

	ToKeyboard KeyboardState() noexcept override try {
		return m_toKeyboard;
	} catch (...) { return ToKeyboard::None; }

	bool IsInitialLaunch(bool doUpdate) noexcept override try {
		constexpr char builtDateTime[] = __DATE__ "-" __TIME__;
		constexpr char checkSection[] = "dictionary";
		constexpr char checkKey[] = "BuiltDateTime";

		bool isInitial = true;
		if (m_settings->HasKey(checkSection, checkKey)) {
			const auto& valueInSetting = m_settings->GetString(checkSection, checkKey);
			isInitial = (valueInSetting != builtDateTime);
		}

		if (doUpdate) {
			m_settings->SetString(checkSection, checkKey, builtDateTime);
		}

		return isInitial;
	} catch (...) { return true; }

	// IEditContext - internal interface
	std::map<TaskType, std::shared_ptr<BaseEvent>> m_taskBag;

	void QueueTask(TaskType taskType, const std::shared_ptr<BaseEvent>& eventArg) override {
		m_taskBag.insert(std::make_pair(taskType, eventArg));
	}
	void CancelTask(TaskType taskType) override {
		m_taskBag.erase(taskType);
	}
	void StartEventProcessing() {
		while (!m_taskBag.empty()) {
			auto processingEvent = *m_taskBag.begin();
			m_taskBag.erase(m_taskBag.begin());

			// TODO: if there are lots of handler, here need to optimize.
			for (const auto& processor : m_taskProcessor) {
				processor->ProcessTask(processingEvent.first, processingEvent.second, this);
			}
		}
	}

	// IEditContext - Setting states
	void SetInjectText(IPhrase* phrase) override {
		// TODO: previously determined primitive
		if (!m_insertText) {
			m_insertText = phrase->Display();
		}
		else {
			m_insertText = RefString(m_insertText.u16str() + phrase->Display().u16ptr());
		}
	}
	void SetCompositionText(const std::shared_ptr<IPhrase>& compositionText, int caret, int caretLen) override {
		m_compositionText = compositionText;
		m_caretInComposition = caret;
		(void)caretLen;
	}
	void SetCandidates(const std::shared_ptr<IPhraseList>& candidateList) override {
		m_candidateList = candidateList;
	}
	void SetPassthroughKey(int osKey) override {
		m_passThrough = osKey;
	}
	void SetKeyboardState(ToKeyboard toKeyboard) override {
		m_toKeyboard = toKeyboard;
	}

	// IEditContext - Getting status
	std::shared_ptr<IEditLine> GetEditLine() override { return m_editLine; }
	std::shared_ptr<IKeyCodeHelper> GetKeyCodeHelper() override { return m_keyCodeHelper; }
	std::shared_ptr<IKeyEventProcessor> GetKeyEventProcessor() override { return m_keyEventProcessor; }
	std::shared_ptr<ILiteralConvert> GetLiteralConvert() override { return m_literalConvert; }
	std::shared_ptr<ITransliterationAdapter> GetTransliterationAdapter() override { return m_transliterationAdaptor; }
	std::shared_ptr<IPhraseList> GetDisplayedCandidateList() override { return m_candidateList; }
	virtual RefString GetContextString() override { return m_contextString; }
	bool IsSelectionRange() override { return false; }
	bool IsNoComposition() override { return m_editLine->IsEmpty(); }
	bool IsCharacterCaret() override { return !m_editLine->IsEmpty(); }
	bool IsClauseCaret() override { return false; }
	bool IsCandiateCaret() override { return false; }

	// private
	void ResetVolatileState()
	{
		m_passThrough = -1;
		m_insertText = RefString();
		m_toKeyboard = ToKeyboard::None;
	}

	virtual ~InputModel() {}
	IOBJECT_COMMON_METHODS
};

// small helper task processor to update composition
std::shared_ptr<ITaskProcessor> CreateCompositionAdaptor()
{
	struct CompositionAdaptor :
		public std::enable_shared_from_this<CompositionAdaptor>,
		public ITaskProcessor,
		public IObject
	{
		void ProcessTask(TaskType taskType, const std::shared_ptr<BaseEvent>&, IEditContext* editContext) override
		{
			switch (taskType) {
			case TaskType::Preprocess:
				// force reset dirty flag
				editContext->GetEditLine()->GetAndResetDirtyFlag();
				break;

			case TaskType::Postprocess:
			{
				auto editLine = editContext->GetEditLine();
				if (editLine->GetAndResetDirtyFlag()) {
					auto phrase = editLine->GetSegmentedText();
					int caret, caretLen;
					std::tie(caret, caretLen) = editLine->GetCaret();
					editContext->SetCompositionText(phrase, caret, caretLen);
				}
			}
				break;
			}
		}

		virtual ~CompositionAdaptor() {}
		IOBJECT_COMMON_METHODS
	};
	return std::make_shared<CompositionAdaptor>();
}


struct JsonInputModel :
	public std::enable_shared_from_this<JsonInputModel>,
	public IJsonInputModel,
	public IObject
{
	std::shared_ptr<IInputModel> m_inputModel;
	int m_passThoughKeyCode;

	JsonInputModel()
	{
		m_inputModel = FACTORYCREATE(InputModel);
	}

    std::shared_ptr<IInputModel> GetRawInputModel() noexcept override
    {
        return m_inputModel;
    }
    
	void JsonCommand(const char* json) noexcept override try
	{
		std::string error;
		auto jsonData = json11::Json::parse(json, error);
		auto commandText = jsonData["command"].string_value();

		BaseEvent::EventType eventType = BaseEvent::EventType::Unknown;
		if (commandText == "keydown") {
			eventType = BaseEvent::EventType::KeyDown;
		}
		else if (commandText == "keyup") {
			eventType = BaseEvent::EventType::KeyUp;
		}
		else if (commandText == "CandChosen") {
			eventType = BaseEvent::EventType::CandidateChosen;
		}

		std::shared_ptr<BaseEvent> baseEvent;

		if (eventType == BaseEvent::EventType::KeyDown || eventType == BaseEvent::EventType::KeyUp) {
			auto keyEvent = KeyEvent::Create(eventType);
			keyEvent->label = RefString(to_utf16(jsonData["label"].string_value()));
			keyEvent->keyId = RefString(to_utf16(jsonData["keyId"].string_value()));
			keyEvent->sipKeyCode = jsonData["keyCode"].int_value();
			keyEvent->osKeyCode = jsonData["osKey"].int_value();
			baseEvent = std::static_pointer_cast<BaseEvent>(keyEvent);
		}
		else if (eventType == BaseEvent::EventType::CandidateChosen) {
			auto candEvent = CandidateEvent::Create(eventType);
			candEvent->candidateIndex = jsonData["candIdx"].int_value();
			baseEvent = std::static_pointer_cast<BaseEvent>(candEvent);
		}
		else {
			auto nullEvent = NullEvent::Create(eventType);
			baseEvent = std::static_pointer_cast<BaseEvent>(nullEvent);
		}
		m_inputModel->EventHandler(baseEvent);
	}
	catch (...) { }

	std::string CompositionState() noexcept override try
	{
		auto phrase = m_inputModel->CompositionText();

		std::string compositionText;
		if (phrase != nullptr && phrase->Display().length() > 0) {
			compositionText = phrase->Display().u8str();
		}
		RefString insertingText = m_inputModel->InsertingText();
		std::string commitText = insertingText.u8str();

		int passThoughKey = m_inputModel->GetPassThroughEvent();

		json11::Json json = json11::Json::object {
			{ "composition", json11::Json::array {
				json11::Json::object{
					{ "display", compositionText }
				}
			} },
			{ "caret", m_inputModel->CarretPosition() },
			{ "commit", commitText },
			{ "through", passThoughKey }
		};

		return json.dump();
	}
	catch (...) { return std::string(); }

	std::string CandidateState() noexcept override try
	{
		std::shared_ptr<IPhraseList> phraseList = m_inputModel->CandidateList();
		json11::Json::array candidateList;

		if (phraseList) {
			int phraseCount = phraseList->PhraseCount();
			for (int idx = 0; idx < phraseCount; ++idx) {
				std::shared_ptr<IPhrase> phrase = phraseList->Phrase(idx);
				candidateList.emplace_back(json11::Json(phrase->Display().u8str()));
			}
		}
		json11::Json json = json11::Json::object {
			{ "candidates", candidateList }
		};
		return json.dump();
	}
	catch (...) { return std::string(); }

	std::string KeyboardState() noexcept override {
		const char* keyboardState = (m_inputModel->KeyboardState() == ToKeyboard::Quit) ? "quit" : "none";
		json11::Json json = json11::Json::object{
			{ "keyboard", keyboardState }
		};
		return json.dump();
	}

	bool IsInitialLaunch(bool doUpdate) noexcept override
	{
		return m_inputModel->IsInitialLaunch(doUpdate);
	}

	virtual ~JsonInputModel() {}
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(InputModel);
FACTORYDEFINE(JsonInputModel);
} // Ribbon
