#if defined(_MSC_BUILD)
#pragma once
#endif
#ifndef _RIBBON_CORELIB_H_
#define _RIBBON_CORELIB_H_

// global warning control
#if defined(_MSC_BUILD)
#pragma warning(disable: 4062 4464 4514 4571 4710 4711 4820)
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wunused-const-variable"
#endif

// system/lang warning control
#if defined(_MSC_BUILD)
#pragma warning(push)
#pragma warning(disable: 4061 4100 4365 4623 4625 4626 4668 4774 4917 4987 5026 5027 5039)
#endif // _MSC_BUILD 

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
//#include <regex> // something wrong under emstripten
#include <set>
#include <stdexcept> // std::runtime_error
#include <string>
#include <thread>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <cassert>
#include <cerrno>
#include <cfloat>
#include <cstdlib>

#include "external/json11/json11.hpp"

#if defined(_MSC_BUILD)
#pragma warning(pop)
#endif

namespace Ribbon {

#define COUNT_OF(a) (sizeof(a) / sizeof(*(a)))

struct ScopeExit {
	ScopeExit(const std::function<void ()>& _f) : f(_f) {}
	~ScopeExit() { f(); }
	std::function<void ()> f;
};

} // Ribbon

#include "corelib/interface.h"
#include "corelib/uuid.h"
#include "corelib/exception.h"
#include "corelib/textutils.h"

#include "corelib/containers.h"
#include "corelib/platformlib.h"
#include "corelib/refstring.h"
#include "corelib/flexiblebin.h"
#include "corelib/settings.h"
#include "corelib/threadpool.h"
#include "corelib/wcharconst.h"
#include "corelib/sipkeydef.h"

#endif // _RIBBON_CORELIB_H_
