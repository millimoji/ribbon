#include "pch.h"
#include "osios.h"

namespace Ribbon { namespace iOS {

std::string s_resourceRoot;
    
inline int StubFunctionToGetAddress() { return 0; }

class MemoryMappedFile :
	public IMemoryMappedFile,
	public std::enable_shared_from_this<MemoryMappedFile>
{
public:
	MemoryMappedFile(const char* fileName) :
		m_mappedAdr(nullptr)
	{
		m_fileSize = _GetFileSize(fileName);
		//Open file
		m_fd = open(fileName, O_RDONLY, 0);
		//assert(fd != -1);
		//Execute mmap
		m_mappedAdr = mmap(nullptr, m_fileSize, PROT_READ, MAP_PRIVATE /*| MAP_POPULATE*/, m_fd, 0);
		//assert(mmappedData != MAP_FAILED);
		//Write the mmapped data to stdout (= FD #1)
		write(1, m_mappedAdr, m_fileSize);
	}
	virtual ~MemoryMappedFile()
	{
		//Cleanup
		/*int rc =*/ munmap(m_mappedAdr, m_fileSize);
		//assert(rc == 0);
		close(m_fd);
	}
	const void* GetAddress() const override
	{
		return m_mappedAdr;
	}
	size_t GetFileSize() const  override
	{
		return m_fileSize;
	}
	size_t _GetFileSize(const char* filename) const
	{
		struct stat st;
		stat(filename, &st);
		return static_cast<size_t>(st.st_size);
	}

private:
	void *m_mappedAdr;
	size_t m_fileSize;
	int m_fd;
};


class PlatformiOS :
	public IPlatform,
	public std::enable_shared_from_this<PlatformiOS>
{
private:
	mutable std::weak_ptr<ISetting> m_globalSetting;
	mutable bool m_isConfigRead = false;
	mutable bool m_isOutputConsole = false;
	mutable bool m_isOutputDebugString = false;
	mutable std::string m_outputFile;

public:
	PlatformiOS() {}
	virtual ~PlatformiOS() {}

	void _UpdateFlagsFromConfig() const
	{
		if (m_isConfigRead) return;
		m_isConfigRead = true;
		std::shared_ptr<ISetting> setting = GetSettings(false);

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
			::sprintf(buf, fmt, arg);
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
			::sprintf(buf, fmt, arg);
			va_end(arg);

			_OutputFormatedText(buf, true);
		}
	}

	void _OutputFormatedText(const char* buf, bool isStdErr) const
	{
		if (m_isOutputConsole)
		{
			if (isStdErr) fputs(buf, stderr); else fputs(buf, stdout);
		}
		if (m_outputFile.length() > 0)
		{
			FILE *pf;
			if ((pf = fopen(m_outputFile.c_str(), "at")) != nullptr)
			{
				fputs(buf, pf);
				fclose(pf);
			}
		}
	}

	Ribbon::UUID CreateUUID() const override
	{
#if 0 // #if !defined(__cplusplus_winrt)
		UUID uuidRibbon;
		UuidCreate(reinterpret_cast< ::UUID*>(&uuidRibbon));
		return uuidRibbon;
#else
		return UUID_NULL;
#endif
	}

	std::shared_ptr<IMemoryMappedFile> OpenMemoryMappedFile(const char* fileName) const override
	{
		return std::dynamic_pointer_cast<IMemoryMappedFile>(std::make_shared<MemoryMappedFile>(fileName));
	}

	std::vector<std::string> EnumerateFiles(const char* directory, const char* fileMask) const override
	{
		return std::vector<std::string>();
	}

	static void AppendPathDelimitor(std::string& path)
	{
		char lastChar = path[path.length() - 1];
		if (lastChar != '\\' || lastChar != '/')
		{
			path += "/";
		}
	}

	std::string GetPathByType(PathType pathType) const override
	{
		switch (pathType)
		{
		case PathType::Current:
            return s_resourceRoot;
		case PathType::BinPath:
            return s_resourceRoot;
		case PathType::UserPath:
            return s_resourceRoot;
		}
		return std::string();
	}

	std::string DefaultSystemConfigFile() const
	{
		return std::string(GetPathByType(PathType::BinPath) + "config.txt");
	}

