// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

//#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
// system/lang warning control
#pragma warning(push)
#pragma warning(disable: 4061 4100 4265 4365 4623 4625 4626 4668 4774 4917 4987 5026 5027 5039)
#include <windows.h>
#include <sal.h>

#include <combaseapi.h>
#include <olectl.h>
#include <assert.h>

#include <strsafe.h>
#include <intsafe.h>

#include <wrl.h>

#include "initguid.h"
#include "msctf.h"
#include "ctffunc.h"

#pragma warning(pop)

#pragma warning(disable: 4061 4263 4264 4365)

namespace wrl = Microsoft::WRL;

#define USE_RIBBON_ENGINE