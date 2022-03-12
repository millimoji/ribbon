#pragma once
#ifndef _RIBBON_EDITFUNCTIONS_H_
#define _RIBBON_EDITFUNCTIONS_H_
namespace Ribbon {

// forward refernce
struct IEditContext;
struct KeyEvent;
struct CandidateEvent;

struct EditFunction
{
	static bool FallbackHandler(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool UnknownKeyHandler(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool AllCancel(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool AllClear(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool AllDetermine(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CanddaiteChangeView(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateBottom(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateCancel(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateDetermine(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateNext(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateNextGroup(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateNextPage(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidatePrev(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidatePrevGroup(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidatePrevPage(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateSecond(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateTop(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CandidateViewMove(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretBackspace(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretBottom(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretDelete(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretDetermineChar(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretDetermineOff(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretInsertFullSpace(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretInsertHalfSpace(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretInsertSpace(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretInsertSpaceR(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretLeft(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretRight(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool CaretTop(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseBottom(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseCancel(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseConversion(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseDetermineCurrent(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseDetermineTop(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseExtend(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseLeft(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseOpenRead(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseRight(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseShorten(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ClauseTop(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ExternalCodeInput(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ExternalPropertyMenu(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ExternalSearchBy(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ExternalSearchDefault(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ExternalWordDialog(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeAlphabetToggle(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeHiragana(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeKanaLock(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeKanaToggle(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeKatakana(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeOnOff(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ModeRomanToggle(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionAlphaFull(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionAlphaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionDefault(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionHiragana(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionKanaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionKatakana(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReconversionUndo(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool RepalceAlphaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool RepalceKanaToggle(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceAlphaFull(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceAlphaToggle(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceConversion(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceHalfWidth(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceHiragana(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceKanaHalf(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceKatakana(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool ReplaceWidthToggle(IEditContext*, const std::shared_ptr<KeyEvent>&);

	// SIP
	static bool ChangeSIPLayout(IEditContext*, const std::shared_ptr<KeyEvent>&);

	// JPN 12Key
	static bool Jpn12keyKanaInput(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool Jpn12keyModifierInput(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool Jpn12keyBreakersInput(IEditContext*, const std::shared_ptr<KeyEvent>&);
	static bool Jpn12keyBackToggleInput(IEditContext*, const std::shared_ptr<KeyEvent>&);

	// EmojiSIP
	static bool ChangeCategory(IEditContext*, const std::shared_ptr<KeyEvent>&);
};

} // Ribbon
#endif // _RIBBON_EDITFUNCTIONS_H_
