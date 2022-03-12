#pragma once
#ifndef _RIBBON_PLATFORMLIB_H_
#define _RIBBON_PLATFORMLIB_H_
namespace Ribbon {

// constants
#ifdef _WINDOWS
const char CHAR_PATH_DELIMITOR = '\\';
const char16_t CHAR16_PATH_DELIMITOR = u'\\';
#else
const char CHAR_PATH_DELIMITOR = '/';
const char16_t CHAR16_PATH_DELIMITOR = u'/';
#endif

extern const int OSKEY_ESCAPE;			// VK_ESCAPE		KEYCODE_ESCAPE			Esc
extern const int OSKEY_F1;				// VK_F1			KEYCODE_F1				F1
extern const int OSKEY_F2;				// VK_F2			KEYCODE_F2				F2
extern const int OSKEY_F3;				// VK_F3			KEYCODE_F3				F3
extern const int OSKEY_F4;				// VK_F4			KEYCODE_F4				F4
extern const int OSKEY_F5;				// VK_F5			KEYCODE_F5				F5
extern const int OSKEY_F6;				// VK_F6			KEYCODE_F6				F6
extern const int OSKEY_F7;				// VK_F7			KEYCODE_F7				F7
extern const int OSKEY_F8;				// VK_F8			KEYCODE_F8				F8
extern const int OSKEY_F9;				// VK_F9			KEYCODE_F9				F9
extern const int OSKEY_F10;				// VK_F10			KEYCODE_F10				F10
extern const int OSKEY_F11;				// VK_F11			KEYCODE_F11				F11
extern const int OSKEY_F12;				// VK_F12			KEYCODE_F12				F12
extern const int OSKEY_GRAVE;			// VK_OEM3			KEYCODE_GRAVE			`
extern const int OSKEY_1;				// '1'				KEYCODE_1				1
extern const int OSKEY_2;				// '2'				KEYCODE_2				2
extern const int OSKEY_3;				// '3'				KEYCODE_3				3
extern const int OSKEY_4;				// '4'				KEYCODE_4				4
extern const int OSKEY_5;				// '5'				KEYCODE_5				5
extern const int OSKEY_6;				// '6'				KEYCODE_6				6
extern const int OSKEY_7;				// '7'				KEYCODE_7				7
extern const int OSKEY_8;				// '8'				KEYCODE_8				8
extern const int OSKEY_9;				// '9'				KEYCODE_9				9
extern const int OSKEY_0;				// '0'				KEYCODE_0				0
extern const int OSKEY_MINUS;			// VK_OEM_MINUS		KEYCODE_MINUS			-
extern const int OSKEY_PLUS;			// VK_OEM_PLUS		KEYCODE_PLUS			:
extern const int OSKEY_BACK;			// VK_BACK			KEYCODE_DEL				BackSpace
extern const int OSKEY_TAB;				// VK_TAB			KEYCODE_TAB				Tab
extern const int OSKEY_Q;				// 'Q"				KEYCODE_Q				Q
extern const int OSKEY_W;				// 'W'				KEYCODE_W				W
extern const int OSKEY_E;				// 'E'				KEYCODE_E				E
extern const int OSKEY_R;				// 'R'				KEYCODE_R				R
extern const int OSKEY_T;				// 'T'				KEYCODE_T				T
extern const int OSKEY_Y;				// 'Y'				KEYCODE_Y				Y
extern const int OSKEY_U;				// 'U'				KEYCODE_U				U
extern const int OSKEY_I;				// 'I'				KEYCODE_I				I
extern const int OSKEY_O;				// 'O'				KEYCODE_O				O
extern const int OSKEY_P;				// 'P'				KEYCODE_P				P
extern const int OSKEY_LBRACKET;		// VK_OEM_4			KEYCODE_LEFT_BRACKET	[
extern const int OSKEY_RBRACKET;		// VK_OEM_6			KEYCODE_RIGHT_BRACKET	]
extern const int OSKEY_BACKSLASH;		// VK_OEM_5			KEYCODE_BACKSLASH		[\]
extern const int OSKEY_CAPSLOCK;		// VK_OEM_ATTN		KEYCODE_CAPS_LOCK		CapsLock
extern const int OSKEY_A;				// 'A'				KEYCODE_A				A
extern const int OSKEY_S;				// 'S'				KEYCODE_S				S
extern const int OSKEY_D;				// 'D'				KEYCODE_D				D
extern const int OSKEY_F;				// 'F'				KEYCODE_F				F
extern const int OSKEY_G;				// 'G'				KEYCODE_G				G
extern const int OSKEY_H;				// 'H'				KEYCODE_H				H
extern const int OSKEY_J;				// 'J'				KEYCODE_J				J
extern const int OSKEY_K;				// 'K'				KEYCODE_K				K
extern const int OSKEY_L;				// 'L'				KEYCODE_L				L
extern const int OSKEY_SEMICOLON;		// VK_OEM_1			KEYCODE_SEMICOLON		;
extern const int OSKEY_APOSTROPHE;		// VK_OEM_7			KEYCODE_APOSTROPHE		^
extern const int OSKEY_ENTER;			// VK_RETURN		KEYCODE_ENTER			Enter
extern const int OSKEY_LSHIFT;			// VK_SHIFT			KEYCODE_SHIFT_LEFT		Shift
extern const int OSKEY_Z;				// 'Z'				KEYCODE_Z				Z
extern const int OSKEY_X;				// 'X'				KEYCODE_X				X
extern const int OSKEY_C;				// 'C'				KEYCODE_C				C
extern const int OSKEY_V;				// 'V'				KEYCODE_V				V
extern const int OSKEY_B;				// 'B'				KEYCODE_B				B
extern const int OSKEY_N;				// 'N'				KEYCODE_N				N
extern const int OSKEY_M;				// 'M'				KEYCODE_M				M
extern const int OSKEY_COMMA;			// VK_OEM_COMMA		KEYCODE_COMMA			,
extern const int OSKEY_PERIOD;			// VK_OEM_PERIOD	KEYCODE_PERIOD			.
extern const int OSKEY_SLASH;			// VK_OEM_2			KEYCODE_SLASH			/
extern const int OSKEY_RSHIFT;			// VK_SHIFT			KEYCODE_SHIFT_RIGHT		Shift
extern const int OSKEY_LCTRL;			// VK_CONTROL		KEYCODE_CTRL_LEFT		Ctrl
extern const int OSKEY_LWIN;			// VK_LWIN									L-Windows
extern const int OSKEY_LALT;			// VK_MENU			KEYCODE_ALT_LEFT		Alt
extern const int OSKEY_SPACE;			// VK_SPACE			KEYCODE_SPACE			Space
extern const int OSKEY_RALT;			// VK_MENU			KEYCODE_ALT_RIGHT		Alt
extern const int OSKEY_RWIN;			// VK_RWIN									R-Windows
extern const int OSKEY_MENU;			// VK_APPS			KEYCODE_MENU			ApplicationMenu
extern const int OSKEY_RCTRL;			// VK_CONTROL		KEYCODE_CTRL_RIGHT		Ctrl
extern const int OSKEY_SYSRQ;			// VK_SNAPSHOT		KEYCODE_SYSRQ			PrintScreen
extern const int OSKEY_SCROLL;			// VK_SCROLL		KEYCODE_SCROLL_LOCK		ScrollLock
extern const int OSKEY_BREAK;			// VK_PAUSE			KEYCODE_BREAK			Pause
extern const int OSKEY_INSERT;			// VK_INSERT		KEYCODE_INSERT			Insert
extern const int OSKEY_HOME;			// VK_HOME			KEYCODE_MOVE_HOME		Home
extern const int OSKEY_PAGEUP;			// VK_PRIOR			KEYCODE_PAGE_UP			PageUp
extern const int OSKEY_DELETE;			// VK_DELETE		KEYCODE_FORWARD_DEL		Delete
extern const int OSKEY_END;				// VK_END			KEYCODE_MOVE_END		End
extern const int OSKEY_PAGEDOWN;		// VK_NEXT			KEYCODE_PAGE_DOWN		PageDown
extern const int OSKEY_UP;				// VK_UP			KEYCODE_DPAD_UP			Up
extern const int OSKEY_LEFT;			// VK_LEFT			KEYCODE_DPAD_LEFT		<-
extern const int OSKEY_DOWN;			// VK_DOWN			KEYCODE_DPAD_DOWN		Down
extern const int OSKEY_RIGHT;			// VK_RIGHT			KEYCODE_DPAD_RIGHT		->
extern const int OSKEY_NUMLOCK;			// VK_NUMLOCK		KEYCODE_NUM_LOCK		NumLock
extern const int OSKEY_NUMPAD_DIVIDE;	// VK_DIVIDE		KEYCODE_NUMPAD_DIVIDE	/
extern const int OSKEY_NUMPAD_MULTIPLY;	// VK_MULTIPLY		KEYCODE_NUMPAD_MULTIPLY	*
extern const int OSKEY_NUMPAD_SUBTRACT;	// VK_SUBTRACT		KEYCODE_NUMPAD_SUBTRACT	-
extern const int OSKEY_NUMPAD_7;		// VK_NUMPAD7		KEYCODE_NUMPAD_7		7
extern const int OSKEY_NUMPAD_8;		// VK_NUMPAD8		KEYCODE_NUMPAD_8		8
extern const int OSKEY_NUMPAD_9;		// VK_NUMPAD9		KEYCODE_NUMPAD_9		9
extern const int OSKEY_NUMPAD_ADD;		// VK_ADD			KEYCODE_NUMPAD_ADD		+
extern const int OSKEY_NUMPAD_4;		// VK_NUMPAD4		KEYCODE_NUMPAD_4		4
extern const int OSKEY_NUMPAD_5;		// VK_NUMPAD5		KEYCODE_NUMPAD_5		5
extern const int OSKEY_NUMPAD_6;		// VK_NUMPAD6		KEYCODE_NUMPAD_6		6
extern const int OSKEY_NUMPAD_1;		// VK_NUMPAD1		KEYCODE_NUMPAD_1		1
extern const int OSKEY_NUMPAD_2;		// VK_NUMPAD2		KEYCODE_NUMPAD_2		2
extern const int OSKEY_NUMPAD_3;		// VK_NUMPAD3		KEYCODE_NUMPAD_3		3
extern const int OSKEY_NUMPAD_0;		// VK_NUMPAD0		KEYCODE_NUMPAD_0		0
extern const int OSKEY_NUMPAD_DOT;		// VK_DECIMAL		KEYCODE_NUMPAD_DOT		.
extern const int OSKEY_KANJI;			// VK_KANJI			KEYCODE_ZENKAKU_HANKAKU
extern const int OSKEY_HENKAN;			// VK_CONVERT		KEYCODE_HENKAN
extern const int OSKEY_MUHENKAN;		// VK_NONCONVERT	KEYCODE_MUHENKAN
extern const int OSKEY_ZENHAN;			// VK_DBE_DBCSCHAR	KEYCODE_ZENKAKU_HANKAKU
extern const int OSKEY_KATAHIRA;		// VK_DBE_KATAKANA	KEYCODE_KATAKANA_HIRAGANA
extern const int OSKEY_ROMAN;			// VK_DBE_ROMAN		KEYCODE_KANA??
extern const int OSKEY_EISU;			// VK_DBE_ALPHANUMERIC KEYCODE_EISU

struct ISetting;
class RefString;

struct IMemoryMappedFile
{
	virtual const void* GetAddress() const = 0;
	virtual size_t GetFileSize() const = 0;
};

struct IPlatform
{
	virtual void Printf(const char*, ...) const = 0;
	virtual void Error(const char*, ...) const = 0;

	virtual UUID CreateUUID() const = 0;

	virtual std::shared_ptr<IMemoryMappedFile> OpenMemoryMappedFile(const char* fileName) const = 0;

	virtual std::vector<std::string> EnumerateFiles(const char* directory, const char* fileMask) const = 0;

	enum class PathType
	{
		Current,
		BinPath,
		UserPath,
	};
	virtual std::string GetPathByType(PathType pathType) const = 0; // contains the lat path delimitor

	virtual std::shared_ptr<ISetting> GetSettings(bool tryFallbackPath = false) const = 0;
};

extern const IPlatform* Platform;
} // Ribbon

#endif // _RIBBON_PLATFORMLIB_H_
