// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "TipPrivate.h"
#include "TfCandidateString.h"

HRESULT CTfCandidateString::CreateInstance(_Outptr_ CTfCandidateString **ppobj)
{  
    if (ppobj == nullptr)
    {
        return E_INVALIDARG;
    }
    *ppobj = nullptr;

    *ppobj = new (std::nothrow) CTfCandidateString();
    if (*ppobj == nullptr) 
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT CTfCandidateString::CreateInstance(REFIID riid, _Outptr_ void **ppvObj)
{ 
    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    *ppvObj = new (std::nothrow) CTfCandidateString();
    if (*ppvObj == nullptr) 
    {
        return E_OUTOFMEMORY;
    }

    return ((CTfCandidateString*)(*ppvObj))->QueryInterface(riid, ppvObj);
}

CTfCandidateString::CTfCandidateString(void)
{
    _refCount = 0;
    _index = 0;
}

CTfCandidateString::~CTfCandidateString()
{
}

// IUnknown methods
STDMETHODIMP CTfCandidateString::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
    if (ppvObj == nullptr)
    {
        return E_POINTER;
    }
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (CTfCandidateString*)this;
    }
    else if (IsEqualIID(riid, IID_ITfCandidateString))
    {
        *ppvObj = (CTfCandidateString*)this;
    }

    if (*ppvObj == nullptr)
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CTfCandidateString::AddRef(void)
{
    return (ULONG)InterlockedIncrement((LONG*)&_refCount);
}

STDMETHODIMP_(ULONG) CTfCandidateString::Release(void)
{
    ULONG refT = (ULONG)InterlockedDecrement((LONG*)&_refCount);
    if (0 < refT)
    {
        return refT;
    }

    delete this;

    return 0;
}

// ITfCandidateString methods
STDMETHODIMP CTfCandidateString::GetString(BSTR *pbstr)
{
    *pbstr = SysAllocString(_candidateStr.c_str());
    return S_OK;
}

STDMETHODIMP CTfCandidateString::GetIndex(_Out_ ULONG *pnIndex)
{
    if (pnIndex == nullptr)
    {
        return E_POINTER;
    }

    *pnIndex = _index;
    return S_OK;
}

STDMETHODIMP CTfCandidateString::SetIndex(ULONG uIndex)
{
    _index = uIndex;
    return S_OK;
}

STDMETHODIMP CTfCandidateString::SetString(_In_ const WCHAR *pch, DWORD_PTR length)
{
    _candidateStr.assign(pch, 0, length);
    return S_OK;
}
