//////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All rights reserved.
//
//  CRibbonIMERibbonIMEStructureArray.h
//
//          CRibbonIMEStructureArray declaration.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

template<class T>
class CRibbonIMEStructureArray
{
    typedef typename    std::vector<T>         value_type;
    typedef const       T&                     CONST_REF;
    typedef typename    value_type             CRibbonIMEArray;
    typedef typename    value_type::iterator   CRibbonIMEIter;

public:
    CRibbonIMEStructureArray(): _imeVector()
    {
    }

    explicit CRibbonIMEStructureArray(size_t iCount): _imeVector(iCount)
    {
    }

    CRibbonIMEStructureArray(size_t iCount, CONST_REF tVal): _imeVector(iCount, tVal)
    {
    }

    virtual ~CRibbonIMEStructureArray() {}

    inline CONST_REF GetAt(size_t iIndex) const
    {
        assert(iIndex <= _imeVector.size());
        assert(_imeVector.size() > 0);

        return _imeVector[iIndex];
    }

    inline T& GetAt(size_t iIndex)
    {
        assert(iIndex <= _imeVector.size());
        assert(_imeVector.size() > 0);

        return _imeVector[iIndex];
    }

    void RemoveAt(size_t iIndex, size_t iElements)
    {
        assert(iIndex <= _imeVector.size());
        assert(_imeVector.size() > 0);

        CRibbonIMEIter beginIter = _imeVector.begin() + iIndex;
        CRibbonIMEIter lastIter = beginIter + iElements - 1;

        _imeVector.erase(beginIter, lastIter);
    }

    size_t Count() const { return _imeVector.size(); }

    void Append(const T& tVal)
    {
        _imeVector.push_back(tVal);
    }

    void Clear()
    {
        _imeVector.clear();
    }

private:
    CRibbonIMEArray _imeVector;   // the actual array of data
    CRibbonIMEIter  _imeIter;
};
