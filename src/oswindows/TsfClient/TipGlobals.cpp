// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "TipPrivate.h"
#include "resource.h"
#include "UIBaseWindow.h"
#include "TipDefine.h"
#include "RibbonIMEBaseStructure.h"

namespace Global {
HINSTANCE dllInstanceHandle;

LONG dllRefCount = 0;

CRITICAL_SECTION CS;
HFONT defaultlFontHandle;				// Global font object we use everywhere

//---------------------------------------------------------------------
// RibbonIME CLSID
//---------------------------------------------------------------------
// {64F5D9EB-6B93-44F9-8063-8A3CB126BFD6}
extern const CLSID RibbonIMECLSID = { 
	0x64f5d9eb, 0x6b93, 0x44f9, { 0x80, 0x63, 0x8a, 0x3c, 0xb1, 0x26, 0xbf, 0xd6 } };

//---------------------------------------------------------------------
// Profile GUID
//---------------------------------------------------------------------
// {CFA407FC-370F-4A32-83B4-E33C835F6B8B}
extern const GUID RibbonIMEGuidProfile = { 
	0xcfa407fc, 0x370f, 0x4a32, { 0x83, 0xb4, 0xe3, 0x3c, 0x83, 0x5f, 0x6b, 0x8b } };

//---------------------------------------------------------------------
// PreserveKey GUID
//---------------------------------------------------------------------
// {13AB93DA-066C-4692-8555-62210A31C910}
extern const GUID RibbonIMEGuidImeModePreserveKey = {
	0x13ab93da, 0x66c, 0x4692, { 0x85, 0x55, 0x62, 0x21, 0xa, 0x31, 0xc9, 0x10 } };

//---------------------------------------------------------------------
// Compartments
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// LanguageBars
//---------------------------------------------------------------------

// {FD84DFDB-A659-4012-91F7-0AC19E74CF10}
extern const GUID RibbonIMEGuidLangBarIMEMode = {
	0xfd84dfdb, 0xa659, 0x4012, { 0x91, 0xf7, 0xa, 0xc1, 0x9e, 0x74, 0xcf, 0x10 } };

// {7D957787-CECD-4B73-A892-78DDB3C42C31}
extern const GUID RibbonIMEGuidDisplayAttributeInput = {
	0x7d957787, 0xcecd, 0x4b73, { 0xa8, 0x92, 0x78, 0xdd, 0xb3, 0xc4, 0x2c, 0x31 } };

// {9A1CC683-F2A7-4701-9C6E-2DA69A5CD474}
extern const GUID RibbonIMEGuidDisplayAttributeConverted = {
	0x7c34cc38, 0xed55, 0x43d3, { 0x97, 0xa2, 0xbb, 0x81, 0xc0, 0x1b, 0x9a, 0x94 } };

//---------------------------------------------------------------------
// UI element
//---------------------------------------------------------------------

// {2F6E5DAE-8C84-4734-A7CA-B05C1DB7940F}
extern const GUID RibbonIMEGuidCandUIElement = {
    0x2f6e5dae, 0x8c84, 0x4734, { 0xa7, 0xca, 0xb0, 0x5c, 0x1d, 0xb7, 0x94, 0xf } };

//---------------------------------------------------------------------
// Unicode byte order mark
//---------------------------------------------------------------------
extern const WCHAR UnicodeByteOrderMark = 0xFEFF;

//---------------------------------------------------------------------
// defined item in setting file table [PreservedKey] section
//---------------------------------------------------------------------
extern const WCHAR ImeModeDescription[] = L"Native/English input";
extern const int ImeModeOnIcoIndex = IME_MODE_ON_ICON_INDEX;
extern const int ImeModeOffIcoIndex = IME_MODE_OFF_ICON_INDEX;

//---------------------------------------------------------------------
// defined item in setting file table [LanguageBar] section
//---------------------------------------------------------------------
extern const WCHAR LangbarImeModeDescription[] = L"Conversion mode";

//---------------------------------------------------------------------
// windows class / titile / atom
//---------------------------------------------------------------------
extern const WCHAR CandidateClassName[] = L"RibbonIME.CandidateWindow";
ATOM AtomCandidateWindow;

extern const WCHAR ShadowClassName[] = L"RibbonIME.ShadowWindow";
ATOM AtomShadowWindow;

extern const WCHAR ScrollBarClassName[] = L"RibbonIME.ScrollBarWindow";
ATOM AtomScrollBarWindow;

BOOL RegisterWindowClass()
{
    if (!CBaseWindow::_InitWindowClass(CandidateClassName, &AtomCandidateWindow))
    {
        return FALSE;
    }
    if (!CBaseWindow::_InitWindowClass(ShadowClassName, &AtomShadowWindow))
    {
        return FALSE;
    }
    if (!CBaseWindow::_InitWindowClass(ScrollBarClassName, &AtomScrollBarWindow))
    {
        return FALSE;
    }
    return TRUE;
}

//---------------------------------------------------------------------
// defined full width characters for Double/Single byte conversion
//---------------------------------------------------------------------
extern const WCHAR FullWidthCharTable[] = {
    //         !       "       #       $       %       &       '       (    )       *       +       ,       -       .       /
    0x3000, 0xFF01, 0xFF02, 0xFF03, 0xFF04, 0xFF05, 0xFF06, 0xFF07, 0xFF08, 0xFF09, 0xFF0A, 0xFF0B, 0xFF0C, 0xFF0D, 0xFF0E, 0xFF0F,
    // 0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
    0xFF10, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18, 0xFF19, 0xFF1A, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0xFF1F,
    // @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       0
    0xFF20, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
    // P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
    0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF3F,
    // '       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o       
    0xFF40, 0xFF41, 0xFF42, 0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A, 0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F,
    // p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~
    0xFF50, 0xFF51, 0xFF52, 0xFF53, 0xFF54, 0xFF55, 0xFF56, 0xFF57, 0xFF58, 0xFF59, 0xFF5A, 0xFF5B, 0xFF5C, 0xFF5D, 0xFF5E
};

//---------------------------------------------------------------------
// defined punctuation characters
//---------------------------------------------------------------------
extern const struct _PUNCTUATION PunctuationTable[14] = {
    {L'!',  0xFF01},
    {L'$',  0xFFE5},
    {L'&',  0x2014},
    {L'(',  0xFF08},
    {L')',  0xFF09},
    {L',',  0xFF0C},
    {L'.',  0x3002},
    {L':',  0xFF1A},
    {L';',  0xFF1B},
    {L'?',  0xFF1F},
    {L'@',  0x00B7},
    {L'\\', 0x3001},
    {L'^',  0x2026},
    {L'_',  0x2014}
};

//+---------------------------------------------------------------------------
//
// CheckModifiers
//
//----------------------------------------------------------------------------

#define TF_MOD_ALLALT     (TF_MOD_RALT | TF_MOD_LALT | TF_MOD_ALT)
#define TF_MOD_ALLCONTROL (TF_MOD_RCONTROL | TF_MOD_LCONTROL | TF_MOD_CONTROL)
#define TF_MOD_ALLSHIFT   (TF_MOD_RSHIFT | TF_MOD_LSHIFT | TF_MOD_SHIFT)
#define TF_MOD_RLALT      (TF_MOD_RALT | TF_MOD_LALT)
#define TF_MOD_RLCONTROL  (TF_MOD_RCONTROL | TF_MOD_LCONTROL)
#define TF_MOD_RLSHIFT    (TF_MOD_RSHIFT | TF_MOD_LSHIFT)

#define CheckMod(m0, m1, mod)        \
    if (m1 & TF_MOD_ ## mod ##)      \
{ \
    if (!(m0 & TF_MOD_ ## mod ##)) \
{      \
    return FALSE;   \
}      \
} \
    else       \
{ \
    if ((m1 ^ m0) & TF_MOD_RL ## mod ##)    \
{      \
    return FALSE;   \
}      \
} \



BOOL CheckModifiers(UINT modCurrent, UINT mod)
{
    mod &= ~TF_MOD_ON_KEYUP;

    if (mod & TF_MOD_IGNORE_ALL_MODIFIER)
    {
        return TRUE;
    }

    if (modCurrent == mod)
    {
        return TRUE;
    }

    if (modCurrent && !mod)
    {
        return FALSE;
    }

    CheckMod(modCurrent, mod, ALT);
    CheckMod(modCurrent, mod, SHIFT);
    CheckMod(modCurrent, mod, CONTROL);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// UpdateModifiers
//
//    wParam - virtual-key code
//    lParam - [0-15]  Repeat count
//  [16-23] Scan code
//  [24]    Extended key
//  [25-28] Reserved
//  [29]    Context code
//  [30]    Previous key state
//  [31]    Transition state
//----------------------------------------------------------------------------

USHORT ModifiersValue = 0;
BOOL   IsShiftKeyDownOnly = FALSE;
BOOL   IsControlKeyDownOnly = FALSE;
BOOL   IsAltKeyDownOnly = FALSE;

BOOL UpdateModifiers(WPARAM wParam, LPARAM lParam)
{
    // high-order bit : key down
    // low-order bit  : toggled
    SHORT sksMenu = GetKeyState(VK_MENU);
    SHORT sksCtrl = GetKeyState(VK_CONTROL);
    SHORT sksShft = GetKeyState(VK_SHIFT);

    switch (wParam & 0xff)
    {
    case VK_MENU:
        // is VK_MENU down?
        if (sksMenu & 0x8000)
        {
            // is extended key?
            if (lParam & 0x01000000)
            {
                ModifiersValue |= (TF_MOD_RALT | TF_MOD_ALT);
            }
            else
            {
                ModifiersValue |= (TF_MOD_LALT | TF_MOD_ALT);
            }

            // is previous key state up?
            if (!(lParam & 0x40000000))
            {
                // is VK_CONTROL and VK_SHIFT up?
                if (!(sksCtrl & 0x8000) && !(sksShft & 0x8000))
                {
                    IsAltKeyDownOnly = TRUE;
                }
                else
                {
                    IsShiftKeyDownOnly = FALSE;
                    IsControlKeyDownOnly = FALSE;
                    IsAltKeyDownOnly = FALSE;
                }
            }
        }
        break;

    case VK_CONTROL:
        // is VK_CONTROL down?
        if (sksCtrl & 0x8000)    
        {
            // is extended key?
            if (lParam & 0x01000000)
            {
                ModifiersValue |= (TF_MOD_RCONTROL | TF_MOD_CONTROL);
            }
            else
            {
                ModifiersValue |= (TF_MOD_LCONTROL | TF_MOD_CONTROL);
            }

            // is previous key state up?
            if (!(lParam & 0x40000000))
            {
                // is VK_SHIFT and VK_MENU up?
                if (!(sksShft & 0x8000) && !(sksMenu & 0x8000))
                {
                    IsControlKeyDownOnly = TRUE;
                }
                else
                {
                    IsShiftKeyDownOnly = FALSE;
                    IsControlKeyDownOnly = FALSE;
                    IsAltKeyDownOnly = FALSE;
                }
            }
        }
        break;

    case VK_SHIFT:
        // is VK_SHIFT down?
        if (sksShft & 0x8000)    
        {
            // is scan code 0x36(right shift)?
            if (((lParam >> 16) & 0x00ff) == 0x36)
            {
                ModifiersValue |= (TF_MOD_RSHIFT | TF_MOD_SHIFT);
            }
            else
            {
                ModifiersValue |= (TF_MOD_LSHIFT | TF_MOD_SHIFT);
            }

            // is previous key state up?
            if (!(lParam & 0x40000000))
            {
                // is VK_MENU and VK_CONTROL up?
                if (!(sksMenu & 0x8000) && !(sksCtrl & 0x8000))
                {
                    IsShiftKeyDownOnly = TRUE;
                }
                else
                {
                    IsShiftKeyDownOnly = FALSE;
                    IsControlKeyDownOnly = FALSE;
                    IsAltKeyDownOnly = FALSE;
                }
            }
        }
        break;

    default:
        IsShiftKeyDownOnly = FALSE;
        IsControlKeyDownOnly = FALSE;
        IsAltKeyDownOnly = FALSE;
        break;
    }

    if (!(sksMenu & 0x8000))
    {
        ModifiersValue &= ~TF_MOD_ALLALT;
    }
    if (!(sksCtrl & 0x8000))
    {
        ModifiersValue &= ~TF_MOD_ALLCONTROL;
    }
    if (!(sksShft & 0x8000))
    {
        ModifiersValue &= ~TF_MOD_ALLSHIFT;
    }

    return TRUE;
}

//---------------------------------------------------------------------
// override CompareElements
//---------------------------------------------------------------------
BOOL CompareElements(LCID locale, const CStringRange* pElement1, const CStringRange* pElement2)
{
    return (CStringRange::Compare(locale, (CStringRange*)pElement1, (CStringRange*)pElement2) == CSTR_EQUAL) ? TRUE : FALSE;
}

//---------------------------------------------------------------------
// print HRESULT
//---------------------------------------------------------------------
void PrintHRESULT(HRESULT hr, const char* comment)
{
	char buf[256];
	sprintf_s(buf, "[%lX] %s\n", hr, comment);
	OutputDebugStringA(buf);
}

}
