#include "pch.h"
#include "oswindows.h"

#if defined(_WINDOWS) && !defined(__cplusplus_winrt )
#pragma comment(lib,"rpcrt4")
#endif
#if defined(__cplusplus_winrt )
#include <Windows.ApplicationModel.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#endif


namespace Ribbon { namespace Windows {

static int StubFunctionToGetAddress() noexcept { return 0; }

class MemoryMappedFile :
	public IMemoryMappedFile,
	public std::enable_shared_from_this<MemoryMappedFile>
{
public:
	MemoryMappedFile(const char* fileName) :
		m_hFile(nullptr),
		m_hMemMap(nullptr),
		m_mappedAdr(nullptr),
		m_fileSize(0)
	{
#if !defined(__cplusplus_winrt)
		std::u16string u16FileName = to_utf16(fileName);
		m_hFile = ::CreateFile(reinterpret_cast<const wchar_t*>(u16FileName.c_str()), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		THROW_IF_FALSE_MSG(m_hFile != INVALID_HANDLE_VALUE, "CreateFile() failed");

		THROW_IF_FALSE(GetFileSizeEx(m_hFile, reinterpret_cast<PLARGE_INTEGER>(&m_fileSize)));

		m_hMemMap = ::CreateFileMapping(m_hFile, 0, PAGE_READONLY, 0, 0, nullptr);
		THROW_IF_FALSE_MSG(m_hFile != nullptr, "CreateFileMapping() failed");

		m_mappedAdr = ::MapViewOfFile(m_hMemMap, FILE_MAP_READ, 0, 0, 0);
		THROW_IF_FALSE_MSG(m_hFile != nullptr, "MapViewOfFile() failed");
#else
		FILE *pf;
		if (fopen_s(&pf, fileName, "rb") == 0) {
			fseek(pf, 0, SEEK_END);
			m_fileSize = ftell(pf);
			fseek(pf, 0, SEEK_SET);

			m_fileData.reset(new uint8_t[m_fileSize]);
			fread(m_fileData.get(), 1, m_fileSize, pf);
			fclose(pf);

			m_mappedAdr = m_fileData.get();
		}
#endif
	}
	virtual ~MemoryMappedFile()
	{
#if !defined(__cplusplus_winrt)
		if (m_mappedAdr != nullptr) UnmapViewOfFile(m_mappedAdr);
		if (m_hMemMap != nullptr) CloseHandle(m_hMemMap);
		if (m_hFile != nullptr && m_hFile != INVALID_HANDLE_VALUE) CloseHandle(m_hFile);
#endif
	}
	const void* GetAddress() const override
	{
		return m_mappedAdr;
	}
	size_t GetFileSize() const override
	{
		return m_fileSize;
	}

private:
	::HANDLE m_hFile;
	::HANDLE m_hMemMap;
	const void *m_mappedAdr;
	size_t m_fileSize;
	std::unique_ptr<uint8_t[]> m_fileData;

	MemoryMappedFile(const MemoryMappedFile&) = delete;
	MemoryMappedFile& operator = (const MemoryMappedFile&) = delete;
};


class PlatformWindows :
	public IPlatform,
	public std::enable_shared_from_this<PlatformWindows>
{
private:
	mutable std::weak_ptr<ISetting> m_globalSetting;
	mutable bool m_isConfigRead = false;
	mutable bool m_isOutputConsole = false;
	mutable bool m_isOutputDebugString = false;
	mutable std::string m_outputFile;

public:
	PlatformWindows() {}
	virtual ~PlatformWindows() {}

#pragma region formatprint
	void _UpdateFlagsFromConfig() const
	{
		if (m_isConfigRead) return;
		m_isConfigRead = true;
		std::shared_ptr<ISetting> setting = GetSettings();

		m_isOutputConsole = setting->GetBool("Debug", "OutputConsole");
		m_isOutputDebugString = setting->GetBool("Debug", "OutputDebugString");
		m_outputFile = setting->GetExpandedString("Debug", "OutputFile");
	}

	void Printf(const char* fmt, ...) const override
	{
		_UpdateFlagsFromConfig();
		if (m_isOutputConsole || m_isOutputDebugString || m_outputFile.length() > 0)
		{
			char buf[4096];
			va_list arg;
			va_start(arg, fmt);
			::vsprintf_s(buf, fmt, arg);
			va_end(arg);
			_OutputFormatedText(buf, false);
		}
	}

	void Error(const char* fmt, ...) const override
	{
		_UpdateFlagsFromConfig();
		if (m_isOutputConsole || m_isOutputDebugString || m_outputFile.length() > 0)
		{
			char buf[4096];
			va_list arg;
			va_start(arg, fmt);
			::vsprintf_s(buf, fmt, arg);
			va_end(arg);

			_OutputFormatedText(buf, true);
		}
	}

	void _OutputFormatedText(const char* buf, bool isStdErr) const
	{
		const auto& utf16str = to_utf16(buf);
		const wchar_t* utf16alias = reinterpret_cast<const wchar_t*>(utf16str.c_str());

		if (m_isOutputConsole)
		{
			if (isStdErr) fputws(utf16alias, stderr); else fputws(utf16alias, stdout);
		}
		if (m_outputFile.length() > 0)
		{
			FILE *pf;
			if (fopen_s(&pf, m_outputFile.c_str(), "at") == 0)
			{
				fputs(buf, pf);
				fclose(pf);
			}
		}
		if (m_isOutputDebugString)
		{
			OutputDebugString(utf16alias);
		}
	}

#pragma endregion

	Ribbon::UUID CreateUUID() const override
	{
#if !defined(__cplusplus_winrt)
		UUID uuidRibbon;
		UuidCreate(reinterpret_cast< ::UUID*>(&uuidRibbon));
		return uuidRibbon;
#else
		return UUID_NULL;
#endif
	}

	std::shared_ptr<IMemoryMappedFile> OpenMemoryMappedFile(const char* fileName) const override
	{
		return std::make_shared<MemoryMappedFile>(fileName);
	}

	std::vector<std::string> EnumerateFiles(const char* directory, const char* fileMask) const override
	{
		std::vector<std::string> result;
		WIN32_FIND_DATA wfd;
		std::string filePattern(directory);
		(filePattern += "\\") += fileMask;

		HANDLE hFind = ::FindFirstFileEx(
			reinterpret_cast<const wchar_t*>(to_utf16(filePattern).c_str()), // Win32
			FindExInfoBasic,
			&wfd,
			FindExSearchNameMatch,
			nullptr,
			0);
		THROW_IF_FALSE_MSG(hFind != nullptr && hFind != INVALID_HANDLE_VALUE, "FindFirstFileEx() failed");
		auto scopeExit = ScopeExit([&]() { FindClose(hFind); });

		do {
			if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				result.push_back(to_utf8(reinterpret_cast<const char16_t*>(wfd.cFileName)));
			}
		} while (FindNextFile(hFind, &wfd));
		return result;
	}

	static void AppendPathDelimitor(std::string& path)
	{
		char lastChar = path[path.length() - 1];
		if (lastChar != '\\' || lastChar != '/')
		{
			path += "\\";
		}
	}

	std::string GetPathByType(PathType pathType) const override
	{
		switch (pathType)
		{
		case PathType::Current:
			{
				wchar_t currentDirBuf[MAX_PATH + 1]; // u16 on win32
				THROW_IF_FALSE(::GetCurrentDirectory(MAX_PATH + 1, currentDirBuf) > 0);
				std::string pathWork = to_utf8(reinterpret_cast<const char16_t*>(currentDirBuf));
				AppendPathDelimitor(pathWork);
				return pathWork;
			}
			break;

		case PathType::BinPath:
			{
#if !defined(__cplusplus_winrt)
				::HMODULE hModule = nullptr;
				THROW_IF_FALSE(::GetModuleHandleEx(
						GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						reinterpret_cast<LPCTSTR>(StubFunctionToGetAddress), &hModule));
				
				wchar_t moduleName[MAX_PATH + 1]; // u16
				THROW_IF_FALSE(::GetModuleFileName(hModule, moduleName, MAX_PATH + 1) > 0);

				wchar_t fullPathName[MAX_PATH + 1]; // u16
				wchar_t* fileNamePart;
				THROW_IF_FALSE(::GetFullPathName(moduleName, MAX_PATH + 1, fullPathName, &fileNamePart) > 0);
				THROW_IF_FALSE_MSG(fileNamePart != nullptr, "GetFullPathName() returns invalid lpFilePart");
				*fileNamePart = 0;
				return to_utf8(reinterpret_cast<const char16_t*>(fullPathName));
#else
				auto appInstalledFolder = ::Windows::ApplicationModel::Package::Current->InstalledLocation;
				auto pathWork = to_utf8(reinterpret_cast<const char16_t*>(appInstalledFolder->Path->Data()));
				AppendPathDelimitor(pathWork);
				return pathWork;
#endif
		}
			break;
		
		case PathType::UserPath:
			{
#if !defined(__cplusplus_winrt)
				wchar_t* gotPath = nullptr; // u16
				THROW_IF_FALSE(SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &gotPath)));
				auto freeGotPath = ScopeExit([&]() { CoTaskMemFree(gotPath); });

				std::string pathWork(to_utf8(reinterpret_cast<const char16_t*>(gotPath)));
				AppendPathDelimitor(pathWork);
				pathWork += TEXT_APPLICATION_NAME_A;
				std::u16string u16pathWork = to_utf16(pathWork);
				DWORD dwAttr = ::GetFileAttributes(reinterpret_cast<const wchar_t*>(u16pathWork.c_str()));
				if (dwAttr == (DWORD)(-1) || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					::CreateDirectory(reinterpret_cast<const wchar_t*>(u16pathWork.c_str()), nullptr);
				}
				AppendPathDelimitor(pathWork);
				return pathWork;
#else
				auto storageFolder = ::Windows::Storage::ApplicationData::Current->LocalFolder->Path;
				auto pathWork = to_utf8(reinterpret_cast<const char16_t*>(storageFolder->Data()));
				AppendPathDelimitor(pathWork);
				return pathWork;
#endif
			}
			break;
		}
		return std::string();
	}

