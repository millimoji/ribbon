// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "TipPrivate.h"
#include "TipGlobals.h"
#include "TipRibbonIME.h"
#include "TfCandidateListUIPresenter.h"
#include "CompositionProcessorEngine.h"
#include "TfKeyHandlerEditSession.h"
#include "TfCompartment.h"

#include "inputmodel/InputModel.h"
#include "TfEditSessionRibbon.h"

// 0xF003, 0xF004 are the keys that the touch keyboard sends for next/previous
#define THIRDPARTY_NEXTPAGE  static_cast<WORD>(0xF003)
#define THIRDPARTY_PREVPAGE  static_cast<WORD>(0xF004)


bool IgnoreKeyEvent(WPARAM wParam, LPARAM lParam) {
	if (wParam == VK_SHIFT || wParam == VK_CONTROL || wParam == VK_MENU ||
		wParam == VK_LSHIFT || wParam == VK_LCONTROL || wParam == VK_LMENU ||
		wParam == VK_RSHIFT || wParam == VK_RCONTROL || wParam == VK_RMENU) {

		if (lParam & 0x40000000) {
			return true;
		}
	}
	return false;
}

std::shared_ptr<Ribbon::KeyEvent> CreateKeyEvent(WPARAM wParam, LPARAM lParam, bool isUp) {
	std::shared_ptr<Ribbon::KeyEvent> keyEvent = std::make_shared<Ribbon::KeyEvent>();
	keyEvent->eventType = isUp ? Ribbon::BaseEvent::EventType::KeyUp : Ribbon::BaseEvent::EventType::KeyDown;
	keyEvent->osKeyCode = static_cast<int>(wParam);
	keyEvent->isHardwareKey = true;

	BYTE kb[256];
	GetKeyboardState(kb);

	UINT scanCode = (lParam & 0xFF0000) >> 16;

	wchar_t unicodeBuf[32];
	int cch = ToUnicode((UINT)wParam, scanCode, kb, unicodeBuf, (int)ARRAYSIZE(unicodeBuf), 0);
	if (cch > 0) {
		keyEvent->label = Ribbon::RefString(reinterpret_cast<const char16_t*>(unicodeBuf), cch);
	}
	return keyEvent;
}

// Because the code mostly works with VKeys, here map a WCHAR back to a VKKey for certain
// vkeys that the IME handles specially
__inline UINT VKeyFromVKPacketAndWchar(UINT vk, WCHAR wch)
{
    UINT vkRet = vk;
    if (LOWORD(vk) == VK_PACKET)
    {
        if (wch == L' ')
        {
            vkRet = VK_SPACE;
        }
        else if ((wch >= L'0') && (wch <= L'9'))
        {
            vkRet = static_cast<UINT>(wch);
        }
        else if ((wch >= L'a') && (wch <= L'z'))
        {
            vkRet = (UINT)(L'A') + ((UINT)(L'z') - static_cast<UINT>(wch));
        }
        else if ((wch >= L'A') && (wch <= L'Z'))
        {
            vkRet = static_cast<UINT>(wch);
        }
        else if (wch == THIRDPARTY_NEXTPAGE)
        {
            vkRet = VK_NEXT;
        }
        else if (wch == THIRDPARTY_PREVPAGE)
        {
            vkRet = VK_PRIOR;
        }
    }
    return vkRet;
}

//+---------------------------------------------------------------------------
//
// _IsKeyEaten
//
//----------------------------------------------------------------------------

