#pragma once
#ifndef _RIBBON_IEDITCONTEXT_H_
#define _RIBBON_IEDITCONTEXT_H_
namespace Ribbon {

struct BaseEvent;
struct KeyEvent;
struct CandidateEvent;
struct IEditLine;
struct IKeyCodeHelper;
struct IKeyEventProcessor;
struct ILiteralConvert;
struct ISymbolEmojiList;
struct ITransliterationAdapter;
enum class ToKeyboard;

enum class TaskType
{
	Preprocess = 1,
	KillFocus,
	KeyDown,
	KeyUp,
	IMEOffInput,
	CandidateUI,
	UpdateCandidate,
	CloseCandidate,
	Postprocess,
	Max,
};

struct IEditContext
{
	// Task system
	virtual void QueueTask(TaskType taskType, const std::shared_ptr<BaseEvent>& eventArg = std::shared_ptr<BaseEvent>()) = 0;
	virtual void CancelTask(TaskType taskType) = 0;

	// Setting states
	virtual void SetInjectText(IPhrase*) = 0;
	virtual void SetCompositionText(const std::shared_ptr<IPhrase>& compositionText, int caret, int caretLength) = 0;
	virtual void SetCandidates(const std::shared_ptr<IPhraseList>& candidateList) = 0;
	virtual void SetPassthroughKey(int osKey) = 0;
	virtual void SetKeyboardState(ToKeyboard toKeyboard) = 0;

	// Getting states
	virtual std::shared_ptr<IEditLine> GetEditLine() = 0;
	virtual std::shared_ptr<IKeyCodeHelper> GetKeyCodeHelper() = 0;
	virtual std::shared_ptr<IKeyEventProcessor> GetKeyEventProcessor() = 0;
	virtual std::shared_ptr<ILiteralConvert> GetLiteralConvert() = 0;
	virtual std::shared_ptr<ITransliterationAdapter> GetTransliterationAdapter() = 0;
	virtual std::shared_ptr<IPhraseList> GetDisplayedCandidateList() = 0;
	virtual RefString GetContextString() = 0;

	virtual bool IsSelectionRange() = 0;
	virtual bool IsNoComposition() = 0;
	virtual bool IsCharacterCaret() = 0;
	virtual bool IsClauseCaret() = 0;
	virtual bool IsCandiateCaret() = 0;

	// Utility function
	void SetInjectText(const char16_t* text) {
		RefString refText(text);
		auto primitive = FACTORYCREATE(Primitive);
		primitive->Display(text);
		primitive->Reading(text);
		auto phrase = FACTORYCREATE(Phrase);
		phrase->Push(primitive);
		SetInjectText(phrase.get());
	}
};

struct ITaskProcessor
{
	virtual void ProcessTask(TaskType, const std::shared_ptr<BaseEvent>&, IEditContext*) = 0;
};

} // Ribbon
#endif // _RIBBON_IEDITCONTEXT_H_