	std::string DefaultSystemConfigFile(bool tryFallbackPath) const
	{
		// bin path
		std::string tryPath = GetPathByType(PathType::BinPath);
		tryPath += TEXT_SYSTEMCONFIG_FILENAME;
		std::u16string u16path = to_utf16(tryPath);
		DWORD fileAttr = ::GetFileAttributes(reinterpret_cast<const wchar_t*>(u16path.c_str()));
		if (fileAttr != (DWORD)(-1) && (fileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			return tryPath;
		}
#ifdef _DEBUG
		tryFallbackPath = true;
#endif
		if (tryFallbackPath)
		{
			// debug path
			tryPath = GetPathByType(PathType::BinPath);
			tryPath += "..\\..\\";
			tryPath += TEXT_SYSTEMCONFIG_FILENAME;
			u16path = to_utf16(tryPath);
			fileAttr = ::GetFileAttributes(reinterpret_cast<const wchar_t*>(u16path.c_str()));
			if (fileAttr != (DWORD)(-1) && (fileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				return tryPath;
			}
		}
		return std::string(); // null
	}

	std::string DefaultUserConfigFile() const
	{
		// search user path
		std::string targetPath = GetPathByType(PathType::UserPath);
		targetPath += TEXT_USERCONFIG_FILENAME;
		return targetPath;
	}

	std::shared_ptr<ISetting> GetSettings(bool tryFallbackPath = false) const override
	{
		std::shared_ptr<ISetting> globalSetting = m_globalSetting.lock();
		if (globalSetting)
		{
			return globalSetting;
		}
		
		std::shared_ptr<ISetting> userSettings = FACTORYCREATE(FileSetting);
		userSettings->SetFilename(DefaultUserConfigFile().c_str(), true/*writable*/);

		std::shared_ptr<ISetting> systemSettings = FACTORYCREATE(FileSetting);
		systemSettings->SetFilename(DefaultSystemConfigFile(tryFallbackPath).c_str(), false/*readonly*/);

		userSettings->CascadeReadonlySetting(systemSettings);

		m_globalSetting = userSettings;
		return userSettings;
	}
};

std::shared_ptr<IPlatform> s_platformHolder;
static struct PlatformWindowsRegister {
	PlatformWindowsRegister() {
		s_platformHolder = std::make_shared<PlatformWindows>();
		Platform = s_platformHolder.get();
	}
} s_PlatformWindowsRegister;

} /*Windows*/

extern const int OSKEY_ESCAPE = VK_ESCAPE;				// KEYCODE_ESCAPE			Esc
extern const int OSKEY_F1 = VK_F1;						// KEYCODE_F1				F1
extern const int OSKEY_F2 = VK_F2;						// KEYCODE_F2				F2
extern const int OSKEY_F3 = VK_F3;						// KEYCODE_F3				F3
extern const int OSKEY_F4 = VK_F4;						// KEYCODE_F4				F4
extern const int OSKEY_F5 = VK_F5;						// KEYCODE_F5				F5
extern const int OSKEY_F6 = VK_F6;						// KEYCODE_F6				F6
extern const int OSKEY_F7 = VK_F7;						// KEYCODE_F7				F7
extern const int OSKEY_F8 = VK_F8;						// KEYCODE_F8				F8
extern const int OSKEY_F9 = VK_F9;						// KEYCODE_F9				F9
extern const int OSKEY_F10 = VK_F10;					// KEYCODE_F10				F10
extern const int OSKEY_F11 = VK_F11;					// KEYCODE_F11				F11
extern const int OSKEY_F12 = VK_F12;					// KEYCODE_F12				F12
extern const int OSKEY_GRAVE = VK_OEM_3;				// KEYCODE_GRAVE			`
extern const int OSKEY_1 = '1';							// KEYCODE_1				1
extern const int OSKEY_2 = '2';							// KEYCODE_2				2
extern const int OSKEY_3 = '3';							// KEYCODE_3				3
extern const int OSKEY_4 = '4';							// KEYCODE_4				4
extern const int OSKEY_5 = '5';							// KEYCODE_5				5
extern const int OSKEY_6 = '6';							// KEYCODE_6				6
extern const int OSKEY_7 = '7';							// KEYCODE_7				7
extern const int OSKEY_8 = '8';							// KEYCODE_8				8
extern const int OSKEY_9 = '9';							// KEYCODE_9				9
extern const int OSKEY_0 = '0';							// KEYCODE_0				0
extern const int OSKEY_MINUS = VK_OEM_MINUS;			// KEYCODE_MINUS			-
extern const int OSKEY_PLUS = VK_OEM_PLUS;				// KEYCODE_PLUS				:
extern const int OSKEY_BACK = VK_BACK;					// KEYCODE_DEL				BackSpace
extern const int OSKEY_TAB = VK_TAB;					// KEYCODE_TAB				Tab
extern const int OSKEY_Q = 'Q';							// KEYCODE_Q				Q
extern const int OSKEY_W = 'W';							// KEYCODE_W				W
extern const int OSKEY_E = 'E';							// KEYCODE_E				E
extern const int OSKEY_R = 'R';							// KEYCODE_R				R
extern const int OSKEY_T = 'T';							// KEYCODE_T				T
extern const int OSKEY_Y = 'Y';							// KEYCODE_Y				Y
extern const int OSKEY_U = 'U';							// KEYCODE_U				U
extern const int OSKEY_I = 'I';							// KEYCODE_I				I
extern const int OSKEY_O = 'O';							// KEYCODE_O				O
extern const int OSKEY_P = 'P';							// KEYCODE_P				P
extern const int OSKEY_LBRACKET = VK_OEM_4;				// KEYCODE_LEFT_BRACKET		[
extern const int OSKEY_RBRACKET = VK_OEM_6;				// KEYCODE_RIGHT_BRACKET	]
extern const int OSKEY_BACKSLASH = VK_OEM_5;			// KEYCODE_BACKSLASH		[\\]
extern const int OSKEY_CAPSLOCK = VK_OEM_ATTN;			// KEYCODE_CAPS_LOCK		CapsLock
extern const int OSKEY_A = 'A';							// KEYCODE_A				A
extern const int OSKEY_S = 'S';							// KEYCODE_S				S
extern const int OSKEY_D = 'D';							// KEYCODE_D				D
extern const int OSKEY_F = 'F';							// KEYCODE_F				F
extern const int OSKEY_G = 'G';							// KEYCODE_G				G
extern const int OSKEY_H = 'H';							// KEYCODE_H				H
extern const int OSKEY_J = 'J';							// KEYCODE_J				J
extern const int OSKEY_K = 'K';							// KEYCODE_K				K
extern const int OSKEY_L = 'L';							// KEYCODE_L				L
extern const int OSKEY_SEMICOLON = VK_OEM_1;			// KEYCODE_SEMICOLON		
extern const int OSKEY_APOSTROPHE = VK_OEM_7;			// KEYCODE_APOSTROPHE		^
extern const int OSKEY_ENTER = VK_RETURN;				// KEYCODE_ENTER			Enter
extern const int OSKEY_LSHIFT = VK_SHIFT;				// KEYCODE_SHIFT_LEFT		Shift
extern const int OSKEY_Z = 'Z';							// KEYCODE_Z				Z
extern const int OSKEY_X = 'X';							// KEYCODE_X				X
extern const int OSKEY_C = 'C';							// KEYCODE_C				C
extern const int OSKEY_V = 'V';							// KEYCODE_V				V
extern const int OSKEY_B = 'B';							// KEYCODE_B				B
extern const int OSKEY_N = 'N';							// KEYCODE_N				N
extern const int OSKEY_M = 'M';							// KEYCODE_M				M
extern const int OSKEY_COMMA = VK_OEM_COMMA;			// KEYCODE_COMMA			,
extern const int OSKEY_PERIOD = VK_OEM_PERIOD;			// KEYCODE_PERIOD			.
extern const int OSKEY_SLASH = VK_OEM_2;				// KEYCODE_SLASH			/
extern const int OSKEY_RSHIFT = VK_SHIFT;				// KEYCODE_SHIFT_RIGHT		Shift
extern const int OSKEY_LCTRL = VK_CONTROL;				// KEYCODE_CTRL_LEFT		Ctrl
extern const int OSKEY_LWIN = VK_LWIN;					// 							L-Windows
extern const int OSKEY_LALT = VK_MENU;					// KEYCODE_ALT_LEFT			Alt
extern const int OSKEY_SPACE = VK_SPACE;				// KEYCODE_SPACE			Space
extern const int OSKEY_RALT = VK_MENU;					// KEYCODE_ALT_RIGHT		Alt
extern const int OSKEY_RWIN = VK_RWIN;					// 							R-Windows
extern const int OSKEY_MENU = VK_APPS;					// KEYCODE_MENU				ApplicationMenu
extern const int OSKEY_RCTRL = VK_CONTROL;				// KEYCODE_CTRL_RIGHT		Ctrl
extern const int OSKEY_SYSRQ = VK_SNAPSHOT;				// KEYCODE_SYSRQ			PrintScreen
extern const int OSKEY_SCROLL = VK_SCROLL;				// KEYCODE_SCROLL_LOCK		ScrollLock
extern const int OSKEY_BREAK = VK_PAUSE;				// KEYCODE_BREAK			Pause
extern const int OSKEY_INSERT = VK_INSERT;				// KEYCODE_INSERT			Insert
extern const int OSKEY_HOME = VK_HOME;					// KEYCODE_MOVE_HOME		Home
extern const int OSKEY_PAGEUP = VK_PRIOR;				// KEYCODE_PAGE_UP			PageUp
extern const int OSKEY_DELETE = VK_DELETE;				// KEYCODE_FORWARD_DEL		Delete
extern const int OSKEY_END = VK_END;					// KEYCODE_MOVE_END			End
extern const int OSKEY_PAGEDOWN = VK_NEXT;				// KEYCODE_PAGE_DOWN		PageDown
extern const int OSKEY_UP = VK_UP;						// KEYCODE_DPAD_UP			Up
extern const int OSKEY_LEFT = VK_LEFT;					// KEYCODE_DPAD_LEFT		<-
extern const int OSKEY_DOWN = VK_DOWN;					// KEYCODE_DPAD_DOWN		Down
extern const int OSKEY_RIGHT = VK_RIGHT;				// KEYCODE_DPAD_RIGHT		->
extern const int OSKEY_NUMLOCK = VK_NUMLOCK;			// KEYCODE_NUM_LOCK			NumLock
extern const int OSKEY_NUMPAD_DIVIDE = VK_DIVIDE;		// KEYCODE_NUMPAD_DIVIDE	/
extern const int OSKEY_NUMPAD_MULTIPLY = VK_MULTIPLY;	// KEYCODE_NUMPAD_MULTIPLY	*
extern const int OSKEY_NUMPAD_SUBTRACT = VK_SUBTRACT;	// KEYCODE_NUMPAD_SUBTRACT	-
extern const int OSKEY_NUMPAD_7 = VK_NUMPAD7;			// KEYCODE_NUMPAD_7			7
extern const int OSKEY_NUMPAD_8 = VK_NUMPAD8;			// KEYCODE_NUMPAD_8			8
extern const int OSKEY_NUMPAD_9 = VK_NUMPAD9;			// KEYCODE_NUMPAD_9			9
extern const int OSKEY_NUMPAD_ADD = VK_ADD;				// KEYCODE_NUMPAD_ADD		+
extern const int OSKEY_NUMPAD_4 = VK_NUMPAD4;			// KEYCODE_NUMPAD_4			4
extern const int OSKEY_NUMPAD_5 = VK_NUMPAD5;			// KEYCODE_NUMPAD_5			5
extern const int OSKEY_NUMPAD_6 = VK_NUMPAD6;			// KEYCODE_NUMPAD_6			6
extern const int OSKEY_NUMPAD_1 = VK_NUMPAD1;			// KEYCODE_NUMPAD_1			1
extern const int OSKEY_NUMPAD_2 = VK_NUMPAD2;			// KEYCODE_NUMPAD_2			2
extern const int OSKEY_NUMPAD_3 = VK_NUMPAD3;			// KEYCODE_NUMPAD_3			3
extern const int OSKEY_NUMPAD_0 = VK_NUMPAD0;			// KEYCODE_NUMPAD_0			0
extern const int OSKEY_NUMPAD_DOT = VK_DECIMAL;			// KEYCODE_NUMPAD_DOT		.
extern const int OSKEY_KANJI = VK_KANJI;				// KEYCODE_ZENKAKU_HANKAKU
extern const int OSKEY_HENKAN = VK_CONVERT;				// KEYCODE_HENKAN
extern const int OSKEY_MUHENKAN = VK_NONCONVERT;		// KEYCODE_MUHENKAN

#if !defined(VK_DBE_ALPHANUMERIC)
#define VK_DBE_ALPHANUMERIC              0x0f0
#define VK_DBE_KATAKANA                  0x0f1
#define VK_DBE_HIRAGANA                  0x0f2
#define VK_DBE_SBCSCHAR                  0x0f3
#define VK_DBE_DBCSCHAR                  0x0f4
#define VK_DBE_ROMAN                     0x0f5
#define VK_DBE_NOROMAN                   0x0f6
#define VK_DBE_ENTERWORDREGISTERMODE     0x0f7
#define VK_DBE_ENTERIMECONFIGMODE        0x0f8
#define VK_DBE_FLUSHSTRING               0x0f9
#define VK_DBE_CODEINPUT                 0x0fa
#define VK_DBE_NOCODEINPUT               0x0fb
#define VK_DBE_DETERMINESTRING           0x0fc
#define VK_DBE_ENTERDLGCONVERSIONMODE    0x0fd
#endif

extern const int OSKEY_ZENHAN = VK_DBE_DBCSCHAR;		// KEYCODE_ZENKAKU_HANKAKU
extern const int OSKEY_KATAHIRA = VK_DBE_KATAKANA;		// KEYCODE_KATAKANA_HIRAGANA
extern const int OSKEY_ROMAN = VK_DBE_ROMAN;			// KEYCODE_KANA ? ?
extern const int OSKEY_EISU = VK_DBE_ALPHANUMERIC;		// KEYCODE_EISU

} /*Ribbon*/
