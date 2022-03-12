// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "TipPrivate.h"
#include "UIBaseWindow.h"
#include "UIButtonWindow.h"

//+---------------------------------------------------------------------------
//
// CButtonWindow
//
//----------------------------------------------------------------------------

CButtonWindow::CButtonWindow()
{
    typeOfControl = 0;
}

//+---------------------------------------------------------------------------
//
// ~CButtonWindow
//
//----------------------------------------------------------------------------

CButtonWindow::~CButtonWindow()
{
}

//+---------------------------------------------------------------------------
//
// _OnPaint
//
//----------------------------------------------------------------------------

void CButtonWindow::_OnPaint(_In_ HDC, _In_ PAINTSTRUCT*)
{
}

//+---------------------------------------------------------------------------
//
// _OnLButtonDown
//
//----------------------------------------------------------------------------

void CButtonWindow::_OnLButtonDown(POINT)
{
    typeOfControl = DFCS_PUSHED;
    _StartCapture();
}

//+---------------------------------------------------------------------------
//
// _WindowProcCallback
//
//----------------------------------------------------------------------------
LRESULT CALLBACK CButtonWindow::_WindowProcCallback(_In_ HWND, UINT, _In_ WPARAM, _In_ LPARAM) 
{ 
    return 0; 
}

//+---------------------------------------------------------------------------
//
// _OnLButtonUp
//
//----------------------------------------------------------------------------

void CButtonWindow::_OnLButtonUp(POINT)
{
    if (_IsCapture())
    {
        _EndCapture();
    }

    typeOfControl = 0;
}