BOOL CRibbonIME::_IsKeyEaten(_In_ ITfContext*, UINT codeIn, _Out_ UINT *pCodeOut, _Out_writes_(1) WCHAR *pwch, _Out_opt_ _KEYSTROKE_STATE *pKeyState)
{
    *pCodeOut = codeIn;

    BOOL isOpen = FALSE;
    CCompartment CompartmentKeyboardOpen(_pThreadMgr, _tfClientId, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE);
    CompartmentKeyboardOpen._GetCompartmentBOOL(isOpen);

    if (pKeyState) {
        pKeyState->Category = CATEGORY_NONE;
        pKeyState->Function = FUNCTION_NONE;
    }
    if (pwch) {
        *pwch = L'\0';
    }

    // if the keyboard is disabled, we don't eat keys.
    if (_IsKeyboardDisabled()) {
        return FALSE;
    }

    //
    // Map virtual key to character code
    //
    BOOL isTouchKeyboardSpecialKeys = FALSE;
    WCHAR wch = ConvertVKey(codeIn);
    *pCodeOut = VKeyFromVKPacketAndWchar(codeIn, wch);
    if ((wch == THIRDPARTY_NEXTPAGE) || (wch == THIRDPARTY_PREVPAGE)) {
        // We always eat the above softkeyboard special keys
        isTouchKeyboardSpecialKeys = TRUE;
        if (pwch) {
            *pwch = wch;
        }
    }

    // if the keyboard is closed, we don't eat keys, with the exception of the touch keyboard specials keys
    if (!isOpen) {
        return isTouchKeyboardSpecialKeys;
    }
    if (pwch) {
        *pwch = wch;
    }

    //
    // Get composition engine
    //
    CCompositionProcessorEngine *pCompositionProcessorEngine;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;

    if (isOpen) {
        //
        // The candidate or phrase list handles the keys through ITfKeyEventSink.
        //
        // eat only keys that CKeyHandlerEditSession can handles.
        //
        if (pCompositionProcessorEngine->IsVirtualKeyNeed(*pCodeOut, pwch, _IsComposing(), _candidateMode, _isCandidateWithWildcard, pKeyState)) {
            return TRUE;
        }
    }

    return isTouchKeyboardSpecialKeys;
}

//+---------------------------------------------------------------------------
//
// ConvertVKey
//
//----------------------------------------------------------------------------

