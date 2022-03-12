#include "pch.h"
#include "oswindows.h"
#include "TsfClient/TipPrivate.h"
#include "TsfClient/TipGlobals.h"
#include "TsfClient/TipRibbonIME.h"

// TODO
namespace Ribbon { namespace Dictionary {
FACTORYEXTERN2(IProgramMain, DictionaryCompiler);
FACTORYEXTERN2(IProgramMain, DictionaryDumper);
FACTORYEXTERN2(IProgramMain, PhraseListBuilder);
} }

using namespace Ribbon;

class ConsoleConnection
{
    bool isForce;
    bool isConsoleAttached;
    FILE *pfIn;
    FILE *pfOut;
    FILE *pfErr;
public:
    ConsoleConnection(bool _isForce) :
        isForce(_isForce), isConsoleAttached(false),
        pfIn(nullptr), pfOut(nullptr), pfErr(nullptr)
    {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            isConsoleAttached = true;
        }
        else if (isForce) {
            AllocConsole();
            isConsoleAttached = true;
        }
        if (isConsoleAttached) {
            freopen_s(&pfIn, "CONIN$", "r", stdin);
            freopen_s(&pfOut, "CONOUT$", "w", stdout);
            freopen_s(&pfErr, "CONOUT$", "w", stderr);
        }
    }
    ~ConsoleConnection()
    {
        if (isConsoleAttached) {
            fclose(pfIn);
            fclose(pfOut);
            fclose(pfErr);
            FreeConsole();
        }
    }
};

class ClassFactory : public IClassFactory
{
    long refCount = 0;
    std::function<HRESULT(IUnknown*,REFIID,void**)> ctor;
public:
    ClassFactory(const std::function<HRESULT(IUnknown*,REFIID, void**)>& _ctor) :
        ctor(_ctor) {}
    virtual ~ClassFactory() {}

    // IUnknown methods.
    IFACEMETHODIMP QueryInterface(REFIID riid, _Outptr_ void **ppvObj) {
        if (riid == IID_IClassFactory || riid == IID_IUnknown) {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() {
        DllAddRef();
        return (ULONG)InterlockedIncrement(&refCount);
    }
    IFACEMETHODIMP_(ULONG) Release() {
        // this class is designed as static object, so this is never deleted.
        DllRelease();
        long val = InterlockedDecrement(&refCount);
        return (ULONG)(val = std::max(val, 0L));
    }
    IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) {
        if (pUnkOuter != nullptr) {
            return CLASS_E_NOAGGREGATION;
        }
        return ctor(pUnkOuter, riid, ppvObject);
    }
    IFACEMETHODIMP LockServer(BOOL fLock) {
        if (fLock) {
            AddRef();
        }
        else {
            Release();
        }
        return S_OK;
    }

    ClassFactory(const ClassFactory&) = delete;
    ClassFactory& operator = (const ClassFactory&) = delete;
};

ClassFactory RibbonImeClassFactory(
    [](_In_ IUnknown* pUnkOuter, _In_ REFIID riid, _COM_Outptr_ void** ppvObject) -> HRESULT {
        return CRibbonIME::CreateInstance(pUnkOuter, riid, ppvObject);
    });

// from Register.cpp
BOOL RegisterProfiles();
void UnregisterProfiles();
BOOL RegisterCategories();
void UnregisterCategories();
BOOL RegisterServer();
void UnregisterServer();

void DllAddRef()
{
    InterlockedIncrement(&Global::dllRefCount);
}

void DllRelease()
{
    InterlockedDecrement(&Global::dllRefCount);
}

extern "C" {

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID /*lpvReserved*/)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        // lpvReserved = null: LoadLibrary, != null: static load
        Global::dllInstanceHandle = hinstDLL;

        if (!InitializeCriticalSectionAndSpinCount(&Global::CS, 0)) {
            return FALSE;
        }
        if (!Global::RegisterWindowClass()) {
            return FALSE;
        }
        return TRUE;

    case DLL_PROCESS_DETACH:
        // lpvReserved = null: FreeLibrary, != null: process exit
        DeleteCriticalSection(&Global::CS);
        return TRUE;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        return TRUE;
    }
}

STDAPI DllGetClassObject(const CLSID& rclsid, const IID& riid, void** ppv)
{
    if (rclsid == Global::RibbonIMECLSID) {
        return RibbonImeClassFactory.QueryInterface(riid, ppv);
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
    return Global::dllRefCount > 0 ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer()
{
    if (!RegisterServer() ||
            !RegisterProfiles() ||
            !RegisterCategories()) {
        DllUnregisterServer();
        return E_FAIL;
    }
    return S_OK;
}

STDAPI DllUnregisterServer()
{
    UnregisterProfiles();
    UnregisterCategories();
    UnregisterServer();
    return S_OK;
}

STDAPI_(void) CreateSystemDictionary(HWND, HINSTANCE, LPSTR, int)
{
    ConsoleConnection conCon(true);
    try
    {
        int argc = 0;
        // here is win32 only, so WSTR is equal ot char16_t, u16string
        char16_t** argv = reinterpret_cast<char16_t**>(CommandLineToArgvW(GetCommandLineW(), &argc));
        auto scopeExit = ScopeExit([&]() { LocalFree(argv); });

        auto dictionaryCompiler = FACTORYCREATENS(Dictionary, DictionaryCompiler);
        dictionaryCompiler->ProgramMain(argc, argv);
    }
    CATCH_LOG();
}

STDAPI_(void) DumpSystemDictionary(HWND, HINSTANCE, LPSTR, int)
{
    ConsoleConnection conCon(true);
    try
    {
        int argc = 0;
        // here is win32 only, so WSTR is equal ot char16_t, u16string
        char16_t** argv = reinterpret_cast<char16_t**>(CommandLineToArgvW(GetCommandLineW(), &argc));
        auto scopeExit = ScopeExit([&]() { LocalFree(argv); });

        auto dictionaryCompiler = FACTORYCREATENS(Dictionary, DictionaryDumper);
        dictionaryCompiler->ProgramMain(argc, argv);
    }
    CATCH_LOG();
}

STDAPI_(void) CreatePhraseList(HWND, HINSTANCE, LPSTR, int)
{
    ConsoleConnection conCon(true);
    try
    {
        int argc = 0;
        // here is win32 only, so WSTR is equal ot char16_t, u16string
        char16_t **argv = reinterpret_cast<char16_t**>(CommandLineToArgvW(GetCommandLineW(), &argc));
        auto scopeExit = ScopeExit([&]() { LocalFree(argv); });

        auto predictionFilter = FACTORYCREATENS(Dictionary, PhraseListBuilder);
        predictionFilter->ProgramMain(argc, argv);
    }
    CATCH_LOG();
}

STDAPI_(void) Hello(HWND, HINSTANCE, LPSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#if 0
    {
        Ref<IUnknown> unk;
        CoCreateInstance(CLSID_RibbonIME, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&unk);

        Ref<ITfTextInputProcessorEx> rtip;
        unk->QueryInterface(IID_ITfTextInputProcessorEx, (void**)&rtip);

        printf("Hello\n");
    }
#endif
    CoUninitialize();
}

} // extern "C"