	std::string DefaultUserConfigFile() const
	{
		return std::string(GetPathByType(PathType::BinPath) + "userconfig.txt");
	}

	std::shared_ptr<ISetting> GetSettings(bool /*tryFallbackPath*/) const override
	{
		std::shared_ptr<ISetting> globalSetting = m_globalSetting.lock();
		if (globalSetting)
		{
			return globalSetting;
		}
		
		std::shared_ptr<ISetting> userSettings = FACTORYCREATE(FileSetting);
		userSettings->SetFilename(DefaultUserConfigFile().c_str(), true/*writable*/);

		std::shared_ptr<ISetting> systemSettings = FACTORYCREATE(FileSetting);
		systemSettings->SetFilename(DefaultSystemConfigFile().c_str(), false/*readonly*/);

		userSettings->CascadeReadonlySetting(systemSettings);

		m_globalSetting = userSettings;
		return userSettings;
	}
};

std::shared_ptr<IPlatform> s_platformHolder;
static struct PlatformiOSRegister {
	PlatformiOSRegister() {
		s_platformHolder = std::make_shared<PlatformiOS>();
		Platform = s_platformHolder.get();
	}
} s_PlatformiOSRegister;

} /*iOS*/

extern const int OSKEY_ESCAPE			= 53;	// esc
extern const int OSKEY_F1				= 122;	// F1
extern const int OSKEY_F2				= 120;	// F2
extern const int OSKEY_F3				= 99;	// F3
extern const int OSKEY_F4				= 118;	// F4
extern const int OSKEY_F5				= 96;	// F5
extern const int OSKEY_F6				= 97;	// F6
extern const int OSKEY_F7				= 98;	// F7
extern const int OSKEY_F8				= 100;	// F8
extern const int OSKEY_F9				= 101;	// F9
extern const int OSKEY_F10				= 109;	// F10
extern const int OSKEY_F11				= 103;	// F11
extern const int OSKEY_F12				= 111;	// F12
extern const int OSKEY_GRAVE			= -1;	// 
extern const int OSKEY_1				= 18;	// 1
extern const int OSKEY_2				= 19;	// 2
extern const int OSKEY_3				= 20;	// 3
extern const int OSKEY_4				= 21;	// 4
extern const int OSKEY_5				= 23;	// 5
extern const int OSKEY_6				= 22;	// 6
extern const int OSKEY_7				= 26;	// 7
extern const int OSKEY_8				= 28;	// 8
extern const int OSKEY_9				= 25;	// 9
extern const int OSKEY_0				= 29;	// 0
extern const int OSKEY_MINUS			= 27;	// -
extern const int OSKEY_PLUS				= 24;	// +
extern const int OSKEY_BACK				= 51;	// delete
extern const int OSKEY_TAB				= 48;	// tab
extern const int OSKEY_Q				= 12;	// q
extern const int OSKEY_W				= 13;	// w
extern const int OSKEY_E				= 14;	// e
extern const int OSKEY_R				= 15;	// r
extern const int OSKEY_T				= 17;	// t
extern const int OSKEY_Y				= 16;	// y
extern const int OSKEY_U				= 32;	// u
extern const int OSKEY_I				= 34;	// i
extern const int OSKEY_O				= 31;	// o
extern const int OSKEY_P				= 35;	// p
extern const int OSKEY_LBRACKET			= 33;	// [
extern const int OSKEY_RBRACKET			= 30;	// ]
extern const int OSKEY_BACKSLASH		= 42;	// backslash
extern const int OSKEY_CAPSLOCK			= 57;	// caps lock
extern const int OSKEY_A				= 0;	// a
extern const int OSKEY_S				= 1;	// s
extern const int OSKEY_D				= 2;	// d
extern const int OSKEY_F				= 3;	// f
extern const int OSKEY_G				= 5;	// g
extern const int OSKEY_H				= 4;	// h
extern const int OSKEY_J				= 38;	// j
extern const int OSKEY_K				= 40;	// k
extern const int OSKEY_L				= 37;	// l
extern const int OSKEY_SEMICOLON		= 41;	// ;
extern const int OSKEY_APOSTROPHE		= 39;	// Åe
extern const int OSKEY_ENTER			= 52;	// return
extern const int OSKEY_LSHIFT			= 56;	// left shift
extern const int OSKEY_Z				= 6;	// z
extern const int OSKEY_X				= 7;	// x
extern const int OSKEY_C				= 8;	// c
extern const int OSKEY_V				= 9;	// v
extern const int OSKEY_B				= 11;	// b
extern const int OSKEY_N				= 45;	// n
extern const int OSKEY_M				= 46;	// m
extern const int OSKEY_COMMA			= 43;	// ,
extern const int OSKEY_PERIOD			= 47;	// .
extern const int OSKEY_SLASH			= 44;	/// 
extern const int OSKEY_RSHIFT			= 60;	// right shift
extern const int OSKEY_LCTRL			= 59;	// left control
extern const int OSKEY_LWIN				= -1;	// 
extern const int OSKEY_LALT				= 58;	// left option
extern const int OSKEY_SPACE			= 49;	// space
extern const int OSKEY_RALT				= 61;	// right option
extern const int OSKEY_RWIN				= -1;	// 
extern const int OSKEY_MENU				= -1;	// 
extern const int OSKEY_RCTRL			= 62;	// right control
extern const int OSKEY_SYSRQ			= -1;	// 
extern const int OSKEY_SCROLL			= -1;	// 
extern const int OSKEY_BREAK			= -1;	// 
extern const int OSKEY_INSERT			= -1;	// 
extern const int OSKEY_HOME				= 116;	// home
extern const int OSKEY_PAGEUP			= 115;	// page up
extern const int OSKEY_DELETE			= 51;	// delete
extern const int OSKEY_END				= 121;	// end
extern const int OSKEY_PAGEDOWN			= 119;	// page down
extern const int OSKEY_UP				= 126;	// up arrow
extern const int OSKEY_LEFT				= 123;	// left arrow
extern const int OSKEY_DOWN				= 125;	// down arrow
extern const int OSKEY_RIGHT			= 124;	// right arrow
extern const int OSKEY_NUMLOCK			= -1;	// 
extern const int OSKEY_NUMPAD_DIVIDE	= 75;	// / 
extern const int OSKEY_NUMPAD_MULTIPLY	= 67;	// *
extern const int OSKEY_NUMPAD_SUBTRACT	= 78;	// -
extern const int OSKEY_NUMPAD_7			= 89;	// 7
extern const int OSKEY_NUMPAD_8			= 91;	// 8
extern const int OSKEY_NUMPAD_9			= 92;	// 9
extern const int OSKEY_NUMPAD_ADD		= 69;	// +
extern const int OSKEY_NUMPAD_4			= 86;	// 4
extern const int OSKEY_NUMPAD_5			= 87;	// 5
extern const int OSKEY_NUMPAD_6			= 88;	// 6
extern const int OSKEY_NUMPAD_1			= 83;	// 1
extern const int OSKEY_NUMPAD_2			= 84;	// 2
extern const int OSKEY_NUMPAD_3			= 85;	// 3
extern const int OSKEY_NUMPAD_0			= 82;	// 0
extern const int OSKEY_NUMPAD_DOT		= 47;	// .
extern const int OSKEY_KANJI			= -1;	// 
extern const int OSKEY_HENKAN			= -1;	// 
extern const int OSKEY_MUHENKAN			= -1;	// 
extern const int OSKEY_ZENHAN			= -1;	// 
extern const int OSKEY_KATAHIRA			= -1;	// 
extern const int OSKEY_ROMAN			= -1;	// 
extern const int OSKEY_EISU				= -1;	// 
extern const int OSKEY_CLEAR			= 71;	// clear
extern const int OSKEY_F13				= 105;	// F13
extern const int OSKEY_F14				= 107;	// F14
extern const int OSKEY_F15				= 113;	// F15
extern const int OSKEY_F16				= 106;	// F16
extern const int OSKEY_F17				= 64;	// F17
extern const int OSKEY_F18				= 79;	// F18
extern const int OSKEY_F19				= 80;	// F19
extern const int OSKEY_FUNCTION			= 63;	// fn
extern const int OSKEY_META_LEFT		= 55;	// left command
extern const int OSKEY_META_RIGHT		= 54;	// right command
extern const int OSKEY_NUMPAD_ENTER		= 76;	// enter
extern const int OSKEY_NUMPAD_EQUALS	= 81;	// =

} /*Ribbon*/
