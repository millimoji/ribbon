export const SIPKEY = {
  ESCAPE: 1,
  F1: 2,
  F2: 3,
  F3: 4,
  F4: 5,
  F5: 6,
  F6: 7,
  F7: 8,
  F8: 9,
  F9: 10,
  F10: 11,
  F11: 12,
  F12: 13,
  GRAVE: 14,
  _1: 49,
  _2: 50,
  _3: 51,
  _4: 52,
  _5: 53,
  _6: 54,
  _7: 55,
  _8: 56,
  _9: 57,
  _0: 48,
  MINUS: 15,
  PLUS: 16,
  BACK: 17,
  TAB: 18,
  Q: 19,
  W: 20,
  E: 21,
  R: 22,
  T: 23,
  Y: 24,
  U: 25,
  I: 26,
  O: 27,
  P: 28,
  LBRACKET: 29,
  RBRACKET: 30,
  BACKSLASH: 31,
  CAPSLOCK: 32,
  A: 33,
  S: 34,
  D: 35,
  F: 36,
  G: 37,
  H: 38,
  J: 39,
  K: 40,
  L: 41,
  SEMICOLON: 42,
  APOSTROPHE: 43,
  ENTER: 44,
  LSHIFT: 45,
  Z: 46,
  X: 47,
  C: 58,
  V: 59,
  B: 60,
  N: 61,
  M: 62,
  COMMA: 63,
  PERIOD: 64,
  SLASH: 65,
  RSHIFT: 66,
  LCTRL: 67,
  LWIN: 68,
  LALT: 69,
  SPACE: 70,
  RALT: 71,
  RWIN: 72,
  MENU: 73,
  RCTRL: 74,
  SYSRQ: 75,
  SCROLL: 76,
  BREAK: 77,
  INSERT: 78,
  HOME: 79,
  PAGEUP: 80,
  DELETE: 81,
  END: 82,
  PAGEDOWN: 83,
  UP: 84,
  LEFT: 85,
  DOWN: 86,
  RIGHT: 87,
  NUMLOCK: 88,
  NUMPAD_DIVIDE: 89,
  NUMPAD_MULTIPLY: 90,
  NUMPAD_SUBTRACT: 91,
  NUMPAD_7: 92,
  NUMPAD_8: 93,
  NUMPAD_9: 94,
  NUMPAD_ADD: 95,
  NUMPAD_4: 96,
  NUMPAD_5: 97,
  NUMPAD_6: 98,
  NUMPAD_1: 99,
  NUMPAD_2: 100,
  NUMPAD_3: 101,
  NUMPAD_0: 102,
  NUMPAD_DOT: 103,
  KANJI: 104,
  KATAHIRA: 105,
  EISU: 106,
  ZENHAN: 107,
  HENKAN: 108,
  MUHENKAN: 109,
  ROMAN: 110,
  // SIP only
  ANYTEXT: 111,
  COMPOSITIONTEXT: 112,
  ANYFUNCTION: 113,
  GAP: 114,
  SWITCH: 115,
  JPN12_KANA: 116,
  JPN12_MOD: 117,
  JPN12_BRK: 118,
  JPN12_BCKTGL: 119,
  CATEGORY: 120
}

