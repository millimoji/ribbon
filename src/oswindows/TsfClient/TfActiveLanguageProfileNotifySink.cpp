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
#include "CompositionProcessorEngine.h"

BOOL CRibbonIME::VerifyRibbonIMECLSID(_In_ REFCLSID clsid)
{
    if (IsEqualCLSID(clsid, Global::RibbonIMECLSID))
    {
        return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// ITfActiveLanguageProfileNotifySink::OnActivated
//
// Sink called by the framework when changes activate language profile.
//----------------------------------------------------------------------------

STDAPI CRibbonIME::OnActivated(_In_ REFCLSID clsid, _In_ REFGUID, _In_ BOOL isActivated)
{
    if (FALSE == VerifyRibbonIMECLSID(clsid))
    {
        return S_OK;
    }

    if (isActivated)
    {
        _AddTextProcessorEngine();
    }

    if (nullptr == _pCompositionProcessorEngine)
    {
        return S_OK;
    }

    if (isActivated)
    {
        _pCompositionProcessorEngine->ShowAllLanguageBarIcons();

        _pCompositionProcessorEngine->ConversionModeCompartmentUpdated(_pThreadMgr);
    }
    else
    {
        _DeleteCandidateList(FALSE, nullptr);

        _pCompositionProcessorEngine->HideAllLanguageBarIcons();
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InitActiveLanguageProfileNotifySink
//
// Advise a active language profile notify sink.
//----------------------------------------------------------------------------

BOOL CRibbonIME::_InitActiveLanguageProfileNotifySink()
{
    ITfSource* pSource = nullptr;
    BOOL ret = FALSE;

    if (_pThreadMgr->QueryInterface(IID_ITfSource, (void **)&pSource) != S_OK)
    {
        return ret;
    }

    if (pSource->AdviseSink(IID_ITfActiveLanguageProfileNotifySink, (ITfActiveLanguageProfileNotifySink *)this, &_activeLanguageProfileNotifySinkCookie) != S_OK)
    {
        _activeLanguageProfileNotifySinkCookie = TF_INVALID_COOKIE;
        goto Exit;
    }

    ret = TRUE;

Exit:
    pSource->Release();
    return ret;
}

//+---------------------------------------------------------------------------
//
// _UninitActiveLanguageProfileNotifySink
//
// Unadvise a active language profile notify sink.  Assumes we have advised one already.
//----------------------------------------------------------------------------

void CRibbonIME::_UninitActiveLanguageProfileNotifySink()
{
    ITfSource* pSource = nullptr;

    if (_activeLanguageProfileNotifySinkCookie == TF_INVALID_COOKIE)
    {
        return; // never Advised
    }

    if (_pThreadMgr->QueryInterface(IID_ITfSource, (void **)&pSource) == S_OK)
    {
        pSource->UnadviseSink(_activeLanguageProfileNotifySinkCookie);
        pSource->Release();
    }

    _activeLanguageProfileNotifySinkCookie = TF_INVALID_COOKIE;
}
