#pragma once
#ifndef _RIBBON_EDITINGMODEL_H_
#define _RIBBON_EDITINGMODEL_H_

namespace Ribbon {

struct BaseEvent {
	enum class EventType {
		Unknown,
		KeyDown,
		KeyUp,
		ShowSymbolEmoji, // recursive, triggered by key-down
		CandidateChosen,
		KillFocus,
	} eventType;
};

struct NullEvent :
	public std::enable_shared_from_this<NullEvent>,
	public BaseEvent
{
	static std::shared_ptr<NullEvent> Create(EventType _eventType) {
		auto ptr = std::make_shared<NullEvent>();
		ptr->eventType = _eventType;
		return ptr;
	}
};

struct KeyEvent : 
	public std::enable_shared_from_this<KeyEvent>,
	public BaseEvent
{
	RefString label;
	RefString keyId;
	int sipKeyCode = -1;
	int osKeyCode = -1;
	uint32_t modifierState = 0;
	bool isHardwareKey = false;
	bool isRepeatedKey = false;

	KeyEvent() = default;
	~KeyEvent() = default;
	KeyEvent(const KeyEvent&) = delete;
	KeyEvent& operator = (const KeyEvent&) = delete;

	bool IsEqualTo(const std::shared_ptr<KeyEvent>& src) {
		return eventType == src->eventType &&
			label == src->label &&
			keyId == src->keyId &&
			sipKeyCode == src->sipKeyCode &&
			osKeyCode == src->osKeyCode &&
			modifierState == src->modifierState &&
			isHardwareKey == src->isHardwareKey;
	}

	static std::shared_ptr<KeyEvent> Create(EventType _eventType) {
		auto ptr = std::make_shared<KeyEvent>();
		ptr->eventType = _eventType;
		return ptr;
	}

	static std::shared_ptr<KeyEvent> Create(int _sipKeyCode, const char16_t* _text = nullptr, const char16_t* _id = nullptr, uint32_t _modifierState = 0) {
		auto ptr = std::make_shared<KeyEvent>();
		ptr->eventType = EventType::KeyDown;
		ptr->label = RefString(_text);
		ptr->keyId = RefString(_id);
		ptr->sipKeyCode = _sipKeyCode;
		ptr->modifierState = _modifierState;
		return ptr;
	}

	static std::shared_ptr<KeyEvent> Create(const char16_t* _text, uint32_t _modifierState = 0) {
		auto ptr = std::make_shared<KeyEvent>();
		ptr->eventType = EventType::KeyDown;
		ptr->label = RefString(_text);
		ptr->sipKeyCode = SIPKEY_COMPOSITIONTEXT;
		ptr->modifierState = _modifierState;
		return ptr;
	}
};

struct CandidateEvent :
	public std::enable_shared_from_this<CandidateEvent>,
	public BaseEvent
{
	int candidateIndex = -1;
	int symEmojiCategory = -1;

	CandidateEvent() = default;
	~CandidateEvent() = default;
	CandidateEvent(const CandidateEvent&) = delete;
	CandidateEvent& operator = (const CandidateEvent&) = delete;

	static std::shared_ptr<CandidateEvent> Create(EventType _eventType) {
		auto ptr = std::make_shared<CandidateEvent>();
		ptr->eventType = _eventType;
		return ptr;
	}
	static std::shared_ptr<CandidateEvent> Create(EventType _eventType, int candIndex) {
		auto ptr = std::make_shared<CandidateEvent>();
		ptr->eventType = _eventType;
		ptr->candidateIndex = candIndex;
		return ptr;
	}
};

struct SystemEvent :
	public std::enable_shared_from_this<SystemEvent>,
	public BaseEvent
{
	SystemEvent() = default;
	~SystemEvent() = default;
	SystemEvent(const SystemEvent&) = delete;
	SystemEvent& operator = (const SystemEvent&) = delete;

	static std::shared_ptr<SystemEvent> Create(EventType _eventType) {
		auto ptr = std::make_shared<SystemEvent>();
		ptr->eventType = _eventType;
		return ptr;
	}
};

enum class ToKeyboard {
	None,
	Quit,
};

struct IInputModel
{
	virtual void EventHandler(const std::shared_ptr<BaseEvent>& baseEvent) noexcept = 0;
	virtual void UpdateContext(const char16_t* contextString) noexcept = 0;

	// Getter
	virtual std::shared_ptr<IPhrase> CompositionText() noexcept = 0;
	virtual std::shared_ptr<IPhraseList> CandidateList() noexcept = 0;
	virtual RefString InsertingText() noexcept = 0;
	virtual int CarretPosition() noexcept = 0;
	virtual int GetPassThroughEvent() noexcept = 0;
	virtual ToKeyboard KeyboardState() noexcept = 0;
	virtual bool IsInitialLaunch(bool doUpdate = false) noexcept = 0;
};

struct IJsonInputModel
{
    virtual std::shared_ptr<IInputModel> GetRawInputModel() noexcept = 0;
	virtual void JsonCommand(const char* json) noexcept = 0;
	virtual std::string CompositionState() noexcept = 0;
	virtual std::string CandidateState() noexcept = 0;
	virtual std::string KeyboardState() noexcept = 0;
	virtual bool IsInitialLaunch(bool doUpdate = false) noexcept = 0;
};

FACTORYEXTERN(InputModel);
FACTORYEXTERN(JsonInputModel);
} // Ribbon
#endif // _RIBBON_EDITINGMODEL_H_