export const kbdData = {
  /* eslint-disable-indent */
  keydefs: {
    'sw-quit': { l: '\uD83C\uDF10', kc: SIPKEY.SWITCH },
    'sw-12kana': { l: 'ロマ', kc: SIPKEY.SWITCH, next: 'qwertyromaji', prev: 'emojiview' },
    'sw-romaji': { l: 'abc', kc: SIPKEY.SWITCH, next: 'qwertyalpha', prev: 'numpadkana' },
    'sw-qwerty': { l: '記', kc: SIPKEY.SWITCH, next: 'symbolview', prev: 'qwertyromaji' },
    'sw-symbol': { l: '☺', kc: SIPKEY.SWITCH, next: 'emojiview', prev: 'qwertyalpha' },
    'sw-emoji': { l: 'あ', kc: SIPKEY.SWITCH, next: 'numpadkana', prev: 'symbolview' },

    'gap': { kc: SIPKEY.GAP },
    'gap05': { w: 0.5, kc: SIPKEY.GAP },
    'shift': { l: '\u23cf', c: 1, w: 1.5, kc: SIPKEY.LSHIFT }, // old: 21eb
    'qw-back': { l: '\u232b', c: 1, w: 1.5, kc: SIPKEY.BACK }, // old: 21d0
    'back': { l: '\u232b', kc: SIPKEY.BACK },
    'qwr-,': { l: ',', slide: [',', '!', '(', '[', '{', '「', '\''] },
    'qwr-.': { l: '.', slide: ['"', '」', '}', ']', ')', '?', '.'] },
    'qwa-,': { l: ',', slide: [',', '!', '(', '[', '{', '\''] },
    'qwa-.': { l: '.', slide: ['"', '}', ']', ')', '?', '.'] },
    'qw--': { l: '-' },
    'qw-space': { l: '\u2423', c: ' ', w: -10, kc: SIPKEY.SPACE },
    'enter': { l: '\u23ce', c: '\n', kc: SIPKEY.ENTER },
    'qw-a': { l: 'a', sl: 'A' },
    'qw-b': { l: 'b', sl: 'B' },
    'qw-c': { l: 'c', sl: 'C' },
    'qw-d': { l: 'd', sl: 'D' },
    'qw-e': { l: 'e', sl: 'E' },
    'qw-f': { l: 'f', sl: 'F' },
    'qw-g': { l: 'g', sl: 'G' },
    'qw-h': { l: 'h', sl: 'H' },
    'qw-i': { l: 'i', sl: 'I' },
    'qw-j': { l: 'j', sl: 'J' },
    'qw-k': { l: 'k', sl: 'K' },
    'qw-l': { l: 'l', sl: 'L' },
    'qw-m': { l: 'm', sl: 'M' },
    'qw-n': { l: 'n', sl: 'N' },
    'qw-o': { l: 'o', sl: 'O' },
    'qw-p': { l: 'p', sl: 'P' },
    'qw-q': { l: 'q', sl: 'Q' },
    'qw-r': { l: 'r', sl: 'R' },
    'qw-s': { l: 's', sl: 'S' },
    'qw-t': { l: 't', sl: 'T' },
    'qw-u': { l: 'u', sl: 'U' },
    'qw-v': { l: 'v', sl: 'V' },
    'qw-w': { l: 'w', sl: 'W' },
    'qw-x': { l: 'x', sl: 'X' },
    'qw-y': { l: 'y', sl: 'Y' },
    'qw-z': { l: 'z', sl: 'Z' },

    'np-a': { l: 'あ', flick: ['い', 'う', 'え', 'お'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-ka': { l: 'か', flick: ['き', 'く', 'け', 'こ'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-sa': { l: 'さ', flick: ['し', 'す', 'せ', 'そ'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-ta': { l: 'た', flick: ['ち', 'つ', 'て', 'と'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-na': { l: 'な', flick: ['に', 'ぬ', 'ね', 'の'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-ha': { l: 'は', flick: ['ひ', 'ふ', 'へ', 'ほ'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-ma': { l: 'ま', flick: ['み', 'む', 'め', 'も'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-ya': { l: 'や', flick: ['', 'ゆ', '', 'よ'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-ra': { l: 'ら', flick: ['り', 'る', 'れ', 'ろ'], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-mod': { l: '小\u3099\u2005 \u309a', kc: SIPKEY.JPN12_MOD },
    'np-wa': { l: 'わ', flick: ['を', 'ん', 'ー', ''], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-.': { l: '、。!?', kc: SIPKEY.JPN12_BRK, flick: ['。', '！', '？', ''], flickKC: SIPKEY.COMPOSITIONTEXT },
    'np-left': { l: '\u25C0', kc: SIPKEY.LEFT },
    'np-right': { l: '\u25B6', kc: SIPKEY.RIGHT },
    'np-space': { l: '\u2423', c: ' ', kc: SIPKEY.SPACE },
    'np-bcktgl': { l: '\u21bb', kc: SIPKEY.JPN12_BCKTGL },

    'sm-0': { l: '0', kc: SIPKEY.CATEGORY },
    'sm-1': { l: '1', kc: SIPKEY.CATEGORY },
    'sm-2': { l: '2', kc: SIPKEY.CATEGORY },
    'sm-3': { l: '3', kc: SIPKEY.CATEGORY },
    'sm-4': { l: '4', kc: SIPKEY.CATEGORY },
    'sm-5': { l: '5', kc: SIPKEY.CATEGORY },
    'sm-6': { l: '6', kc: SIPKEY.CATEGORY },

    'em-0': { l: '0', kc: SIPKEY.CATEGORY },
    'em-1': { l: '1', kc: SIPKEY.CATEGORY },
    'em-2': { l: '2', kc: SIPKEY.CATEGORY },
    'em-3': { l: '3', kc: SIPKEY.CATEGORY },
    'em-4': { l: '4', kc: SIPKEY.CATEGORY },
    'em-5': { l: '5', kc: SIPKEY.CATEGORY },
    'em-6': { l: '6', kc: SIPKEY.CATEGORY }
  },
  layouts: {
    qwertyromaji: {
      frames: [
        'qwerty'
      ],
      data: [
        // QWERTY
        //  q w e r t y u i o p
        //  a s d f g h j k l -
        //  SH z x c v b n m BS
        //  SW  SPC    , . EN
        ['qw-q', 'qw-w', 'qw-e', 'qw-r', 'qw-t', 'qw-y', 'qw-u', 'qw-i', 'qw-o', 'qw-p'],
        ['qw-a', 'qw-s', 'qw-d', 'qw-f', 'qw-g', 'qw-h', 'qw-j', 'qw-k', 'qw-l', 'qw--'],
        ['qw-z', 'qw-x', 'qw-c', 'qw-v', 'qw-b', 'qw-n', 'qw-m'],
        ['sw-romaji', 'qwr-,', 'qw-space', 'qwr-.']
      ],
      defaultKC: SIPKEY.COMPOSITIONTEXT
    },
    qwertyalpha: {
      frames: [
        'qwerty'
      ],
      data: [
        // QWERTY
        //  q w e r t y u i o p
        //   a s d f g h j k l
        //  SH z x c v b n m BS
        //  SW  SPC    , . EN
        ['qw-q', 'qw-w', 'qw-e', 'qw-r', 'qw-t', 'qw-y', 'qw-u', 'qw-i', 'qw-o', 'qw-p'],
        // ['gap05', 'qw-a', 'qw-s', 'qw-d', 'qw-f', 'qw-g', 'qw-h', 'qw-j', 'qw-k', 'qw-l', 'gap05'],
        ['qw-a', 'qw-s', 'qw-d', 'qw-f', 'qw-g', 'qw-h', 'qw-j', 'qw-k', 'qw-l', 'qw--'],
        ['qw-z', 'qw-x', 'qw-c', 'qw-v', 'qw-b', 'qw-n', 'qw-m'],
        ['sw-qwerty', 'qwa-,', 'qw-space', 'qwa-.']
      ],
      defaultKC: SIPKEY.ANYTEXT
    },
    numpadkana: {
      frames: [
        'numpadkana'
      ],
      data: [
        ['np-a', 'np-ka', 'np-sa'],
        ['np-ta', 'np-na', 'np-ha'],
        ['np-ma', 'np-ya', 'np-ra'],
        ['sw-12kana', 'np-mod', 'np-wa', 'np-.']
      ],
      defaultKC: SIPKEY.JPN12_KANA
    },
    symbolview: {
      frames: [
        'symbolemoji'
      ],
      data: [
        [ 'sw-symbol', 'sm-0', 'sm-1', 'sm-2', 'sm-3', 'sm-4', 'sm-5', 'sm-6' ]
      ],
      candType: 'gird'
    },
    emojiview: {
      frames: [
        'symbolemoji'
      ],
      data: [
        [ 'sw-emoji', 'em-0', 'em-1', 'em-2', 'em-3', 'em-4', 'em-5', 'em-6' ]
      ],
      candType: 'gird'
    }
  },
  frames: {
    qwerty: {
      data: [
        {l: [], r: []},
        {l: [], r: []},
        {l: ['shift'], r: ['qw-back']},
        {l: [], r: ['enter']}
      ]
    },
    numpadkana: {
      data: [
        {l: ['np-left'], r: ['np-right']},
        {l: ['np-bcktgl'], r: ['back']},
        {l: ['sw-quit'], r: ['np-space']},
        {l: [], r: ['enter']}
      ]
    },
    symbolemoji: {
      data: [
        {l: [], r: ['back', 'enter']}
      ]
    }
  }
}
