#include "pch.h"
#include "osandroid.h"

namespace Ribbon { namespace Android {

inline int StubFunctionToGetAddress() { return 0; }

class MemoryMappedFile :
	public IMemoryMappedFile,
	public std::enable_shared_from_this<MemoryMappedFile>
{
public:
	MemoryMappedFile(const char* fileName)
	{
		void *mappedAdr = nullptr;
		size_t fileSize = 0;
		int fd = -1;

		//Open file
		fd = open(fileName, O_RDONLY, 0);
		if (fd < 0) {
			return;
		}
		//Execute mmap
		fileSize = _GetFileSize(fileName);
		if (fileSize <= 0) {
			close(fd);
			return;
		}
		mappedAdr = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED /*| MAP_POPULATE*/, fd, 0);
		if (mappedAdr == MAP_FAILED) {
			close(fd);
			return;
		}

		m_mappedAdr = mappedAdr;
		m_fileSize = fileSize;
		m_fd = fd;
	}
	virtual ~MemoryMappedFile()
	{
		//Cleanup
		if (m_mappedAdr != nullptr) {
			/*int rc =*/ munmap(m_mappedAdr, m_fileSize);
			//assert(rc == 0);
		}
		if (m_fd >= 0) {
			close(m_fd);
		}
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
		if (stat(filename, &st) != 0) {
			return 0;
		}
		return st.st_size;
	}

private:
	void *m_mappedAdr = nullptr;
	size_t m_fileSize = 0;
	int m_fd = -1;
};


class PlatformAndroid :
	public IPlatform,
	public std::enable_shared_from_this<PlatformAndroid>
{
private:
	mutable std::weak_ptr<ISetting> m_globalSetting;
	mutable bool m_isConfigRead = false;
	mutable bool m_isOutputConsole = false;
	mutable bool m_isOutputDebugString = false;
	mutable std::string m_outputFile;

public:
	PlatformAndroid() {}
	virtual ~PlatformAndroid() {}

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
			return std::string("/data/data/net.millimo.android.ribbon.ImePreferences/app_data/");

		case PathType::BinPath:
			return std::string("/data/data/net.millimo.android.ribbon.ImePreferences/app_data/");

		case PathType::UserPath:
			return std::string("/data/data/net.millimo.android.ribbon.ImePreferences/app_data/");
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
static struct PlatformAndroidRegister {
	PlatformAndroidRegister() {
		s_platformHolder = std::make_shared<PlatformAndroid>();
		Platform = s_platformHolder.get();
	}
} s_PlatformAndroidRegister;

} /*Android*/

const int KEYCODE_UNKNOWN = 0x00000000;			// Key code constant: Unknown key code.
const int KEYCODE_SOFT_LEFT = 0x00000001;			// Key code constant: Soft Left key. Usually situated below the display on phones and used as a multi-function feature key for selecting a software defined function shown on the bottom left of the display.
const int KEYCODE_SOFT_RIGHT = 0x00000002;			// Key code constant: Soft Right key. Usually situated below the display on phones and used as a multi-function feature key for selecting a software defined function shown on the bottom right of the display.
const int KEYCODE_HOME = 0x00000003;			// Key code constant: Home key. This key is handled by the framework and is never delivered to applications.
const int KEYCODE_BACK = 0x00000004;			// Key code constant: Back key.
const int KEYCODE_CALL = 0x00000005;			// Key code constant: Call key.
const int KEYCODE_ENDCALL = 0x00000006;			// Key code constant: End Call key.
const int KEYCODE_0 = 0x00000007;			// Key code constant: '0' key.
const int KEYCODE_1 = 0x00000008;			// Key code constant: '1' key.
const int KEYCODE_2 = 0x00000009;			// Key code constant: '2' key.
const int KEYCODE_3 = 0x0000000a;			// Key code constant: '3' key.
const int KEYCODE_4 = 0x0000000b;			// Key code constant: '4' key.
const int KEYCODE_5 = 0x0000000c;			// Key code constant: '5' key.
const int KEYCODE_6 = 0x0000000d;			// Key code constant: '6' key.
const int KEYCODE_7 = 0x0000000e;			// Key code constant: '7' key.
const int KEYCODE_8 = 0x0000000f;			// Key code constant: '8' key.
const int KEYCODE_9 = 0x00000010;			// Key code constant: '9' key.
const int KEYCODE_STAR = 0x00000011;			// Key code constant: '*' key.
const int KEYCODE_POUND = 0x00000012;			// Key code constant: '#' key.
const int KEYCODE_DPAD_UP = 0x00000013;			// Key code constant: Directional Pad Up key. May also be synthesized from trackball motions.
const int KEYCODE_DPAD_DOWN = 0x00000014;			// Key code constant: Directional Pad Down key. May also be synthesized from trackball motions.
const int KEYCODE_DPAD_LEFT = 0x00000015;			// Key code constant: Directional Pad Left key. May also be synthesized from trackball motions.
const int KEYCODE_DPAD_RIGHT = 0x00000016;			// Key code constant: Directional Pad Right key. May also be synthesized from trackball motions.
const int KEYCODE_DPAD_CENTER = 0x00000017;			// Key code constant: Directional Pad Center key. May also be synthesized from trackball motions.
const int KEYCODE_VOLUME_UP = 0x00000018;			// Key code constant: Volume Up key. Adjusts the speaker volume up.
const int KEYCODE_VOLUME_DOWN = 0x00000019;			// Key code constant: Volume Down key. Adjusts the speaker volume down.
const int KEYCODE_POWER = 0x0000001a;			// Key code constant: Power key.
const int KEYCODE_CAMERA = 0x0000001b;			// Key code constant: Camera key. Used to launch a camera application or take pictures.
const int KEYCODE_CLEAR = 0x0000001c;			// Key code constant: Clear key.
const int KEYCODE_A = 0x0000001d;			// Key code constant: 'A' key.
const int KEYCODE_B = 0x0000001e;			// Key code constant: 'B' key.
const int KEYCODE_C = 0x0000001f;			// Key code constant: 'C' key.
const int KEYCODE_D = 0x00000020;			// Key code constant: 'D' key.
const int KEYCODE_E = 0x00000021;			// Key code constant: 'E' key.
const int KEYCODE_F = 0x00000022;			// Key code constant: 'F' key.
const int KEYCODE_G = 0x00000023;			// Key code constant: 'G' key.
const int KEYCODE_H = 0x00000024;			// Key code constant: 'H' key.
const int KEYCODE_I = 0x00000025;			// Key code constant: 'I' key.
const int KEYCODE_J = 0x00000026;			// Key code constant: 'J' key.
const int KEYCODE_K = 0x00000027;			// Key code constant: 'K' key.
const int KEYCODE_L = 0x00000028;			// Key code constant: 'L' key.
const int KEYCODE_M = 0x00000029;			// Key code constant: 'M' key.
const int KEYCODE_N = 0x0000002a;			// Key code constant: 'N' key.
const int KEYCODE_O = 0x0000002b;			// Key code constant: 'O' key.
const int KEYCODE_P = 0x0000002c;			// Key code constant: 'P' key.
const int KEYCODE_Q = 0x0000002d;			// Key code constant: 'Q' key.
const int KEYCODE_R = 0x0000002e;			// Key code constant: 'R' key.
const int KEYCODE_S = 0x0000002f;			// Key code constant: 'S' key.
const int KEYCODE_T = 0x00000030;			// Key code constant: 'T' key.
const int KEYCODE_U = 0x00000031;			// Key code constant: 'U' key.
const int KEYCODE_V = 0x00000032;			// Key code constant: 'V' key.
const int KEYCODE_W = 0x00000033;			// Key code constant: 'W' key.
const int KEYCODE_X = 0x00000034;			// Key code constant: 'X' key.
const int KEYCODE_Y = 0x00000035;			// Key code constant: 'Y' key.
const int KEYCODE_Z = 0x00000036;			// Key code constant: 'Z' key.
const int KEYCODE_COMMA = 0x00000037;			// Key code constant: ',' key.
const int KEYCODE_PERIOD = 0x00000038;			// Key code constant: '.' key.
const int KEYCODE_ALT_LEFT = 0x00000039;			// Key code constant: Left Alt modifier key.
const int KEYCODE_ALT_RIGHT = 0x0000003a;			// Key code constant: Right Alt modifier key.
const int KEYCODE_SHIFT_LEFT = 0x0000003b;			// Key code constant: Left Shift modifier key.
const int KEYCODE_SHIFT_RIGHT = 0x0000003c;			// Key code constant: Right Shift modifier key.
const int KEYCODE_TAB = 0x0000003d;			// Key code constant: Tab key.
const int KEYCODE_SPACE = 0x0000003e;			// Key code constant: Space key.
const int KEYCODE_SYM = 0x0000003f;			// Key code constant: Symbol modifier key. Used to enter alternate symbols.
const int KEYCODE_EXPLORER = 0x00000040;			// Key code constant: Explorer special function key. Used to launch a browser application.
const int KEYCODE_ENVELOPE = 0x00000041;			// Key code constant: Envelope special function key. Used to launch a mail application.
const int KEYCODE_ENTER = 0x00000042;			// Key code constant: Enter key.
const int KEYCODE_DEL = 0x00000043;			// Key code constant: Backspace key. Deletes characters before the insertion point, unlike KEYCODE_FORWARD_DEL.
const int KEYCODE_GRAVE = 0x00000044;			// Key code constant: '`' (backtick) key.
const int KEYCODE_MINUS = 0x00000045;			// Key code constant: '-'.
const int KEYCODE_EQUALS = 0x00000046;			// Key code constant: '=' key.
const int KEYCODE_LEFT_BRACKET = 0x00000047;			// Key code constant: '[' key.
const int KEYCODE_RIGHT_BRACKET = 0x00000048;			// Key code constant: ']' key.
const int KEYCODE_BACKSLASH = 0x00000049;			// Key code constant: '\' key.
const int KEYCODE_SEMICOLON = 0x0000004a;			// Key code constant: ';' key.
const int KEYCODE_APOSTROPHE = 0x0000004b;			// Key code constant: ''' (apostrophe) key.
const int KEYCODE_SLASH = 0x0000004c;			// Key code constant: '/' key.
const int KEYCODE_AT = 0x0000004d;			// Key code constant: '@' key.
const int KEYCODE_NUM = 0x0000004e;			// Key code constant: Number modifier key. Used to enter numeric symbols. This key is not Num Lock; it is more like KEYCODE_ALT_LEFT and is interpreted as an ALT key by MetaKeyKeyListener.
const int KEYCODE_HEADSETHOOK = 0x0000004f;			// Key code constant: Headset Hook key. Used to hang up calls and stop media.
const int KEYCODE_FOCUS = 0x00000050;			// Key code constant: Camera Focus key. Used to focus the camera.
const int KEYCODE_PLUS = 0x00000051;			// Key code constant: '+' key.
const int KEYCODE_MENU = 0x00000052;			// Key code constant: Menu key.
const int KEYCODE_NOTIFICATION = 0x00000053;			// Key code constant: Notification key.
const int KEYCODE_SEARCH = 0x00000054;			// Key code constant: Search key.
const int KEYCODE_MEDIA_PLAY_PAUSE = 0x00000055;			// Key code constant: Play/Pause media key.
const int KEYCODE_MEDIA_STOP = 0x00000056;			// Key code constant: Stop media key.
const int KEYCODE_MEDIA_NEXT = 0x00000057;			// Key code constant: Play Next media key.
const int KEYCODE_MEDIA_PREVIOUS = 0x00000058;			// Key code constant: Play Previous media key.
const int KEYCODE_MEDIA_REWIND = 0x00000059;			// Key code constant: Rewind media key.
const int KEYCODE_MEDIA_FAST_FORWARD = 0x0000005a;			// Key code constant: Fast Forward media key.
const int KEYCODE_MUTE = 0x0000005b;			// Key code constant: Mute key. Mutes the microphone, unlike KEYCODE_VOLUME_MUTE.
const int KEYCODE_PAGE_UP = 0x0000005c;			// Key code constant: Page Up key.
const int KEYCODE_PAGE_DOWN = 0x0000005d;			// Key code constant: Page Down key.
const int KEYCODE_PICTSYMBOLS = 0x0000005e;			// Key code constant: Picture Symbols modifier key. Used to switch symbol sets (Emoji, Kao-moji).
const int KEYCODE_SWITCH_CHARSET = 0x0000005f;			// Key code constant: Switch Charset modifier key. Used to switch character sets (Kanji, Katakana).
const int KEYCODE_BUTTON_A = 0x00000060;			// Key code constant: A Button key. On a game controller, the A button should be either the button labeled A or the first button on the bottom row of controller buttons.
const int KEYCODE_BUTTON_B = 0x00000061;			// Key code constant: B Button key. On a game controller, the B button should be either the button labeled B or the second button on the bottom row of controller buttons.
const int KEYCODE_BUTTON_C = 0x00000062;			// Key code constant: C Button key. On a game controller, the C button should be either the button labeled C or the third button on the bottom row of controller buttons.
const int KEYCODE_BUTTON_X = 0x00000063;			// Key code constant: X Button key. On a game controller, the X button should be either the button labeled X or the first button on the upper row of controller buttons.
const int KEYCODE_BUTTON_Y = 0x00000064;			// Key code constant: Y Button key. On a game controller, the Y button should be either the button labeled Y or the second button on the upper row of controller buttons.
const int KEYCODE_BUTTON_Z = 0x00000065;			// Key code constant: Z Button key. On a game controller, the Z button should be either the button labeled Z or the third button on the upper row of controller buttons.
const int KEYCODE_BUTTON_L1 = 0x00000066;			// Key code constant: L1 Button key. On a game controller, the L1 button should be either the button labeled L1 (or L) or the top left trigger button.
const int KEYCODE_BUTTON_R1 = 0x00000067;			// Key code constant: R1 Button key. On a game controller, the R1 button should be either the button labeled R1 (or R) or the top right trigger button.
const int KEYCODE_BUTTON_L2 = 0x00000068;			// Key code constant: L2 Button key. On a game controller, the L2 button should be either the button labeled L2 or the bottom left trigger button.
const int KEYCODE_BUTTON_R2 = 0x00000069;			// Key code constant: R2 Button key. On a game controller, the R2 button should be either the button labeled R2 or the bottom right trigger button.
const int KEYCODE_BUTTON_THUMBL = 0x0000006a;			// Key code constant: Left Thumb Button key. On a game controller, the left thumb button indicates that the left (or only) joystick is pressed.
const int KEYCODE_BUTTON_THUMBR = 0x0000006b;			// Key code constant: Right Thumb Button key. On a game controller, the right thumb button indicates that the right joystick is pressed.
const int KEYCODE_BUTTON_START = 0x0000006c;			// Key code constant: Start Button key. On a game controller, the button labeled Start.
const int KEYCODE_BUTTON_SELECT = 0x0000006d;			// Key code constant: Select Button key. On a game controller, the button labeled Select.
const int KEYCODE_BUTTON_MODE = 0x0000006e;			// Key code constant: Mode Button key. On a game controller, the button labeled Mode.
const int KEYCODE_ESCAPE = 0x0000006f;			// Key code constant: Escape key.
const int KEYCODE_FORWARD_DEL = 0x00000070;			// Key code constant: Forward Delete key. Deletes characters ahead of the insertion point, unlike KEYCODE_DEL.
const int KEYCODE_CTRL_LEFT = 0x00000071;			// Key code constant: Left Control modifier key.
const int KEYCODE_CTRL_RIGHT = 0x00000072;			// Key code constant: Right Control modifier key.
const int KEYCODE_CAPS_LOCK = 0x00000073;			// Key code constant: Caps Lock key.
const int KEYCODE_SCROLL_LOCK = 0x00000074;			// Key code constant: Scroll Lock key.
const int KEYCODE_META_LEFT = 0x00000075;			// Key code constant: Left Meta modifier key.
const int KEYCODE_META_RIGHT = 0x00000076;			// Key code constant: Right Meta modifier key.
const int KEYCODE_FUNCTION = 0x00000077;			// Key code constant: Function modifier key.
const int KEYCODE_SYSRQ = 0x00000078;			// Key code constant: System Request / Print Screen key.
const int KEYCODE_BREAK = 0x00000079;			// Key code constant: Break / Pause key.
const int KEYCODE_MOVE_HOME = 0x0000007a;			// Key code constant: Home Movement key. Used for scrolling or moving the cursor around to the start of a line or to the top of a list.
const int KEYCODE_MOVE_END = 0x0000007b;			// Key code constant: End Movement key. Used for scrolling or moving the cursor around to the end of a line or to the bottom of a list.
const int KEYCODE_INSERT = 0x0000007c;			// Key code constant: Insert key. Toggles insert / overwrite edit mode.
const int KEYCODE_FORWARD = 0x0000007d;			// Key code constant: Forward key. Navigates forward in the history stack. Complement of KEYCODE_BACK.
const int KEYCODE_MEDIA_PLAY = 0x0000007e;			// Key code constant: Play media key.
const int KEYCODE_MEDIA_PAUSE = 0x0000007f;			// Key code constant: Pause media key.
const int KEYCODE_MEDIA_CLOSE = 0x00000080;			// Key code constant: Close media key. May be used to close a CD tray, for example.
const int KEYCODE_MEDIA_EJECT = 0x00000081;			// Key code constant: Eject media key. May be used to eject a CD tray, for example.
const int KEYCODE_MEDIA_RECORD = 0x00000082;			// Key code constant: Record media key.
const int KEYCODE_F1 = 0x00000083;			// Key code constant: F1 key.
const int KEYCODE_F2 = 0x00000084;			// Key code constant: F2 key.
const int KEYCODE_F3 = 0x00000085;			// Key code constant: F3 key.
const int KEYCODE_F4 = 0x00000086;			// Key code constant: F4 key.
const int KEYCODE_F5 = 0x00000087;			// Key code constant: F5 key.
const int KEYCODE_F6 = 0x00000088;			// Key code constant: F6 key.
const int KEYCODE_F7 = 0x00000089;			// Key code constant: F7 key.
const int KEYCODE_F8 = 0x0000008a;			// Key code constant: F8 key.
const int KEYCODE_F9 = 0x0000008b;			// Key code constant: F9 key.
const int KEYCODE_F10 = 0x0000008c;			// Key code constant: F10 key.
const int KEYCODE_F11 = 0x0000008d;			// Key code constant: F11 key.
const int KEYCODE_F12 = 0x0000008e;			// Key code constant: F12 key.
const int KEYCODE_NUM_LOCK = 0x0000008f;			// Key code constant: Num Lock key. This is the Num Lock key; it is different from KEYCODE_NUM. This key alters the behavior of other keys on the numeric keypad.
const int KEYCODE_NUMPAD_0 = 0x00000090;			// Key code constant: Numeric keypad '0' key.
const int KEYCODE_NUMPAD_1 = 0x00000091;			// Key code constant: Numeric keypad '1' key.
const int KEYCODE_NUMPAD_2 = 0x00000092;			// Key code constant: Numeric keypad '2' key.
const int KEYCODE_NUMPAD_3 = 0x00000093;			// Key code constant: Numeric keypad '3' key.
const int KEYCODE_NUMPAD_4 = 0x00000094;			// Key code constant: Numeric keypad '4' key.
const int KEYCODE_NUMPAD_5 = 0x00000095;			// Key code constant: Numeric keypad '5' key.
const int KEYCODE_NUMPAD_6 = 0x00000096;			// Key code constant: Numeric keypad '6' key.
const int KEYCODE_NUMPAD_7 = 0x00000097;			// Key code constant: Numeric keypad '7' key.
const int KEYCODE_NUMPAD_8 = 0x00000098;			// Key code constant: Numeric keypad '8' key.
const int KEYCODE_NUMPAD_9 = 0x00000099;			// Key code constant: Numeric keypad '9' key.
const int KEYCODE_NUMPAD_DIVIDE = 0x0000009a;			// Key code constant: Numeric keypad '/' key (for division).
const int KEYCODE_NUMPAD_MULTIPLY = 0x0000009b;			// Key code constant: Numeric keypad '*' key (for multiplication).
const int KEYCODE_NUMPAD_SUBTRACT = 0x0000009c;			// Key code constant: Numeric keypad '-' key (for subtraction).
const int KEYCODE_NUMPAD_ADD = 0x0000009d;			// Key code constant: Numeric keypad '+' key (for addition).
const int KEYCODE_NUMPAD_DOT = 0x0000009e;			// Key code constant: Numeric keypad '.' key (for decimals or digit grouping).
const int KEYCODE_NUMPAD_COMMA = 0x0000009f;			// Key code constant: Numeric keypad ',' key (for decimals or digit grouping).
const int KEYCODE_NUMPAD_ENTER = 0x000000a0;			// Key code constant: Numeric keypad Enter key.
const int KEYCODE_NUMPAD_EQUALS = 0x000000a1;			// Key code constant: Numeric keypad '=' key.
const int KEYCODE_NUMPAD_LEFT_PAREN = 0x000000a2;			// Key code constant: Numeric keypad '(' key.
const int KEYCODE_NUMPAD_RIGHT_PAREN = 0x000000a3;			// Key code constant: Numeric keypad ')' key.
const int KEYCODE_VOLUME_MUTE = 0x000000a4;			// Key code constant: Volume Mute key. Mutes the speaker, unlike KEYCODE_MUTE. This key should normally be implemented as a toggle such that the first press mutes the speaker and the second press restores the original volume.
const int KEYCODE_INFO = 0x000000a5;			// Key code constant: Info key. Common on TV remotes to show additional information related to what is currently being viewed.
const int KEYCODE_CHANNEL_UP = 0x000000a6;			// Key code constant: Channel up key. On TV remotes, increments the television channel.
const int KEYCODE_CHANNEL_DOWN = 0x000000a7;			// Key code constant: Channel down key. On TV remotes, decrements the television channel.
const int KEYCODE_ZOOM_IN = 0x000000a8;			// Key code constant: Zoom in key.
const int KEYCODE_ZOOM_OUT = 0x000000a9;			// Key code constant: Zoom out key.
const int KEYCODE_TV = 0x000000aa;			// Key code constant: TV key. On TV remotes, switches to viewing live TV.
const int KEYCODE_WINDOW = 0x000000ab;			// Key code constant: Window key. On TV remotes, toggles picture-in-picture mode or other windowing functions. On Android Wear devices, triggers a display offset.
const int KEYCODE_GUIDE = 0x000000ac;			// Key code constant: Guide key. On TV remotes, shows a programming guide.
const int KEYCODE_DVR = 0x000000ad;			// Key code constant: DVR key. On some TV remotes, switches to a DVR mode for recorded shows.
const int KEYCODE_BOOKMARK = 0x000000ae;			// Key code constant: Bookmark key. On some TV remotes, bookmarks content or web pages.
const int KEYCODE_CAPTIONS = 0x000000af;			// Key code constant: Toggle captions key. Switches the mode for closed-captioning text, for example during television shows.
const int KEYCODE_SETTINGS = 0x000000b0;			// Key code constant: Settings key. Starts the system settings activity.
const int KEYCODE_TV_POWER = 0x000000b1;			// Key code constant: TV power key. On TV remotes, toggles the power on a television screen.
const int KEYCODE_TV_INPUT = 0x000000b2;			// Key code constant: TV input key. On TV remotes, switches the input on a television screen.
const int KEYCODE_STB_POWER = 0x000000b3;			// Key code constant: Set-top-box power key. On TV remotes, toggles the power on an external Set-top-box.
const int KEYCODE_STB_INPUT = 0x000000b4;			// Key code constant: Set-top-box input key. On TV remotes, switches the input mode on an external Set-top-box.
const int KEYCODE_AVR_POWER = 0x000000b5;			// Key code constant: A/V Receiver power key. On TV remotes, toggles the power on an external A/V Receiver.
const int KEYCODE_AVR_INPUT = 0x000000b6;			// Key code constant: A/V Receiver input key. On TV remotes, switches the input mode on an external A/V Receiver.
const int KEYCODE_PROG_RED = 0x000000b7;			// Key code constant: Red "programmable" key. On TV remotes, acts as a contextual/programmable key.
const int KEYCODE_PROG_GREEN = 0x000000b8;			// Key code constant: Green "programmable" key. On TV remotes, actsas a contextual/programmable key.
const int KEYCODE_PROG_YELLOW = 0x000000b9;			// Key code constant: Yellow "programmable" key. On TV remotes, acts as a contextual/programmable key.
const int KEYCODE_PROG_BLUE = 0x000000ba;			// Key code constant: Blue "programmable" key. On TV remotes, acts as a contextual/programmable key.
const int KEYCODE_APP_SWITCH = 0x000000bb;			// Key code constant: App switch key. Should bring up the application switcher dialog.
const int KEYCODE_BUTTON_1 = 0x000000bc;			// Key code constant: Generic Game Pad Button #1.
const int KEYCODE_BUTTON_2 = 0x000000bd;			// Key code constant: Generic Game Pad Button #2.
const int KEYCODE_BUTTON_3 = 0x000000be;			// Key code constant: Generic Game Pad Button #3.
const int KEYCODE_BUTTON_4 = 0x000000bf;			// Key code constant: Generic Game Pad Button #4.
const int KEYCODE_BUTTON_5 = 0x000000c0;			// Key code constant: Generic Game Pad Button #5.
const int KEYCODE_BUTTON_6 = 0x000000c1;			// Key code constant: Generic Game Pad Button #6.
const int KEYCODE_BUTTON_7 = 0x000000c2;			// Key code constant: Generic Game Pad Button #7.
const int KEYCODE_BUTTON_8 = 0x000000c3;			// Key code constant: Generic Game Pad Button #8.
const int KEYCODE_BUTTON_9 = 0x000000c4;			// Key code constant: Generic Game Pad Button #9.
const int KEYCODE_BUTTON_10 = 0x000000c5;			// Key code constant: Generic Game Pad Button #10.
const int KEYCODE_BUTTON_11 = 0x000000c6;			// Key code constant: Generic Game Pad Button #11.
const int KEYCODE_BUTTON_12 = 0x000000c7;			// Key code constant: Generic Game Pad Button #12.
const int KEYCODE_BUTTON_13 = 0x000000c8;			// Key code constant: Generic Game Pad Button #13.
const int KEYCODE_BUTTON_14 = 0x000000c9;			// Key code constant: Generic Game Pad Button #14.
const int KEYCODE_BUTTON_15 = 0x000000ca;			// Key code constant: Generic Game Pad Button #15.
const int KEYCODE_BUTTON_16 = 0x000000cb;			// Key code constant: Generic Game Pad Button #16.
const int KEYCODE_LANGUAGE_SWITCH = 0x000000cc;			// Key code constant: Language Switch key. Toggles the current input language such as switching between English and Japanese on a QWERTY keyboard. On some devices, the same function may be performed by pressing Shift+Spacebar.
const int KEYCODE_MANNER_MODE = 0x000000cd;			// Key code constant: Manner Mode key. Toggles silent or vibrate mode on and off to make the device behave more politely in certain settings such as on a crowded train. On some devices, the key may only operate when long-pressed.
const int KEYCODE_3D_MODE = 0x000000ce;			// Key code constant: 3D Mode key. Toggles the display between 2D and 3D mode.
const int KEYCODE_CONTACTS = 0x000000cf;			// Key code constant: Contacts special function key. Used to launch an address book application.
const int KEYCODE_CALENDAR = 0x000000d0;			// Key code constant: Calendar special function key. Used to launch a calendar application.
const int KEYCODE_MUSIC = 0x000000d1;			// Key code constant: Music special function key. Used to launch a music player application.
const int KEYCODE_CALCULATOR = 0x000000d2;			// Key code constant: Calculator special function key. Used to launch a calculator application.
const int KEYCODE_ZENKAKU_HANKAKU = 0x000000d3;			// Key code constant: Japanese full-width / half-width key.
const int KEYCODE_EISU = 0x000000d4;			// Key code constant: Japanese alphanumeric key.
const int KEYCODE_MUHENKAN = 0x000000d5;			// Key code constant: Japanese non-conversion key.
const int KEYCODE_HENKAN = 0x000000d6;			// Key code constant: Japanese conversion key.
const int KEYCODE_KATAKANA_HIRAGANA = 0x000000d7;			// Key code constant: Japanese katakana / hiragana key.
const int KEYCODE_YEN = 0x000000d8;			// Key code constant: Japanese Yen key.
const int KEYCODE_RO = 0x000000d9;			// Key code constant: Japanese Ro key.
const int KEYCODE_KANA = 0x000000da;			// Key code constant: Japanese kana key.
const int KEYCODE_ASSIST = 0x000000db;			// Key code constant: Assist key. Launches the global assist activity. Not delivered to applications.
const int KEYCODE_BRIGHTNESS_DOWN = 0x000000dc;			// Key code constant: Brightness Down key. Adjusts the screen brightness down.
const int KEYCODE_BRIGHTNESS_UP = 0x000000dd;			// Key code constant: Brightness Up key. Adjusts the screen brightness up.
const int KEYCODE_MEDIA_AUDIO_TRACK = 0x000000de;			// Key code constant: Audio Track key. Switches the audio tracks.
const int KEYCODE_SLEEP = 0x000000df;			// Key code constant: Sleep key. Puts the device to sleep. Behaves somewhat like KEYCODE_POWER but it has no effect if the device is already asleep.
const int KEYCODE_WAKEUP = 0x000000e0;			// Key code constant: Wakeup key. Wakes up the device. Behaves somewhat like KEYCODE_POWER but it has no effect if the device is already awake.
const int KEYCODE_PAIRING = 0x000000e1;			// Key code constant: Pairing key. Initiates peripheral pairing mode. Useful for pairing remote control devices or game controllers, especially if no other input mode is available.
const int KEYCODE_MEDIA_TOP_MENU = 0x000000e2;			// Key code constant: Media Top Menu key. Goes to the top of media menu.
const int KEYCODE_11 = 0x000000e3;			// Key code constant: '11' key.
const int KEYCODE_12 = 0x000000e4;			// Key code constant: '12' key.
const int KEYCODE_LAST_CHANNEL = 0x000000e5;			// Key code constant: Last Channel key. Goes to the last viewed channel.
const int KEYCODE_TV_DATA_SERVICE = 0x000000e6;			// Key code constant: TV data service key. Displays data services like weather, sports.
const int KEYCODE_VOICE_ASSIST = 0x000000e7;			// Key code constant: Voice Assist key. Launches the global voice assist activity. Not delivered to applications.
const int KEYCODE_TV_RADIO_SERVICE = 0x000000e8;			// Key code constant: Radio key. Toggles TV service / Radio service.
const int KEYCODE_TV_TELETEXT = 0x000000e9;			// Key code constant: Teletext key. Displays Teletext service.
const int KEYCODE_TV_NUMBER_ENTRY = 0x000000ea;			// Key code constant: Number entry key. Initiates to enter multi-digit channel nubmber when each digit key is assigned for selecting separate channel. Corresponds to Number Entry Mode (0x1D) of CEC User Control Code.
const int KEYCODE_TV_TERRESTRIAL_ANALOG = 0x000000eb;			// Key code constant: Analog Terrestrial key. Switches to analog terrestrial broadcast service.
const int KEYCODE_TV_TERRESTRIAL_DIGITAL = 0x000000ec;			// Key code constant: Digital Terrestrial key. Switches to digital terrestrial broadcast service.
const int KEYCODE_TV_SATELLITE = 0x000000ed;			// Key code constant: Satellite key. Switches to digital satellite broadcast service.
const int KEYCODE_TV_SATELLITE_BS = 0x000000ee;			// Key code constant: BS key. Switches to BS digital satellite broadcasting service available in Japan.
const int KEYCODE_TV_SATELLITE_CS = 0x000000ef;			// Key code constant: CS key. Switches to CS digital satellite broadcasting service available in Japan.
const int KEYCODE_TV_SATELLITE_SERVICE = 0x000000f0;			// Key code constant: BS/CS key. Toggles between BS and CS digital satellite services.
const int KEYCODE_TV_NETWORK = 0x000000f1;			// Key code constant: Toggle Network key. Toggles selecting broacast services.
const int KEYCODE_TV_ANTENNA_CABLE = 0x000000f2;			// Key code constant: Antenna/Cable key. Toggles broadcast input source between antenna and cable.
const int KEYCODE_TV_INPUT_HDMI_1 = 0x000000f3;			// Key code constant: HDMI #1 key. Switches to HDMI input #1.
const int KEYCODE_TV_INPUT_HDMI_2 = 0x000000f4;			// Key code constant: HDMI #2 key. Switches to HDMI input #2.
const int KEYCODE_TV_INPUT_HDMI_3 = 0x000000f5;			// Key code constant: HDMI #3 key. Switches to HDMI input #3.
const int KEYCODE_TV_INPUT_HDMI_4 = 0x000000f6;			// Key code constant: HDMI #4 key. Switches to HDMI input #4.
const int KEYCODE_TV_INPUT_COMPOSITE_1 = 0x000000f7;			// Key code constant: Composite #1 key. Switches to composite video input #1.
const int KEYCODE_TV_INPUT_COMPOSITE_2 = 0x000000f8;			// Key code constant: Composite #2 key. Switches to composite video input #2.
const int KEYCODE_TV_INPUT_COMPONENT_1 = 0x000000f9;			// Key code constant: Component #1 key. Switches to component video input #1.
const int KEYCODE_TV_INPUT_COMPONENT_2 = 0x000000fa;			// Key code constant: Component #2 key. Switches to component video input #2.
const int KEYCODE_TV_INPUT_VGA_1 = 0x000000fb;			// Key code constant: VGA #1 key. Switches to VGA (analog RGB) input #1.
const int KEYCODE_TV_AUDIO_DESCRIPTION = 0x000000fc;			// Key code constant: Audio description key. Toggles audio description off / on.
const int KEYCODE_TV_AUDIO_DESCRIPTION_MIX_UP = 0x000000fd;			// Key code constant: Audio description mixing volume up key. Louden audio description volume as compared with normal audio volume.
const int KEYCODE_TV_AUDIO_DESCRIPTION_MIX_DOWN = 0x000000fe;			// Key code constant: Audio description mixing volume down key. Lessen audio description volume as compared with normal audio volume.
const int KEYCODE_TV_ZOOM_MODE = 0x000000ff;			// Key code constant: Zoom mode key. Changes Zoom mode (Normal, Full, Zoom, Wide-zoom, etc.)
const int KEYCODE_TV_CONTENTS_MENU = 0x00000100;			// Key code constant: Contents menu key. Goes to the title list. Corresponds to Contents Menu (0x0B) of CEC User Control Code
const int KEYCODE_TV_MEDIA_CONTEXT_MENU = 0x00000101;			// Key code constant: Media context menu key. Goes to the context menu of media contents. Corresponds to Media Context-sensitive Menu (0x11) of CEC User Control Code.
const int KEYCODE_TV_TIMER_PROGRAMMING = 0x00000102;			// Key code constant: Timer programming key. Goes to the timer recording menu. Corresponds to Timer Programming (0x54) of CEC User Control Code.
const int KEYCODE_HELP = 0x00000103;			// Key code constant: Help key.
const int KEYCODE_NAVIGATE_PREVIOUS = 0x00000104;			// Key code constant: Navigate to previous key. Goes backward by one item in an ordered collection of items.
const int KEYCODE_NAVIGATE_NEXT = 0x00000105;			// Key code constant: Navigate to next key. Advances to the next item in an ordered collection of items.
const int KEYCODE_NAVIGATE_IN = 0x00000106;			// Key code constant: Navigate in key. Activates the item that currently has focus or expands to the next level of a navigation hierarchy.
const int KEYCODE_NAVIGATE_OUT = 0x00000107;			// Key code constant: Navigate out key. Backs out one level of a navigation hierarchy or collapses the item that currently has focus.
const int KEYCODE_STEM_PRIMARY = 0x00000108;			// Key code constant: Primary stem key for Wear Main power/reset button on watch.
const int KEYCODE_STEM_1 = 0x00000109;			// Key code constant: Generic stem key 1 for Wear
const int KEYCODE_STEM_2 = 0x0000010a;			// Key code constant: Generic stem key 2 for Wear
const int KEYCODE_STEM_3 = 0x0000010b;			// Key code constant: Generic stem key 3 for Wear
const int KEYCODE_DPAD_UP_LEFT = 0x0000010c;			// Key code constant: Directional Pad Up-Left
const int KEYCODE_DPAD_DOWN_LEFT = 0x0000010d;			// Key code constant: Directional Pad Down-Left
const int KEYCODE_DPAD_UP_RIGHT = 0x0000010e;			// Key code constant: Directional Pad Up-Right
const int KEYCODE_DPAD_DOWN_RIGHT = 0x0000010f;			// Key code constant: Directional Pad Down-Right
const int KEYCODE_MEDIA_SKIP_FORWARD = 0x00000110;			// Key code constant: Skip forward media key.
const int KEYCODE_MEDIA_SKIP_BACKWARD = 0x00000111;			// Key code constant: Skip backward media key.
const int KEYCODE_MEDIA_STEP_FORWARD = 0x00000112;			// Key code constant: Step forward media key. Steps media forward, one frame at a time.
const int KEYCODE_MEDIA_STEP_BACKWARD = 0x00000113;			// Key code constant: Step backward media key. Steps media backward, one frame at a time.
const int KEYCODE_SOFT_SLEEP = 0x00000114;			// Key code constant: put device to sleep unless a wakelock is held.
const int KEYCODE_CUT = 0x00000115;			// Key code constant: Cut key.
const int KEYCODE_COPY = 0x00000116;			// Key code constant: Copy key.
const int KEYCODE_PASTE = 0x00000117;			// Key code constant: Paste key.
const int KEYCODE_SYSTEM_NAVIGATION_UP = 0x00000118;			// Key code constant: Consumed by the system for navigation up
const int KEYCODE_SYSTEM_NAVIGATION_DOWN = 0x00000119;			// Key code constant: Consumed by the system for navigation down
const int KEYCODE_SYSTEM_NAVIGATION_LEFT = 0x0000011a;			// Key code constant: Consumed by the system for navigation left
const int KEYCODE_SYSTEM_NAVIGATION_RIGHT = 0x0000011b;			// Key code constant: Consumed by the system for navigation right


extern const int OSKEY_ESCAPE = KEYCODE_ESCAPE;						// Esc
extern const int OSKEY_F1 = KEYCODE_F1;								// F1
extern const int OSKEY_F2 = KEYCODE_F2;								// F2
extern const int OSKEY_F3 = KEYCODE_F3;								// F3
extern const int OSKEY_F4 = KEYCODE_F4;								// F4
extern const int OSKEY_F5 = KEYCODE_F5;								// F5
extern const int OSKEY_F6 = KEYCODE_F6;								// F6
extern const int OSKEY_F7 = KEYCODE_F7;								// F7
extern const int OSKEY_F8 = KEYCODE_F8;								// F8
extern const int OSKEY_F9 = KEYCODE_F9;								// F9
extern const int OSKEY_F10 = KEYCODE_F10;							// F10
extern const int OSKEY_F11 = KEYCODE_F11;							// F11
extern const int OSKEY_F12 = KEYCODE_F12;							// F12
extern const int OSKEY_GRAVE = KEYCODE_GRAVE;						// `
extern const int OSKEY_1 = KEYCODE_1;								// 1
extern const int OSKEY_2 = KEYCODE_2;								// 2
extern const int OSKEY_3 = KEYCODE_3;								// 3
extern const int OSKEY_4 = KEYCODE_4;								// 4
extern const int OSKEY_5 = KEYCODE_5;								// 5
extern const int OSKEY_6 = KEYCODE_6;								// 6
extern const int OSKEY_7 = KEYCODE_7;								// 7
extern const int OSKEY_8 = KEYCODE_8;								// 8
extern const int OSKEY_9 = KEYCODE_9;								// 9
extern const int OSKEY_0 = KEYCODE_0;								// 0
extern const int OSKEY_MINUS = KEYCODE_MINUS;						// -
extern const int OSKEY_PLUS = KEYCODE_PLUS;							// :
extern const int OSKEY_BACK = KEYCODE_DEL;							// BackSpace
extern const int OSKEY_TAB = KEYCODE_TAB;							// Tab
extern const int OSKEY_Q = KEYCODE_Q;								// Q
extern const int OSKEY_W = KEYCODE_W;								// W
extern const int OSKEY_E = KEYCODE_E;								// E
extern const int OSKEY_R = KEYCODE_R;								// R
extern const int OSKEY_T = KEYCODE_T;								// T
extern const int OSKEY_Y = KEYCODE_Y;								// Y
extern const int OSKEY_U = KEYCODE_U;								// U
extern const int OSKEY_I = KEYCODE_I;								// I
extern const int OSKEY_O = KEYCODE_O;								// O
extern const int OSKEY_P = KEYCODE_P;								// P
extern const int OSKEY_LBRACKET = KEYCODE_LEFT_BRACKET;				// [
extern const int OSKEY_RBRACKET = KEYCODE_RIGHT_BRACKET;			// ]
extern const int OSKEY_BACKSLASH = KEYCODE_BACKSLASH;				// [\]
extern const int OSKEY_CAPSLOCK = KEYCODE_CAPS_LOCK;				// CapsLock
extern const int OSKEY_A = KEYCODE_A;								// A
extern const int OSKEY_S = KEYCODE_S;								// S
extern const int OSKEY_D = KEYCODE_D;								// D
extern const int OSKEY_F = KEYCODE_F;								// F
extern const int OSKEY_G = KEYCODE_G;								// G
extern const int OSKEY_H = KEYCODE_H;								// H
extern const int OSKEY_J = KEYCODE_J;								// J
extern const int OSKEY_K = KEYCODE_K;								// K
extern const int OSKEY_L = KEYCODE_L;								// L
extern const int OSKEY_SEMICOLON = KEYCODE_SEMICOLON;				// ;
extern const int OSKEY_APOSTROPHE = KEYCODE_APOSTROPHE;				// ^
extern const int OSKEY_ENTER = KEYCODE_ENTER;						// Enter
extern const int OSKEY_LSHIFT = KEYCODE_SHIFT_LEFT;					// Shift
extern const int OSKEY_Z = KEYCODE_Z;								// Z
extern const int OSKEY_X = KEYCODE_X;								// X
extern const int OSKEY_C = KEYCODE_C;								// C
extern const int OSKEY_V = KEYCODE_V;								// V
extern const int OSKEY_B = KEYCODE_B;								// B
extern const int OSKEY_N = KEYCODE_N;								// N
extern const int OSKEY_M = KEYCODE_M;								// M
extern const int OSKEY_COMMA = KEYCODE_COMMA;						// ,
extern const int OSKEY_PERIOD = KEYCODE_PERIOD;						// .
extern const int OSKEY_SLASH = KEYCODE_SLASH;						// /
extern const int OSKEY_RSHIFT = KEYCODE_SHIFT_RIGHT;				// Shift
extern const int OSKEY_LCTRL = KEYCODE_CTRL_LEFT;					// Ctrl
extern const int OSKEY_LWIN = -1;									// L-Windows
extern const int OSKEY_LALT = KEYCODE_ALT_LEFT;						// Alt
extern const int OSKEY_SPACE = KEYCODE_SPACE;						// Space
extern const int OSKEY_RALT = KEYCODE_ALT_RIGHT;					// Alt
extern const int OSKEY_RWIN = -1;									// R-Windows
extern const int OSKEY_MENU = KEYCODE_MENU;							// ApplicationMenu
extern const int OSKEY_RCTRL = KEYCODE_CTRL_RIGHT;					// Ctrl
extern const int OSKEY_SYSRQ = KEYCODE_SYSRQ;						// PrintScreen
extern const int OSKEY_SCROLL = KEYCODE_SCROLL_LOCK;				// ScrollLock
extern const int OSKEY_BREAK = KEYCODE_BREAK;						// Pause
extern const int OSKEY_INSERT = KEYCODE_INSERT;						// Insert
extern const int OSKEY_HOME = KEYCODE_MOVE_HOME;					// Home
extern const int OSKEY_PAGEUP = KEYCODE_PAGE_UP;					// PageUp
extern const int OSKEY_DELETE = KEYCODE_FORWARD_DEL;				// Delete
extern const int OSKEY_END = KEYCODE_MOVE_END;						// End
extern const int OSKEY_PAGEDOWN = KEYCODE_PAGE_DOWN;				// PageDown
extern const int OSKEY_UP = KEYCODE_DPAD_UP;						// Up
extern const int OSKEY_LEFT = KEYCODE_DPAD_LEFT;					// <-
extern const int OSKEY_DOWN = KEYCODE_DPAD_DOWN;					// Down
extern const int OSKEY_RIGHT = KEYCODE_DPAD_RIGHT;					// ->
extern const int OSKEY_NUMLOCK = KEYCODE_NUM_LOCK;					// NumLock
extern const int OSKEY_NUMPAD_DIVIDE = KEYCODE_NUMPAD_DIVIDE;		// /
extern const int OSKEY_NUMPAD_MULTIPLY = KEYCODE_NUMPAD_MULTIPLY;	// *
extern const int OSKEY_NUMPAD_SUBTRACT = KEYCODE_NUMPAD_SUBTRACT;	// -
extern const int OSKEY_NUMPAD_7 = KEYCODE_NUMPAD_7;					// 7
extern const int OSKEY_NUMPAD_8 = KEYCODE_NUMPAD_8;					// 8
extern const int OSKEY_NUMPAD_9 = KEYCODE_NUMPAD_9;					// 9
extern const int OSKEY_NUMPAD_ADD = KEYCODE_NUMPAD_ADD;				// +
extern const int OSKEY_NUMPAD_4 = KEYCODE_NUMPAD_4;					// 4
extern const int OSKEY_NUMPAD_5 = KEYCODE_NUMPAD_5;					// 5
extern const int OSKEY_NUMPAD_6 = KEYCODE_NUMPAD_6;					// 6
extern const int OSKEY_NUMPAD_1 = KEYCODE_NUMPAD_1;					// 1
extern const int OSKEY_NUMPAD_2 = KEYCODE_NUMPAD_2;					// 2
extern const int OSKEY_NUMPAD_3 = KEYCODE_NUMPAD_3;					// 3
extern const int OSKEY_NUMPAD_0 = KEYCODE_NUMPAD_0;					// 0
extern const int OSKEY_NUMPAD_DOT = KEYCODE_NUMPAD_DOT;				// .
extern const int OSKEY_KANJI = KEYCODE_ZENKAKU_HANKAKU;				//
extern const int OSKEY_HENKAN = KEYCODE_HENKAN;						//
extern const int OSKEY_MUHENKAN = KEYCODE_MUHENKAN;					//
extern const int OSKEY_ZENHAN = KEYCODE_ZENKAKU_HANKAKU;			//
extern const int OSKEY_KATAHIRA = KEYCODE_KATAKANA_HIRAGANA;		//
extern const int OSKEY_ROMAN = KEYCODE_KANA;						//
extern const int OSKEY_EISU = KEYCODE_EISU;							//

} /*Ribbon*/
