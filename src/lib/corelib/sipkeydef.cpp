#include "pch.h"
namespace Ribbon {

static std::pair<uint16_t, uint16_t> SIPKEYtoOSKEY[] = {
{ (uint16_t)SIPKEY_ESCAPE,			(uint16_t)OSKEY_ESCAPE },
{ (uint16_t)SIPKEY_F1,				(uint16_t)OSKEY_F1 },
{ (uint16_t)SIPKEY_F2,				(uint16_t)OSKEY_F2 },
{ (uint16_t)SIPKEY_F3,				(uint16_t)OSKEY_F3 },
{ (uint16_t)SIPKEY_F4,				(uint16_t)OSKEY_F4 },
{ (uint16_t)SIPKEY_F5,				(uint16_t)OSKEY_F5 },
{ (uint16_t)SIPKEY_F6,				(uint16_t)OSKEY_F6 },
{ (uint16_t)SIPKEY_F7,				(uint16_t)OSKEY_F7 },
{ (uint16_t)SIPKEY_F8,				(uint16_t)OSKEY_F8 },
{ (uint16_t)SIPKEY_F9,				(uint16_t)OSKEY_F9 },
{ (uint16_t)SIPKEY_F10,				(uint16_t)OSKEY_F10 },
{ (uint16_t)SIPKEY_F11,				(uint16_t)OSKEY_F11 },
{ (uint16_t)SIPKEY_F12,				(uint16_t)OSKEY_F12 },
{ (uint16_t)SIPKEY_GRAVE,			(uint16_t)OSKEY_GRAVE },
{ (uint16_t)SIPKEY_1,				(uint16_t)OSKEY_1 },
{ (uint16_t)SIPKEY_2,				(uint16_t)OSKEY_2 },
{ (uint16_t)SIPKEY_3,				(uint16_t)OSKEY_3 },
{ (uint16_t)SIPKEY_4,				(uint16_t)OSKEY_4 },
{ (uint16_t)SIPKEY_5,				(uint16_t)OSKEY_5 },
{ (uint16_t)SIPKEY_6,				(uint16_t)OSKEY_6 },
{ (uint16_t)SIPKEY_7,				(uint16_t)OSKEY_7 },
{ (uint16_t)SIPKEY_8,				(uint16_t)OSKEY_8 },
{ (uint16_t)SIPKEY_9,				(uint16_t)OSKEY_9 },
{ (uint16_t)SIPKEY_0,				(uint16_t)OSKEY_0 },
{ (uint16_t)SIPKEY_MINUS,			(uint16_t)OSKEY_MINUS },
{ (uint16_t)SIPKEY_PLUS,			(uint16_t)OSKEY_PLUS },
{ (uint16_t)SIPKEY_BACK,			(uint16_t)OSKEY_BACK },
{ (uint16_t)SIPKEY_TAB,				(uint16_t)OSKEY_TAB },
{ (uint16_t)SIPKEY_Q,				(uint16_t)OSKEY_Q },
{ (uint16_t)SIPKEY_W,				(uint16_t)OSKEY_W },
{ (uint16_t)SIPKEY_E,				(uint16_t)OSKEY_E },
{ (uint16_t)SIPKEY_R,				(uint16_t)OSKEY_R },
{ (uint16_t)SIPKEY_T,				(uint16_t)OSKEY_T },
{ (uint16_t)SIPKEY_Y,				(uint16_t)OSKEY_Y },
{ (uint16_t)SIPKEY_U,				(uint16_t)OSKEY_U },
{ (uint16_t)SIPKEY_I,				(uint16_t)OSKEY_I },
{ (uint16_t)SIPKEY_O,				(uint16_t)OSKEY_O },
{ (uint16_t)SIPKEY_P,				(uint16_t)OSKEY_P },
{ (uint16_t)SIPKEY_LBRACKET,		(uint16_t)OSKEY_LBRACKET },
{ (uint16_t)SIPKEY_RBRACKET,		(uint16_t)OSKEY_RBRACKET },
{ (uint16_t)SIPKEY_BACKSLASH,		(uint16_t)OSKEY_BACKSLASH },
{ (uint16_t)SIPKEY_CAPSLOCK,		(uint16_t)OSKEY_CAPSLOCK },
{ (uint16_t)SIPKEY_A,				(uint16_t)OSKEY_A },
{ (uint16_t)SIPKEY_S,				(uint16_t)OSKEY_S },
{ (uint16_t)SIPKEY_D,				(uint16_t)OSKEY_D },
{ (uint16_t)SIPKEY_F,				(uint16_t)OSKEY_F },
{ (uint16_t)SIPKEY_G,				(uint16_t)OSKEY_G },
{ (uint16_t)SIPKEY_H,				(uint16_t)OSKEY_H },
{ (uint16_t)SIPKEY_J,				(uint16_t)OSKEY_J },
{ (uint16_t)SIPKEY_K,				(uint16_t)OSKEY_K },
{ (uint16_t)SIPKEY_L,				(uint16_t)OSKEY_L },
{ (uint16_t)SIPKEY_SEMICOLON,		(uint16_t)OSKEY_SEMICOLON },
{ (uint16_t)SIPKEY_APOSTROPHE,		(uint16_t)OSKEY_APOSTROPHE },
{ (uint16_t)SIPKEY_ENTER,			(uint16_t)OSKEY_ENTER },
{ (uint16_t)SIPKEY_LSHIFT,			(uint16_t)OSKEY_LSHIFT },
{ (uint16_t)SIPKEY_Z,				(uint16_t)OSKEY_Z },
{ (uint16_t)SIPKEY_X,				(uint16_t)OSKEY_X },
{ (uint16_t)SIPKEY_C,				(uint16_t)OSKEY_C },
{ (uint16_t)SIPKEY_V,				(uint16_t)OSKEY_V },
{ (uint16_t)SIPKEY_B,				(uint16_t)OSKEY_B },
{ (uint16_t)SIPKEY_N,				(uint16_t)OSKEY_N },
{ (uint16_t)SIPKEY_M,				(uint16_t)OSKEY_M },
{ (uint16_t)SIPKEY_COMMA,			(uint16_t)OSKEY_COMMA },
{ (uint16_t)SIPKEY_PERIOD,			(uint16_t)OSKEY_PERIOD },
{ (uint16_t)SIPKEY_SLASH,			(uint16_t)OSKEY_SLASH },
{ (uint16_t)SIPKEY_RSHIFT,			(uint16_t)OSKEY_RSHIFT },
{ (uint16_t)SIPKEY_LCTRL,			(uint16_t)OSKEY_LCTRL },
{ (uint16_t)SIPKEY_LWIN,			(uint16_t)OSKEY_LWIN },
{ (uint16_t)SIPKEY_LALT,			(uint16_t)OSKEY_LALT },
{ (uint16_t)SIPKEY_SPACE,			(uint16_t)OSKEY_SPACE },
{ (uint16_t)SIPKEY_RALT,			(uint16_t)OSKEY_RALT },
{ (uint16_t)SIPKEY_RWIN,			(uint16_t)OSKEY_RWIN },
{ (uint16_t)SIPKEY_MENU,			(uint16_t)OSKEY_MENU },
{ (uint16_t)SIPKEY_RCTRL,			(uint16_t)OSKEY_RCTRL },
{ (uint16_t)SIPKEY_SYSRQ,			(uint16_t)OSKEY_SYSRQ },
{ (uint16_t)SIPKEY_SCROLL,			(uint16_t)OSKEY_SCROLL },
{ (uint16_t)SIPKEY_BREAK,			(uint16_t)OSKEY_BREAK },
{ (uint16_t)SIPKEY_INSERT,			(uint16_t)OSKEY_INSERT },
{ (uint16_t)SIPKEY_HOME,			(uint16_t)OSKEY_HOME },
{ (uint16_t)SIPKEY_PAGEUP,			(uint16_t)OSKEY_PAGEUP },
{ (uint16_t)SIPKEY_DELETE,			(uint16_t)OSKEY_DELETE },
{ (uint16_t)SIPKEY_END,				(uint16_t)OSKEY_END },
{ (uint16_t)SIPKEY_PAGEDOWN,		(uint16_t)OSKEY_PAGEDOWN },
{ (uint16_t)SIPKEY_UP,				(uint16_t)OSKEY_UP },
{ (uint16_t)SIPKEY_LEFT,			(uint16_t)OSKEY_LEFT },
{ (uint16_t)SIPKEY_DOWN,			(uint16_t)OSKEY_DOWN },
{ (uint16_t)SIPKEY_RIGHT,			(uint16_t)OSKEY_RIGHT },
{ (uint16_t)SIPKEY_NUMLOCK,			(uint16_t)OSKEY_NUMLOCK },
{ (uint16_t)SIPKEY_NUMPAD_DIVIDE,	(uint16_t)OSKEY_NUMPAD_DIVIDE },
{ (uint16_t)SIPKEY_NUMPAD_MULTIPLY,	(uint16_t)OSKEY_NUMPAD_MULTIPLY },
{ (uint16_t)SIPKEY_NUMPAD_SUBTRACT,	(uint16_t)OSKEY_NUMPAD_SUBTRACT },
{ (uint16_t)SIPKEY_NUMPAD_7,		(uint16_t)OSKEY_NUMPAD_7 },
{ (uint16_t)SIPKEY_NUMPAD_8,		(uint16_t)OSKEY_NUMPAD_8 },
{ (uint16_t)SIPKEY_NUMPAD_9,		(uint16_t)OSKEY_NUMPAD_9 },
{ (uint16_t)SIPKEY_NUMPAD_ADD,		(uint16_t)OSKEY_NUMPAD_ADD },
{ (uint16_t)SIPKEY_NUMPAD_4,		(uint16_t)OSKEY_NUMPAD_4 },
{ (uint16_t)SIPKEY_NUMPAD_5,		(uint16_t)OSKEY_NUMPAD_5 },
{ (uint16_t)SIPKEY_NUMPAD_6,		(uint16_t)OSKEY_NUMPAD_6 },
{ (uint16_t)SIPKEY_NUMPAD_1,		(uint16_t)OSKEY_NUMPAD_1 },
{ (uint16_t)SIPKEY_NUMPAD_2,		(uint16_t)OSKEY_NUMPAD_2 },
{ (uint16_t)SIPKEY_NUMPAD_3,		(uint16_t)OSKEY_NUMPAD_3 },
{ (uint16_t)SIPKEY_NUMPAD_0,		(uint16_t)OSKEY_NUMPAD_0 },
{ (uint16_t)SIPKEY_NUMPAD_DOT,		(uint16_t)OSKEY_NUMPAD_DOT },
{ (uint16_t)SIPKEY_KANJI,			(uint16_t)OSKEY_KANJI },
{ (uint16_t)SIPKEY_KATAHIRA,		(uint16_t)OSKEY_KATAHIRA },
{ (uint16_t)SIPKEY_EISU,			(uint16_t)OSKEY_EISU },
{ (uint16_t)SIPKEY_ZENHAN,			(uint16_t)OSKEY_ZENHAN },
{ (uint16_t)SIPKEY_HENKAN,			(uint16_t)OSKEY_HENKAN },
{ (uint16_t)SIPKEY_MUHENKAN,		(uint16_t)OSKEY_MUHENKAN },
{ (uint16_t)SIPKEY_ROMAN,			(uint16_t)OSKEY_ROMAN },
};

struct OSKEYINFO {
	int			osKey;
	char16_t	normalChar;
	char16_t	shiftChar;
	char16_t	capsChar;
};

OSKEYINFO c_osKeyInfo[] = {
	{ OSKEY_GRAVE,		u'`',	u'~',	u'`' },
	{ OSKEY_1,			u'1',	u'!',	u'1' },
	{ OSKEY_2,			u'2',	u'@',	u'2' },
	{ OSKEY_3,			u'3',	u'#',	u'3' },
	{ OSKEY_4,			u'4',	u'$',	u'4' },
	{ OSKEY_5,			u'5',	u'%',	u'5' },
	{ OSKEY_6,			u'6',	u'^',	u'6' },
	{ OSKEY_7,			u'7',	u'&',	u'7' },
	{ OSKEY_8,			u'8',	u'*',	u'8' },
	{ OSKEY_9,			u'9',	u'(',	u'9' },
	{ OSKEY_0,			u'0',	u')',	u'0' },
	{ OSKEY_MINUS,		u'-',	u'_',	u'-' },
	{ OSKEY_PLUS,		u'=',	u'+',	u'=' },
	{ OSKEY_Q,			u'q',	u'Q',	u'Q' },
	{ OSKEY_W,			u'w',	u'W',	u'W' },
	{ OSKEY_E,			u'e',	u'E',	u'E' },
	{ OSKEY_R,			u'r',	u'R',	u'R' },
	{ OSKEY_T,			u't',	u'T',	u'T' },
	{ OSKEY_Y,			u'y',	u'Y',	u'Y' },
	{ OSKEY_U,			u'u',	u'U',	u'U' },
	{ OSKEY_I,			u'i',	u'I',	u'I' },
	{ OSKEY_O,			u'o',	u'O',	u'O' },
	{ OSKEY_P,			u'p',	u'P',	u'P' },
	{ OSKEY_LBRACKET,	u'[',	u'{',	u'[' },
	{ OSKEY_RBRACKET,	u']',	u'}',	u']' },
	{ OSKEY_BACKSLASH,	u'\\',	u'|',	u'\\' },
	{ OSKEY_A,			u'a',	u'A',	u'A' },
	{ OSKEY_S,			u's',	u'S',	u'S' },
	{ OSKEY_D,			u'd',	u'D',	u'D' },
	{ OSKEY_F,			u'f',	u'F',	u'F' },
	{ OSKEY_G,			u'g',	u'G',	u'G' },
	{ OSKEY_H,			u'h',	u'H',	u'H' },
	{ OSKEY_J,			u'j',	u'J',	u'J' },
	{ OSKEY_K,			u'k',	u'K',	u'K' },
	{ OSKEY_L,			u'l',	u'L',	u'L' },
	{ OSKEY_SEMICOLON,	u';',	u':',	u';' },
	{ OSKEY_APOSTROPHE,	u'\'',	u'"',	u'\'' },
	{ OSKEY_Z,			u'z',	u'Z',	u'Z' },
	{ OSKEY_X,			u'x',	u'X',	u'X' },
	{ OSKEY_C,			u'c',	u'C',	u'C' },
	{ OSKEY_V,			u'v',	u'V',	u'V' },
	{ OSKEY_B,			u'b',	u'B',	u'B' },
	{ OSKEY_N,			u'n',	u'N',	u'N' },
	{ OSKEY_M,			u'm',	u'M',	u'M' },
	{ OSKEY_COMMA,		u',',	u'<',	u',' },
	{ OSKEY_PERIOD,		u'.',	u'>',	u'.' },
	{ OSKEY_SLASH,		u'/',	u'?',	u'/' },
	{ OSKEY_SPACE,		u' ',	u' ',	u' ' },
	{ OSKEY_NUMPAD_DIVIDE, 		u'/',	u'/',	u'/' },
	{ OSKEY_NUMPAD_MULTIPLY,	u'*',	u'*',	u'*' },
	{ OSKEY_NUMPAD_SUBTRACT,	u'-',	u'-',	u'-' },
	{ OSKEY_NUMPAD_7,			u'7',	u'7',	u'7' },
	{ OSKEY_NUMPAD_8,			u'8',	u'8',	u'8' },
	{ OSKEY_NUMPAD_9,			u'9',	u'9',	u'9' },
	{ OSKEY_NUMPAD_ADD,			u'+',	u'+',	u'+' },
	{ OSKEY_NUMPAD_4,			u'4',	u'4',	u'4' },
	{ OSKEY_NUMPAD_5,			u'5',	u'5',	u'5' },
	{ OSKEY_NUMPAD_6,			u'6',	u'6',	u'6' },
	{ OSKEY_NUMPAD_1,			u'1',	u'1',	u'1' },
	{ OSKEY_NUMPAD_2,			u'2',	u'2',	u'2' },
	{ OSKEY_NUMPAD_3,			u'3',	u'3',	u'3' },
	{ OSKEY_NUMPAD_0,			u'0',	u'0',	u'0' },
	{ OSKEY_NUMPAD_DOT,			u'.',	u'.',	u'.' },
};

struct KeyCodeHelper :
	std::enable_shared_from_this<KeyCodeHelper>,
	public IKeyCodeHelper,
	public IObject
{
	int m_countItem;
	std::vector<std::pair<uint16_t, uint16_t>> m_osKeyToSipKey;

	virtual ~KeyCodeHelper() {}

	KeyCodeHelper() {
		m_countItem = COUNT_OF(c_osKeyInfo);
		std::sort(&c_osKeyInfo[0], &c_osKeyInfo[m_countItem],
			[](const OSKEYINFO& lhs, const OSKEYINFO& rhs) {
				return lhs.osKey < rhs.osKey;
			});

		int countSipKey = COUNT_OF(SIPKEYtoOSKEY);
		m_osKeyToSipKey.resize((size_t)countSipKey);
		memcpy(&m_osKeyToSipKey[0], &SIPKEYtoOSKEY[0], sizeof(SIPKEYtoOSKEY[0]) * countSipKey);

		// sort by SIP key
		std::sort(&SIPKEYtoOSKEY[0], &SIPKEYtoOSKEY[countSipKey],
			[](const std::pair<uint16_t, uint16_t>& lhs, const std::pair<uint16_t, uint16_t>& rhs) {
				return lhs.first < rhs.first;
			});

		// sort by OS key
		std::sort(m_osKeyToSipKey.begin(), m_osKeyToSipKey.end(),
			[](const std::pair<uint16_t, uint16_t>& lhs, const std::pair<uint16_t, uint16_t>& rhs) {
			return lhs.second < rhs.second;
		});
	}

	int SIPKeyToOSKey(int sipKey) override {
		if (sipKey > SIPKEY_NUMPAD_DOT) {
			return -1;
		}
		if (sipKey < 1) {
			return -1;
		}
		THROW_IF_FALSE(SIPKEYtoOSKEY[sipKey - 1].first == sipKey);
		return SIPKEYtoOSKEY[sipKey - 1].second;
	}
	int OSKeyToSIPKey(int osKey) override {
		auto it = std::lower_bound(m_osKeyToSipKey.begin(), m_osKeyToSipKey.end(), std::make_pair((uint16_t)0, (uint16_t)osKey),
			[](const std::pair<uint16_t, uint16_t>& lhs, const std::pair<uint16_t, uint16_t>& rhs) {
				return lhs.second < rhs.second;
			});
		if ((int)it->second == osKey) {
			return it->first;
		}
		return -1;
	}

	/*
	bool HandleIfSipModifierKey(int osKey, bool isDown) override {
		uint32_t modifierBit = 0;
		if (osKey == OSKEY_CAPSLOCK) {
			modifierBit = CAPS_ON;
		}
		else if ((osKey == OSKEY_LSHIFT) || (osKey == OSKEY_RSHIFT)) {
			modifierBit = SHIFT_ON;
		}
		else if ((osKey == OSKEY_LCTRL) || (osKey == OSKEY_RCTRL)) {
			modifierBit = CONTROL_ON;
		}
		else if ((osKey == OSKEY_LWIN) || (osKey == OSKEY_RWIN)) {
			return true;
		}
		else if ((osKey == OSKEY_LALT) || (osKey == OSKEY_RALT)) {
			modifierBit = ALT_ON;
		}
		if (modifierBit != 0) {
			if (isDown) {
				m_modifierState |= modifierBit;
			}
			else {
				m_modifierState &= ~modifierBit;
			}
			return true;
		}
		return false;
	}
	*/

	int UnicdoeFromKey(int osKey, uint32_t modifierState) override {
		if ((modifierState & (CONTROL_ON | ALT_ON)) != 0) {
			return -1;
		}

		OSKEYINFO osKeyInfo;
		osKeyInfo.osKey = osKey;
		auto it = std::lower_bound(&c_osKeyInfo[0], &c_osKeyInfo[m_countItem], osKeyInfo,
			[](const OSKEYINFO& lhs, const OSKEYINFO& rhs) {
				return lhs.osKey < rhs.osKey;
			});

		if (it->osKey != osKey || it == &c_osKeyInfo[m_countItem]) {
			return -1;
		}

		if ((modifierState & SHIFT_ON) != 0) {
			if ((modifierState & CAPS_ON) != 0) {
				if (it->normalChar == it->capsChar) {
					return it->shiftChar;
				}
				return it->normalChar;
			}
			return it->shiftChar;
		}
		if ((modifierState & CAPS_ON) != 0) {
			return it->capsChar;
		}
		return it->normalChar;
	}

	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(KeyCodeHelper);
} // Ribbon