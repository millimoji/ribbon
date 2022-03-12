#pragma once
#ifndef _RIBBON_SIPKEYDEF_H_
#define _RIBBON_SIPKEYDEF_H_
namespace Ribbon
{

const int SIPKEY_ESCAPE = 1;
const int SIPKEY_F1 = 2;
const int SIPKEY_F2 = 3;
const int SIPKEY_F3 = 4;
const int SIPKEY_F4 = 5;
const int SIPKEY_F5 = 6;
const int SIPKEY_F6 = 7;
const int SIPKEY_F7 = 8;
const int SIPKEY_F8 = 9;
const int SIPKEY_F9 = 10;
const int SIPKEY_F10 = 11;
const int SIPKEY_F11 = 12;
const int SIPKEY_F12 = 13;
const int SIPKEY_GRAVE = 14;
const int SIPKEY_1 = 49;
const int SIPKEY_2 = 50;
const int SIPKEY_3 = 51;
const int SIPKEY_4 = 52;
const int SIPKEY_5 = 53;
const int SIPKEY_6 = 54;
const int SIPKEY_7 = 55;
const int SIPKEY_8 = 56;
const int SIPKEY_9 = 57;
const int SIPKEY_0 = 48;
const int SIPKEY_MINUS = 15;
const int SIPKEY_PLUS = 16;
const int SIPKEY_BACK = 17;
const int SIPKEY_TAB = 18;
const int SIPKEY_Q = 19;
const int SIPKEY_W = 20;
const int SIPKEY_E = 21;
const int SIPKEY_R = 22;
const int SIPKEY_T = 23;
const int SIPKEY_Y = 24;
const int SIPKEY_U = 25;
const int SIPKEY_I = 26;
const int SIPKEY_O = 27;
const int SIPKEY_P = 28;
const int SIPKEY_LBRACKET = 29;
const int SIPKEY_RBRACKET = 30;
const int SIPKEY_BACKSLASH = 31;
const int SIPKEY_CAPSLOCK = 32;
const int SIPKEY_A = 33;
const int SIPKEY_S = 34;
const int SIPKEY_D = 35;
const int SIPKEY_F = 36;
const int SIPKEY_G = 37;
const int SIPKEY_H = 38;
const int SIPKEY_J = 39;
const int SIPKEY_K = 40;
const int SIPKEY_L = 41;
const int SIPKEY_SEMICOLON = 42;
const int SIPKEY_APOSTROPHE = 43;
const int SIPKEY_ENTER = 44;
const int SIPKEY_LSHIFT = 45;
const int SIPKEY_Z = 46;
const int SIPKEY_X = 47;
const int SIPKEY_C = 58;
const int SIPKEY_V = 59;
const int SIPKEY_B = 60;
const int SIPKEY_N = 61;
const int SIPKEY_M = 62;
const int SIPKEY_COMMA = 63;
const int SIPKEY_PERIOD = 64;
const int SIPKEY_SLASH = 65;
const int SIPKEY_RSHIFT = 66;
const int SIPKEY_LCTRL = 67;
const int SIPKEY_LWIN = 68;
const int SIPKEY_LALT = 69;
const int SIPKEY_SPACE = 70;
const int SIPKEY_RALT = 71;
const int SIPKEY_RWIN = 72;
const int SIPKEY_MENU = 73;
const int SIPKEY_RCTRL = 74;
const int SIPKEY_SYSRQ = 75;
const int SIPKEY_SCROLL = 76;
const int SIPKEY_BREAK = 77;
const int SIPKEY_INSERT = 78;
const int SIPKEY_HOME = 79;
const int SIPKEY_PAGEUP = 80;
const int SIPKEY_DELETE = 81;
const int SIPKEY_END = 82;
const int SIPKEY_PAGEDOWN = 83;
const int SIPKEY_UP = 84;
const int SIPKEY_LEFT = 85;
const int SIPKEY_DOWN = 86;
const int SIPKEY_RIGHT = 87;
const int SIPKEY_NUMLOCK = 88;
const int SIPKEY_NUMPAD_DIVIDE = 89;
const int SIPKEY_NUMPAD_MULTIPLY = 90;
const int SIPKEY_NUMPAD_SUBTRACT = 91;
const int SIPKEY_NUMPAD_7 = 92;
const int SIPKEY_NUMPAD_8 = 93;
const int SIPKEY_NUMPAD_9 = 94;
const int SIPKEY_NUMPAD_ADD = 95;
const int SIPKEY_NUMPAD_4 = 96;
const int SIPKEY_NUMPAD_5 = 97;
const int SIPKEY_NUMPAD_6 = 98;
const int SIPKEY_NUMPAD_1 = 99;
const int SIPKEY_NUMPAD_2 = 100;
const int SIPKEY_NUMPAD_3 = 101;
const int SIPKEY_NUMPAD_0 = 102;
const int SIPKEY_NUMPAD_DOT = 103;
const int SIPKEY_KANJI = 104;
const int SIPKEY_KATAHIRA = 105;
const int SIPKEY_EISU = 106;
const int SIPKEY_ZENHAN = 107;
const int SIPKEY_HENKAN = 108;
const int SIPKEY_MUHENKAN = 109;
const int SIPKEY_ROMAN = 110;

// below, SIP key has no corresponding OSKEY
const int SIPKEY_ANYTEXT = 111;
const int SIPKEY_COMPOSITIONTEXT = 112;
const int SIPKEY_ANYFUNCTION = 113;
const int SIPKEY_GAP = 114;
const int SIPKEY_SWITCH = 115;
const int SIPKEY_JPN12_KANA = 116;
const int SIPKEY_JPN12_MOD = 117;
const int SIPKEY_JPN12_BRK = 118;
const int SIPKEY_JPN12_BCKTGL = 119;
const int SIPKEY_CATEGORY = 120;

const uint32_t SHIFT_ON = 0x01;
const uint32_t CAPS_ON = 0x02;
const uint32_t CONTROL_ON = 0x04;
const uint32_t ALT_ON = 0x08;
const uint32_t IME_ON = 0x10;
const uint32_t IME_MODE_NATIVE = 0x01;
const uint32_t IME_MODE_KATAKANA_FULL = 0x02;
const uint32_t IME_MODE_KATAKANA_HALF = 0x03;
const uint32_t IME_MODE_ALPHABET_FULL = 0x04;
const uint32_t IME_MODE_ALPHABET_HALF = 0x05;

struct IKeyCodeHelper
{
	virtual int SIPKeyToOSKey(int sipKey) = 0;
	virtual int OSKeyToSIPKey(int osKey) = 0;
	virtual int UnicdoeFromKey(int osKey, uint32_t modifierState) = 0;
};

FACTORYEXTERN(KeyCodeHelper);
} // Ribbon
#endif //_RIBBON_SIPKEYDEF_H_
