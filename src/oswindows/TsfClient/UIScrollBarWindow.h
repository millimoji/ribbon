//////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2005  Microsoft Corporation.  All rights reserved.
//
//  ScrollBarWindow.h
//
// CScrollBarWindow declaration.
//
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UIBaseWindow.h"
#include "UIButtonWindow.h"

class CScrollButtonWindow;

enum SCROLLBAR_DIRECTION
{
    SCROLL_NONE_DIR,
    SCROLL_PAGEDOWN,    // or page left
    SCROLL_PAGEUP       // or page right
};

enum SHELL_MODE
{
    STOREAPP,
    DESKTOP,
};

struct CScrollInfo
{
    CScrollInfo()
    {
        nMax = 0;
        nPage = 0;
        nPos = 0;
    }

    int  nMax;
    int  nPage;
    int  nPos;
};

class CScrollBarWindow;

class CScrollBarWindowFactory
{
public:
    static CScrollBarWindowFactory* Instance();
    CScrollBarWindow* MakeScrollBarWindow(SHELL_MODE shellMode);
    void Release();

protected:
    CScrollBarWindowFactory();

private:
    static CScrollBarWindowFactory* _instance;

};
//////////////////////////////////////////////////////////////////////
//
// CScrollBarWindow declaration.
//
//////////////////////////////////////////////////////////////////////

class CScrollBarWindow : public CBaseWindow
{
public:
    CScrollBarWindow();
    virtual ~CScrollBarWindow();

    BOOL _Create(ATOM atom, DWORD dwExStyle, DWORD dwStyle, CBaseWindow *pParent = nullptr, int wndWidth = 0, int wndHeight = 0);

    void _Resize(int x, int y, int cx, int cy);
    void _Show(BOOL isShowWnd);
    void _Enable(BOOL isEnable);

    LRESULT CALLBACK _WindowProcCallback(_In_ HWND wndHandle, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
    virtual void _OnPaint(_In_ HDC dcHandle, _In_ PAINTSTRUCT *pps);
    virtual void _OnLButtonDown(POINT pt);
    virtual void _OnLButtonUp(POINT pt);
    virtual void _OnMouseMove(POINT pt);
    virtual void _OnTimer() { };

    virtual void _OnOwnerWndMoved(BOOL isResized);

    virtual void _SetScrollInfo(_In_ CScrollInfo *lpsi);
    virtual void _GetScrollInfo(_Out_ CScrollInfo *lpsi);

    virtual BOOL _GetBtnUpRect(_Out_ RECT *prc);
    virtual BOOL _GetBtnDnRect(_Out_ RECT *prc);

    virtual void _ShiftLine(int nLine, BOOL isNotify)
    {
        DWORD dwSB = 0;

        dwSB = (nLine > 0 ? SB_LINEDOWN :
            nLine < 0 ? SB_LINEUP : 0);

        _SetCurPos(_scrollInfo.nPos + nLine, isNotify ? dwSB : -1);
    }

    virtual void _ShiftPage(int nPage, BOOL isNotify)
    {
        DWORD dwSB = 0;

        dwSB = (nPage > 0 ? SB_PAGEDOWN :
            nPage < 0 ? SB_PAGEUP : 0);

        _SetCurPos(_scrollInfo.nPos + _scrollInfo.nPage * nPage, isNotify ? dwSB : -1);
    }

    virtual void _ShiftPosition(int iPos, BOOL isNotify)
    {
        _SetCurPos(iPos, isNotify ? SB_THUMBPOSITION : -1);
    }

    virtual void _GetScrollArea(_Out_ RECT *prc);
    virtual void _SetCurPos(int nPos, int dwSB);

private:
    void _AdjustWindowPos();

    CScrollButtonWindow* _pBtnUp;
    CScrollButtonWindow* _pBtnDn;

    CScrollInfo _scrollInfo;
    SIZE _sizeOfScrollBtn;
    SCROLLBAR_DIRECTION _scrollDir;
};


//////////////////////////////////////////////////////////////////////
//
// CScrollBarNullWindow declaration.
//
//////////////////////////////////////////////////////////////////////

class CScrollBarNullWindow : public CScrollBarWindow
{
public:
    CScrollBarNullWindow() { };
    virtual ~CScrollBarNullWindow() { };

    virtual BOOL _Create(ATOM, DWORD, DWORD, _In_opt_ CBaseWindow* = nullptr, int = 0, int = 0, _In_opt_ HWND = nullptr)
    { 
        return TRUE; 
    };

    virtual void _Destroy() { };

    virtual void _Move(int, int) { };
    virtual void _Resize(int, int, int, int) { };
    virtual void _Show(BOOL) { };
    virtual BOOL _IsWindowVisible() { return TRUE; };
    virtual void _Enable(BOOL) { };
    virtual BOOL _IsEnabled() { return TRUE; };
    virtual void _InvalidateRect() { };
    virtual BOOL _GetWindowRect(_Out_ LPRECT lpRect) { lpRect->bottom = 0; lpRect->left = 0; lpRect->right = 0; lpRect->top = 0; return TRUE; };
    virtual BOOL _GetClientRect(_Out_ LPRECT lpRect) { lpRect->bottom = 0; lpRect->left = 0; lpRect->right = 0; lpRect->top = 0; return TRUE; };

    virtual LRESULT CALLBACK _WindowProcCallback(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM) 
    { 
        return 0; 
    };
    virtual void _OnPaint(_In_ HDC, _In_ PAINTSTRUCT*) { };
	virtual void _OnLButtonDown(POINT) { }
    virtual void _OnLButtonUp(POINT) { }
    virtual void _OnMouseMove(POINT) { }
    virtual void _OnTimer() { };

    virtual void _OnOwnerWndMoved(BOOL) { };

    virtual void _SetScrollInfo(_In_ CScrollInfo*) { };
    virtual void _GetScrollInfo(_In_ CScrollInfo*) { };

    virtual BOOL _GetBtnUpRect(_Out_ RECT *prc) { prc->bottom = 0; prc->left = 0; prc->right = 0; prc->top = 0; return TRUE; };
    virtual BOOL _GetBtnDnRect(_Out_ RECT *prc) { prc->bottom = 0; prc->left = 0; prc->right = 0; prc->top = 0; return TRUE; };

    virtual void _ShiftLine(int, BOOL) { };
    virtual void _ShiftPage(int, BOOL) { };
    virtual void _ShiftPosition(int, BOOL) { };
    virtual void _GetScrollArea(_Out_ RECT *prc) { prc->bottom = 0; prc->left = 0; prc->right = 0; prc->top = 0; };
    virtual void _SetCurPos(int, int) { };
};

//////////////////////////////////////////////////////////////////////
//
// CScrollButtonWondow declaration.
//
//////////////////////////////////////////////////////////////////////

class CScrollButtonWindow : public CButtonWindow
{
public:
    CScrollButtonWindow(UINT uSubType)
    {
        subTypeOfControl = uSubType;
    }
    virtual ~CScrollButtonWindow()
    {
    }

    LRESULT CALLBACK _WindowProcCallback(_In_ HWND, UINT, _In_ WPARAM, _In_ LPARAM) 
    { 
        return 0; 
    }
    virtual void _OnPaint(_In_ HDC dcHandle, _In_ PAINTSTRUCT *pps);
    virtual void _OnLButtonDown(POINT pt);
    virtual void _OnLButtonUp(POINT pt);
    virtual void _OnTimer();

private:
    UINT subTypeOfControl;        // DFCS_SCROLLxxx for DrawFrameControl
};
