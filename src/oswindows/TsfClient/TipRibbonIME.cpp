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
#include "TfCompartment.h"
#include "inputmodel/InputModel.h"

//+---------------------------------------------------------------------------
//
// CreateInstance
//
//----------------------------------------------------------------------------

/* static */
HRESULT CRibbonIME::CreateInstance(_In_ IUnknown *pUnkOuter, REFIID riid, _Outptr_ void **ppvObj)
{
    CRibbonIME* pRibbonIME = nullptr;
    HRESULT hr = S_OK;

    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (nullptr != pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    pRibbonIME = new (std::nothrow) CRibbonIME();
    if (pRibbonIME == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    hr = pRibbonIME->QueryInterface(riid, ppvObj);

    pRibbonIME->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CRibbonIME::CRibbonIME()
{
    DllAddRef();

    _pThreadMgr = nullptr;

    _threadMgrEventSinkCookie = TF_INVALID_COOKIE;

    _pTextEditSinkContext = nullptr;
    _textEditSinkCookie = TF_INVALID_COOKIE;

    _activeLanguageProfileNotifySinkCookie = TF_INVALID_COOKIE;

    _dwThreadFocusSinkCookie = TF_INVALID_COOKIE;

    _pComposition = nullptr;

    _pCompositionProcessorEngine = nullptr;

    _candidateMode = CANDIDATE_NONE;
    _pCandidateListUIPresenter = nullptr;
    _isCandidateWithWildcard = FALSE;

    _pDocMgrLastFocused = nullptr;

    _pSIPIMEOnOffCompartment = nullptr;
    _dwSIPIMEOnOffCompartmentSinkCookie = 0;
    _msgWndHandle = nullptr;

    _pContext = nullptr;

    _refCount = 1;

	m_ribbonInputModel = FACTORYCREATENS(Ribbon, InputModel);
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CRibbonIME::~CRibbonIME()
{
    if (_pCandidateListUIPresenter)
    {
        delete _pCandidateListUIPresenter;
        _pCandidateListUIPresenter = nullptr;
    }
    DllRelease();
}

//+---------------------------------------------------------------------------
//
// QueryInterface
//
//----------------------------------------------------------------------------

STDAPI CRibbonIME::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor))
    {
        *ppvObj = (ITfTextInputProcessor *)this;
    }
    else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
    {
        *ppvObj = (ITfTextInputProcessorEx *)this;
    }
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
    {
        *ppvObj = (ITfThreadMgrEventSink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfTextEditSink))
    {
        *ppvObj = (ITfTextEditSink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
    {
        *ppvObj = (ITfKeyEventSink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfActiveLanguageProfileNotifySink))
    {
        *ppvObj = (ITfActiveLanguageProfileNotifySink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfCompositionSink))
    {
        *ppvObj = (ITfKeyEventSink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    {
        *ppvObj = (ITfDisplayAttributeProvider *)this;
    }
    else if (IsEqualIID(riid, IID_ITfThreadFocusSink))
    {
        *ppvObj = (ITfThreadFocusSink *)this;
    }
    else if (IsEqualIID(riid, IID_ITfFunctionProvider))
    {
        *ppvObj = (ITfFunctionProvider *)this;
    }
    else if (IsEqualIID(riid, IID_ITfFunction))
    {
        *ppvObj = (ITfFunction *)this;
    }
    else if (IsEqualIID(riid, IID_ITfFnGetPreferredTouchKeyboardLayout))
    {
        *ppvObj = (ITfFnGetPreferredTouchKeyboardLayout *)this;
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------------------
//
// AddRef
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CRibbonIME::AddRef()
{
    return ++_refCount;
}

//+---------------------------------------------------------------------------
//
// Release
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CRibbonIME::Release()
{
    LONG cr = --_refCount;

    assert(_refCount >= 0);

    if (_refCount == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ITfTextInputProcessorEx::ActivateEx
//
//----------------------------------------------------------------------------

STDAPI CRibbonIME::ActivateEx(ITfThreadMgr *pThreadMgr, TfClientId tfClientId, DWORD dwFlags)
{
    _pThreadMgr = pThreadMgr;
    _pThreadMgr->AddRef();

    _tfClientId = tfClientId;
    _dwActivateFlags = dwFlags;

    if (!_InitThreadMgrEventSink())
    {
        goto ExitError;
    }

    ITfDocumentMgr* pDocMgrFocus = nullptr;
    if (SUCCEEDED(_pThreadMgr->GetFocus(&pDocMgrFocus)) && (pDocMgrFocus != nullptr))
    {
        _InitTextEditSink(pDocMgrFocus);
        pDocMgrFocus->Release();
    }

    if (!_InitKeyEventSink())
    {
        goto ExitError;
    }

    if (!_InitActiveLanguageProfileNotifySink())
    {
        goto ExitError;
    }

    if (!_InitThreadFocusSink())
    {
        goto ExitError;
    }

    if (!_InitDisplayAttributeGuidAtom())
    {
        goto ExitError;
    }

    if (!_InitFunctionProviderSink())
    {
        goto ExitError;
    }

    if (!_AddTextProcessorEngine())
    {
        goto ExitError;
    }

    return S_OK;

ExitError:
    Deactivate();
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// ITfTextInputProcessorEx::Deactivate
//
//----------------------------------------------------------------------------

STDAPI CRibbonIME::Deactivate()
{
    if (_pCompositionProcessorEngine)
    {
        delete _pCompositionProcessorEngine;
        _pCompositionProcessorEngine = nullptr;
    }

    ITfContext* pContext = _pContext;
    if (_pContext)
    {   
        pContext->AddRef();
        _EndComposition(_pContext);
    }

    if (_pCandidateListUIPresenter)
    {
        delete _pCandidateListUIPresenter;
        _pCandidateListUIPresenter = nullptr;

        if (pContext)
        {
            pContext->Release();
        }

        _candidateMode = CANDIDATE_NONE;
        _isCandidateWithWildcard = FALSE;
    }

    _UninitFunctionProviderSink();

    _UninitThreadFocusSink();

    _UninitActiveLanguageProfileNotifySink();

    _UninitKeyEventSink();

    _UninitThreadMgrEventSink();

    CCompartment CompartmentKeyboardOpen(_pThreadMgr, _tfClientId, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE);
    CompartmentKeyboardOpen._ClearCompartment();

    if (_pThreadMgr != nullptr)
    {
        _pThreadMgr->Release();
    }

    _tfClientId = TF_CLIENTID_NULL;

    if (_pDocMgrLastFocused)
    {
        _pDocMgrLastFocused->Release();
		_pDocMgrLastFocused = nullptr;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ITfFunctionProvider::GetType
//
//----------------------------------------------------------------------------
HRESULT CRibbonIME::GetType(__RPC__out GUID *pguid)
{
    HRESULT hr = E_INVALIDARG;
    if (pguid)
    {
        *pguid = Global::RibbonIMECLSID;
        hr = S_OK;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// ITfFunctionProvider::::GetDescription
//
//----------------------------------------------------------------------------
HRESULT CRibbonIME::GetDescription(__RPC__deref_out_opt BSTR *pbstrDesc)
{
    HRESULT hr = E_INVALIDARG;
    if (pbstrDesc != nullptr)
    {
        *pbstrDesc = nullptr;
        hr = E_NOTIMPL;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// ITfFunctionProvider::::GetFunction
//
//----------------------------------------------------------------------------
HRESULT CRibbonIME::GetFunction(__RPC__in REFGUID rguid, __RPC__in REFIID riid, __RPC__deref_out_opt IUnknown **ppunk)
{
    HRESULT hr = E_NOINTERFACE;

    if ((IsEqualGUID(rguid, GUID_NULL)) 
        && (IsEqualGUID(riid, __uuidof(ITfFnSearchCandidateProvider))))
    {
        hr = _pITfFnSearchCandidateProvider->QueryInterface(riid, (void**)ppunk);
    }
    else if (IsEqualGUID(rguid, GUID_NULL))
    {
        hr = QueryInterface(riid, (void **)ppunk);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// ITfFunction::GetDisplayName
//
//----------------------------------------------------------------------------
HRESULT CRibbonIME::GetDisplayName(_Out_ BSTR *pbstrDisplayName)
{
    HRESULT hr = E_INVALIDARG;
    if (pbstrDisplayName != nullptr)
    {
        *pbstrDisplayName = nullptr;
        hr = E_NOTIMPL;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// ITfFnGetPreferredTouchKeyboardLayout::GetLayout
// The tkblayout will be Optimized layout.
//----------------------------------------------------------------------------
HRESULT CRibbonIME::GetLayout(_Out_ TKBLayoutType *ptkblayoutType, _Out_ WORD *pwPreferredLayoutId)
{
    HRESULT hr = E_INVALIDARG;
    if ((ptkblayoutType != nullptr) && (pwPreferredLayoutId != nullptr))
    {
        *ptkblayoutType = TKBLT_OPTIMIZED;
        *pwPreferredLayoutId = TKBL_OPT_SIMPLIFIED_CHINESE_PINYIN;
        hr = S_OK;
    }
    return hr;
}