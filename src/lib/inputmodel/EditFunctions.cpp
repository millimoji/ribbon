#include "pch.h"
#include "EditFunctions.h"
#include "IEditContext.h"
#include "EditLine.h"
#include "InputModel.h"
#include "KeyEventProcessor.h"
#include "SymEmojiList.h"
#include "TransliterationAdapter.h"

namespace Ribbon {

bool EditFunction::FallbackHandler(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent) {
	bool isHandled = UnknownKeyHandler(editContext, keyEvent);
	return editContext->IsNoComposition() ? isHandled : true;
}
bool EditFunction::UnknownKeyHandler(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent) {
	char16_t textBuf[4];
	const char16_t* textPtr = textBuf;

	auto keyCodeHelper = editContext->GetKeyCodeHelper();
	int unicodeTextForKey = keyCodeHelper->UnicdoeFromKey(keyEvent->osKeyCode, keyEvent->modifierState);
	textBuf[0] = static_cast<char16_t>(std::max(unicodeTextForKey, 0));
	textBuf[1] = 0;

	if (keyEvent->sipKeyCode == SIPKEY_ANYTEXT || keyEvent->sipKeyCode == SIPKEY_COMPOSITIONTEXT) {
		textPtr = keyEvent->label.u16ptr();
	}

	uint32_t textRangeType = 0;

	if (textPtr[0]) {
		if ((keyEvent->modifierState & IME_ON) != 0 || keyEvent->sipKeyCode == SIPKEY_COMPOSITIONTEXT) {
			textRangeType = TextRangeType_RawInput | TextRangeType_PhoneticSource;
		}
		else if (!editContext->IsNoComposition()) {
			textRangeType = TextRangeType_RawInput;
		}
	}
	if (textRangeType == 0) {
		AllDetermine(editContext, keyEvent);
		if (!textPtr[0] || keyEvent->isHardwareKey) { // TODO: should be controlled by config.txt?
			editContext->SetPassthroughKey(keyEvent->osKeyCode);
		}
		else {
			editContext->SetInjectText(textPtr);
		}
		editContext->QueueTask(TaskType::IMEOffInput, keyEvent);
		return true;
	}

	auto editLine = editContext->GetEditLine();
	editLine->UpdateText(RefString(textPtr), textRangeType);
	editContext->GetLiteralConvert()->ConvertStream(editLine->GetLiteralProvider().get());
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::Jpn12keyKanaInput(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent) {
	auto editLine = editContext->GetEditLine();
	auto jaConvert = editContext->GetLiteralConvert()->GetJaLiteralConvert();
	jaConvert->Jpn12keyKanaInput(editLine.get(), keyEvent);
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::Jpn12keyModifierInput(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent) {
	auto editLine = editContext->GetEditLine();
	auto jaConvert = editContext->GetLiteralConvert()->GetJaLiteralConvert();
	jaConvert->Jpn12keyModifierInput(editLine.get(), keyEvent);
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::Jpn12keyBreakersInput(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent) {
	auto editLine = editContext->GetEditLine();
	auto jaConvert = editContext->GetLiteralConvert()->GetJaLiteralConvert();
	jaConvert->Jpn12keyBreakersInput(editLine.get(), keyEvent);
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::Jpn12keyBackToggleInput(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent) {
	auto editLine = editContext->GetEditLine();
	auto jaConvert = editContext->GetLiteralConvert()->GetJaLiteralConvert();
	jaConvert->Jpn12keyBackToggleInput(editLine.get(), keyEvent);
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::AllCancel(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	if (editContext->IsCandiateCaret()) {
		editContext->GetEditLine()->Clear();
		return true;
	}
	return false;
}
bool EditFunction::AllClear(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	editContext->GetEditLine()->Clear();
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::AllDetermine(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	if (editLine->IsEmpty()) {
		return false;
	}
	editContext->SetInjectText(editLine->GetSegmentedText().get());
	editLine->Clear();
	editContext->QueueTask(TaskType::UpdateCandidate);
	return true;
}
bool EditFunction::CanddaiteChangeView(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateBottom(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateCancel(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	editContext->SetCandidates(FACTORYCREATE(PhraseList));
	return true;
}
bool EditFunction::CandidateDetermine(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateNext(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateNextGroup(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateNextPage(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidatePrev(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidatePrevGroup(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidatePrevPage(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateSecond(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateTop(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CandidateViewMove(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CaretBackspace(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	if (editLine->IsEmpty()) {
		return false;
	}
	std::tuple<int, int> caretInfo = editLine->GetCaret();
	if (std::get<1>(caretInfo) > 0) {
		editLine->DeleteText();
	}
	else if (std::get<0>(caretInfo) > 0) {
		editLine->SetCaret(std::get<0>(caretInfo) - 1, 1);
		editLine->DeleteText();
	}
	editContext->QueueTask(TaskType::UpdateCandidate, std::shared_ptr<BaseEvent>());
	return true;
}
bool EditFunction::CaretBottom(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	editLine->SetCaret(-1, 0);
	return true;
}
bool EditFunction::CaretDelete(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	if (editLine->IsEmpty()) {
		return false;
	}
	std::tuple<int, int> caretInfo = editLine->GetCaret();
	if (std::get<1>(caretInfo) > 0) {
		editLine->DeleteText();
	}
	else {
		editLine->SetCaret(std::get<0>(caretInfo), 1);
		editLine->DeleteText();
	}
	return true;
}
bool EditFunction::CaretDetermineChar(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CaretDetermineOff(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::CaretInsertFullSpace(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	editContext->SetInjectText(u"\u3000");
	return true;
}
bool EditFunction::CaretInsertHalfSpace(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	editContext->SetInjectText(u" ");
	return true;
}
bool EditFunction::CaretInsertSpace(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	editContext->SetInjectText(u" ");
	return true;
}
bool EditFunction::CaretInsertSpaceR(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	editContext->SetInjectText(u"\u3000");
	return true;
}
bool EditFunction::CaretLeft(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	if (editLine->IsEmpty()) {
		return false;
	}
	std::tuple<int, int> caretInfo = editLine->GetCaret();

	if (std::get<0>(caretInfo) > 0) {
		editLine->SetCaret(std::get<0>(caretInfo) - 1, 0);
	}
	return true;
}
bool EditFunction::CaretRight(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	if (editLine->IsEmpty()) {
		return false;
	}
	std::tuple<int, int> caretInfo = editLine->GetCaret();
	int visualLength = editLine->GetVisualLength();

	if (std::get<0>(caretInfo) < visualLength) {
		editLine->SetCaret(std::get<0>(caretInfo) + 1, 0);
	}
	return true;
}
bool EditFunction::CaretTop(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto editLine = editContext->GetEditLine();
	editLine->SetCaret(0, 0);
	return true;
}
bool EditFunction::ClauseBottom(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseCancel(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseConversion(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseDetermineCurrent(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseDetermineTop(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseExtend(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseLeft(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseOpenRead(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseRight(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseShorten(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ClauseTop(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ExternalCodeInput(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ExternalPropertyMenu(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ExternalSearchBy(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ExternalSearchDefault(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ExternalWordDialog(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ModeAlphabetToggle(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ModeHiragana(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ModeKanaLock(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ModeKanaToggle(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ModeKatakana(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ModeOnOff(IEditContext* editContext, const std::shared_ptr<KeyEvent>&) {
	auto keyEventProcessor = editContext->GetKeyEventProcessor();
	auto modifierState = keyEventProcessor->ModifierState();
	modifierState ^= IME_ON;
	keyEventProcessor->ModifierState(modifierState);
	return true;
}
bool EditFunction::ModeRomanToggle(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionAlphaFull(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionAlphaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionDefault(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionHiragana(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionKanaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionKatakana(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReconversionUndo(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::RepalceAlphaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::RepalceKanaToggle(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceAlphaFull(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceAlphaToggle(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceConversion(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceHalfWidth(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceHiragana(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceKanaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceKatakana(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }
bool EditFunction::ReplaceWidthToggle(IEditContext*, const std::shared_ptr<KeyEvent>&) { return true; }

bool EditFunction::ChangeSIPLayout(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent)
{
	SymbolEmojiCategory category = SymbolEmojiCategory::Max;
	if (keyEvent->keyId == u"symbolview") {
		category = SymbolEmojiCategory::SymStandard;
	}
	else if (keyEvent->keyId == u"emojiview") {
		category = SymbolEmojiCategory::Human;
	}
	else if (keyEvent->keyId == u"sw-quit") {
		editContext->SetKeyboardState(ToKeyboard::Quit);
	}
	if (category != SymbolEmojiCategory::Max) {
		auto editLine = editContext->GetEditLine();
		if (!editLine->IsEmpty()) {
			editContext->SetInjectText(editLine->GetSegmentedText().get());
			editLine->Clear();
		}
		auto candEvent = CandidateEvent::Create(BaseEvent::EventType::ShowSymbolEmoji);
		candEvent->symEmojiCategory = static_cast<int>(category);
		editContext->QueueTask(TaskType::CandidateUI, candEvent);
	}
	else {
		editContext->QueueTask(TaskType::CloseCandidate);
	}
	return true;
}

bool EditFunction::ChangeCategory(IEditContext* editContext, const std::shared_ptr<KeyEvent>& keyEvent)
{
	constexpr SymbolEmojiCategory emojiArray[]{
		SymbolEmojiCategory::Human,
		SymbolEmojiCategory::Animal,
		SymbolEmojiCategory::Food,
		SymbolEmojiCategory::TimeRegion,
		SymbolEmojiCategory::Game,
		SymbolEmojiCategory::Items,
		SymbolEmojiCategory::SymbolMarks,
		SymbolEmojiCategory::Flags,
	};
	constexpr SymbolEmojiCategory symbolArray[]{
		SymbolEmojiCategory::SymStandard,
		SymbolEmojiCategory::SymParenthes,
		SymbolEmojiCategory::SymArrows,
		SymbolEmojiCategory::SymMath,
		SymbolEmojiCategory::Numbers,
		SymbolEmojiCategory::Images,
		SymbolEmojiCategory::FullWidth,
	};

	SymbolEmojiCategory category = SymbolEmojiCategory::Human;
	const auto& keyId = keyEvent->keyId.u16ptr();
	if (textstartwith(keyId, u"em-")) {
		if (keyId[3] >= u'0' && keyId[3] <= u'6') {
			category = emojiArray[keyId[3] - u'0'];
		}
	}
	else if (textstartwith(keyId, u"sm-")) {
		if (keyId[3] >= u'0' && keyId[3] <= u'6') {
			category = symbolArray[keyId[3] - u'0'];
		}
	}

	auto candEvent = CandidateEvent::Create(BaseEvent::EventType::ShowSymbolEmoji);
	candEvent->symEmojiCategory = static_cast<int>(category);
	editContext->QueueTask(TaskType::CandidateUI, candEvent);
	return true;
}

} // Ribbon