WCHAR CRibbonIME::ConvertVKey(UINT code)
{
    //
    // Map virtual key to scan code
    //
    UINT scanCode = 0;
    scanCode = MapVirtualKey(code, 0);

    //
    // Keyboard state
    //
    BYTE abKbdState[256] = {'\0'};
    if (!GetKeyboardState(abKbdState))
    {
        return 0;
    }

    //
    // Map virtual key to character code
    //
    WCHAR wch = '\0';
    if (ToUnicode(code, scanCode, abKbdState, &wch, 1, 0) == 1)
    {
        return wch;
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
// _IsKeyboardDisabled
//
//----------------------------------------------------------------------------

BOOL CRibbonIME::_IsKeyboardDisabled()
{
    ITfDocumentMgr* pDocMgrFocus = nullptr;
    ITfContext* pContext = nullptr;
    BOOL isDisabled = FALSE;

    if ((_pThreadMgr->GetFocus(&pDocMgrFocus) != S_OK) ||
        (pDocMgrFocus == nullptr))
    {
        // if there is no focus document manager object, the keyboard 
        // is disabled.
        isDisabled = TRUE;
    }
    else if ((pDocMgrFocus->GetTop(&pContext) != S_OK) ||
        (pContext == nullptr))
    {
        // if there is no context object, the keyboard is disabled.
        isDisabled = TRUE;
    }
    else
    {
        CCompartment CompartmentKeyboardDisabled(_pThreadMgr, _tfClientId, GUID_COMPARTMENT_KEYBOARD_DISABLED);
        CompartmentKeyboardDisabled._GetCompartmentBOOL(isDisabled);

        CCompartment CompartmentEmptyContext(_pThreadMgr, _tfClientId, GUID_COMPARTMENT_EMPTYCONTEXT);
        CompartmentEmptyContext._GetCompartmentBOOL(isDisabled);
    }

    if (pContext)
    {
        pContext->Release();
    }

    if (pDocMgrFocus)
    {
        pDocMgrFocus->Release();
    }

    return isDisabled;
}

//+---------------------------------------------------------------------------
//
// ITfKeyEventSink::OnSetFocus
//
// Called by the system whenever this service gets the keystroke device focus.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnSetFocus(BOOL)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfKeyEventSink::OnTestKeyDown
//
// Called by the system to query this service wants a potential keystroke.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnTestKeyDown(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
#ifdef USE_RIBBON_ENGINE
	(void)pContext;
	m_isKeyTested = true;

	if (IgnoreKeyEvent(wParam, lParam)) {
		*pIsEaten = TRUE;
		return S_OK;
	}

	*pIsEaten = FALSE;
	Global::UpdateModifiers(wParam, lParam);

	std::wstring contextString;
	if (SUCCEEDED(GetTextContext(pContext, _tfClientId, contextString))) {
		m_ribbonInputModel->UpdateContext(reinterpret_cast<const char16_t*>(contextString.c_str()));
	}

	std::shared_ptr<Ribbon::KeyEvent> keyEvent = CreateKeyEvent(wParam, lParam, false);

	m_ribbonInputModel->EventHandler(keyEvent);

	if (m_ribbonInputModel->GetPassThroughEvent() < 0) {
		*pIsEaten = TRUE;
	}

#else
    Global::UpdateModifiers(wParam, lParam);

    _KEYSTROKE_STATE KeystrokeState;
    WCHAR wch = '\0';
    UINT code = 0;
    *pIsEaten = _IsKeyEaten(pContext, (UINT)wParam, &code, &wch, &KeystrokeState);

    if (KeystrokeState.Category == CATEGORY_INVOKE_COMPOSITION_EDIT_SESSION)
    {
        //
        // Invoke key handler edit session
        //
        KeystrokeState.Category = CATEGORY_COMPOSING;

        _InvokeKeyHandler(pContext, code, wch, (DWORD)lParam, KeystrokeState);
    }
#endif
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfKeyEventSink::OnKeyDown
//
// Called by the system to offer this service a keystroke.  If *pIsEaten == TRUE
// on exit, the application will not handle the keystroke.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
#ifdef USE_RIBBON_ENGINE
	if (IgnoreKeyEvent(wParam, lParam)) {
		*pIsEaten = TRUE;
		return S_OK;
	}

	if (!m_isKeyTested) {
		*pIsEaten = FALSE;
		Global::UpdateModifiers(wParam, lParam);

		std::wstring contextString;
		if (SUCCEEDED(GetTextContext(pContext, _tfClientId, contextString))) {
			m_ribbonInputModel->UpdateContext(reinterpret_cast<const char16_t*>(contextString.c_str()));
		}

		std::shared_ptr<Ribbon::KeyEvent> keyEvent = CreateKeyEvent(wParam, lParam, false);

		m_ribbonInputModel->EventHandler(keyEvent);

		if (m_ribbonInputModel->GetPassThroughEvent() < 0) {
			*pIsEaten = TRUE;
		}
	}
	else {
		*pIsEaten = TRUE;
	}
	if (true /**pIsEaten*/) {
		UpdateEditControl(pContext, _tfClientId, this, m_ribbonInputModel, _gaDisplayAttributeInput);
	}
	m_isKeyTested = false;

	(void)pContext;
	(void)lParam;
#else
    _KEYSTROKE_STATE KeystrokeState;
    WCHAR wch = '\0';
    UINT code = 0;

    *pIsEaten = _IsKeyEaten(pContext, (UINT)wParam, &code, &wch, &KeystrokeState);

    if (*pIsEaten)
    {
        bool needInvokeKeyHandler = true;
        //
        // Invoke key handler edit session
        //
        if (code == VK_ESCAPE)
        {
            KeystrokeState.Category = CATEGORY_COMPOSING;
        }

        // Always eat THIRDPARTY_NEXTPAGE and THIRDPARTY_PREVPAGE keys, but don't always process them.
        if ((wch == THIRDPARTY_NEXTPAGE) || (wch == THIRDPARTY_PREVPAGE))
        {
            needInvokeKeyHandler = !((KeystrokeState.Category == CATEGORY_NONE) && (KeystrokeState.Function == FUNCTION_NONE));
        }

        if (needInvokeKeyHandler)
        {
            _InvokeKeyHandler(pContext, code, wch, (DWORD)lParam, KeystrokeState);
        }
    }
    else if (KeystrokeState.Category == CATEGORY_INVOKE_COMPOSITION_EDIT_SESSION)
    {
        // Invoke key handler edit session
        KeystrokeState.Category = CATEGORY_COMPOSING;
        _InvokeKeyHandler(pContext, code, wch, (DWORD)lParam, KeystrokeState);
    }
#endif
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfKeyEventSink::OnTestKeyUp
//
// Called by the system to query this service wants a potential keystroke.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnTestKeyUp(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
	(void)pContext;
#ifdef USE_RIBBON_ENGINE
	m_isKeyTested = true;

	*pIsEaten = FALSE;
	Global::UpdateModifiers(wParam, lParam);

	std::shared_ptr<Ribbon::KeyEvent> keyEvent = CreateKeyEvent(wParam, lParam, true);

	m_ribbonInputModel->EventHandler(keyEvent);

	if (m_ribbonInputModel->GetPassThroughEvent() < 0) {
		*pIsEaten = TRUE;
	}
	(void)lParam;
#else
    if (pIsEaten == nullptr)
    {
        return E_INVALIDARG;
    }

    Global::UpdateModifiers(wParam, lParam);

    WCHAR wch = '\0';
    UINT code = 0;

    *pIsEaten = _IsKeyEaten(pContext, (UINT)wParam, &code, &wch, NULL);
#endif
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfKeyEventSink::OnKeyUp
//
// Called by the system to offer this service a keystroke.  If *pIsEaten == TRUE
// on exit, the application will not handle the keystroke.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
	(void)pContext; (void)lParam;
#ifdef USE_RIBBON_ENGINE
	if (!m_isKeyTested) {
		*pIsEaten = FALSE;
		Global::UpdateModifiers(wParam, lParam);

		std::shared_ptr<Ribbon::KeyEvent> keyEvent = CreateKeyEvent(wParam, lParam, true);

		m_ribbonInputModel->EventHandler(keyEvent);

		if (m_ribbonInputModel->GetPassThroughEvent() < 0) {
			*pIsEaten = TRUE;
		}
	}
	else {
		*pIsEaten = TRUE;
	}
	m_isKeyTested = false;
#else
    WCHAR wch = '\0';
    UINT code = 0;

    *pIsEaten = _IsKeyEaten(pContext, (UINT)wParam, &code, &wch, NULL);
#endif
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfKeyEventSink::OnPreservedKey
//
// Called when a hotkey (registered by us, or by the system) is typed.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnPreservedKey(ITfContext*, REFGUID rguid, BOOL *pIsEaten)
{
	*pIsEaten = FALSE;
#ifdef USE_RIBBON_ENGINE
	UNREFERENCED_PARAMETER(rguid);
#else
    CCompositionProcessorEngine *pCompositionProcessorEngine;
    pCompositionProcessorEngine = _pCompositionProcessorEngine;

    pCompositionProcessorEngine->OnPreservedKey(rguid, pIsEaten, _GetThreadMgr(), _GetClientId());
#endif
	return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InitKeyEventSink
//
// Advise a keystroke sink.
//----------------------------------------------------------------------------

BOOL CRibbonIME::_InitKeyEventSink()
{
    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
    HRESULT hr = S_OK;

    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr)))
    {
        return FALSE;
    }

    hr = pKeystrokeMgr->AdviseKeyEventSink(_tfClientId, (ITfKeyEventSink *)this, TRUE);

    pKeystrokeMgr->Release();

    return (hr == S_OK);
}

//+---------------------------------------------------------------------------
//
// _UninitKeyEventSink
//
// Unadvise a keystroke sink.  Assumes we have advised one already.
//----------------------------------------------------------------------------

void CRibbonIME::_UninitKeyEventSink()
{
    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;

    if (FAILED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr)))
    {
        return;
    }

    pKeystrokeMgr->UnadviseKeyEventSink(_tfClientId);

    pKeystrokeMgr->Release();
}
