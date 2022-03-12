// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "TipPrivate.h"
#include "TfCandidateList.h"
#include "TfEnumTfCandidates.h"
#include "TfCandidateString.h"

HRESULT CTfCandidateList::CreateInstance(_Outptr_ ITfCandidateList **ppobj, size_t candStrReserveSize)
{  
    if (ppobj == nullptr)
    {
        return E_INVALIDARG;
    }
    *ppobj = nullptr;

    *ppobj = new (std::nothrow) CTfCandidateList(candStrReserveSize);
    if (*ppobj == nullptr) 
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT CTfCandidateList::CreateInstance(REFIID riid, _Outptr_ void **ppvObj, size_t candStrReserveSize)
{  
    if (ppvObj == nullptr)
    {
        return E_INVALIDARG;
    }
    *ppvObj = nullptr;

    *ppvObj = new (std::nothrow) CTfCandidateList(candStrReserveSize);
    if (*ppvObj == nullptr) 
    {
        return E_OUTOFMEMORY;
    }

    return ((CTfCandidateList*)(*ppvObj))->QueryInterface(riid, ppvObj);
}

CTfCandidateList::CTfCandidateList(size_t candStrReserveSize)
{
    _refCount = 0;

    if (0 < candStrReserveSize)
    {
        _tfCandStrList.reserve(candStrReserveSize);
    }
}

CTfCandidateList::~CTfCandidateList()
{
}

STDMETHODIMP CTfCandidateList::QueryInterface(REFIID riid, _Outptr_ void **ppvObj)
{
    if (ppvObj == nullptr)
    {
        return E_POINTER;
    }
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (ITfCandidateList*)this;
    }
    else if (IsEqualIID(riid, IID_ITfCandidateList))
    {
        *ppvObj = (ITfCandidateList*)this;
    }

    if (*ppvObj == nullptr)
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CTfCandidateList::AddRef()
{
    return (ULONG)InterlockedIncrement((LONG*)&_refCount);
}

STDMETHODIMP_(ULONG) CTfCandidateList::Release()
{
    ULONG  cRefT  = (ULONG)InterlockedDecrement((LONG*)&_refCount);
    if (0 < cRefT)
    {
        return cRefT;
    }

    delete this;

    return 0;
}

STDMETHODIMP CTfCandidateList::EnumCandidates(_Outptr_ IEnumTfCandidates **ppEnum)
{
    return CEnumTfCandidates::CreateInstance(IID_IEnumTfCandidates, (void**)ppEnum, _tfCandStrList);
}

STDMETHODIMP CTfCandidateList::GetCandidate(ULONG nIndex, _Outptr_result_maybenull_ ITfCandidateString **ppCandStr)
{
    if (ppCandStr == nullptr)
    {
        return E_POINTER;
    }
    *ppCandStr = nullptr;

    ULONG sizeCandStr = (ULONG)_tfCandStrList.Count();
    if (sizeCandStr <= nIndex)
    {
        return E_FAIL;
    }

    for (UINT i = 0; i < _tfCandStrList.Count(); i++)
    {
        ITfCandidateString** ppCandStrCur = _tfCandStrList.GetAt(i);
        ULONG indexCur = 0;
        if ((nullptr != ppCandStrCur) && (SUCCEEDED((*ppCandStrCur)->GetIndex(&indexCur))))
        {
            if (nIndex == indexCur)
            {
                BSTR bstr;
                CTfCandidateString* pTipCandidateStrCur = (CTfCandidateString*)(*ppCandStrCur);
                pTipCandidateStrCur->GetString(&bstr);

                CTfCandidateString::CreateInstance(IID_ITfCandidateString, (void**)ppCandStr);

                if (nullptr != (*ppCandStr))
                {
                    CTfCandidateString* pTipCandidateStr = (CTfCandidateString*)(*ppCandStr);
                    pTipCandidateStr->SetString((LPCWSTR)bstr, SysStringLen(bstr));
                }

                SysFreeString(bstr);

                break;
            }
        }
    }
    return S_OK;
}

STDMETHODIMP CTfCandidateList::GetCandidateNum(_Out_ ULONG *pnCnt)
{
    if (pnCnt == nullptr)
    {
        return E_POINTER;
    }

    *pnCnt = (ULONG)(_tfCandStrList.Count());
    return S_OK;
}

STDMETHODIMP CTfCandidateList::SetResult(ULONG /*nIndex*/, TfCandidateResult /*imcr*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTfCandidateList::SetCandidate(_In_ ITfCandidateString **ppCandStr)
{
    if (ppCandStr == nullptr)
    {
        return E_POINTER;
    }

    ITfCandidateString** ppCandLast = _tfCandStrList.Append();
    if (ppCandLast)
    {
        *ppCandLast = *ppCandStr;
    }

    return S_OK;
}
