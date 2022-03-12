#include "pch.h"
#include "InputModel.h"
#include "KeyEventProcessor.h"
#include "EditFunctions.h"

namespace Ribbon {
namespace {

const uint16_t WITH_IMEOFF = 4;
const uint16_t WITH_CONTROL = 2;
const uint16_t WITH_SHIFT = 1;
const uint32_t ON_NOCOMPOSITION = 1;
const uint32_t ON_CHARACTERCURSOR = 2;
const uint32_t ON_CLAUSECURSOR = 4;
const uint32_t ON_CANDIDATECURSOR = 8;

typedef bool (*KeyHandlerType)(IEditContext*, const std::shared_ptr<KeyEvent>&);

struct KeyAssignEntry
{
    uint16_t modifierBits;
    int sipKeyCode;
	uint32_t stateBits;
	KeyHandlerType handler;
};

/* This table is only for JPN */
static KeyAssignEntry s_keyAssignTable[] = {
/*                                     + No input
         + IME OFF                        |  + Character Cursor
         |  + CONTROL                     |  |  + Clauase Cursor
         |  |  + SHIFT                    |  |  |  + Candidate Cursor
         |  |  |                          |  |  |  |                  */
#if 0
    { 0       +1 , SIPKEY_KATAHIRA ,     +0             , EditFunction::ReconversionKatakana  },
    { 0          , SIPKEY_EISU     ,     +0             , EditFunction::ReconversionAlphaHalf },
    { 0          , SIPKEY_F10      ,     +0             , EditFunction::ReconversionAlphaHalf },
    { 0          , SIPKEY_F6       ,     +0             , EditFunction::ReconversionHiragana  },
    { 0          , SIPKEY_F7       ,     +0             , EditFunction::ReconversionKatakana  },
    { 0          , SIPKEY_F8       ,     +0             , EditFunction::ReconversionKanaHalf  },
    { 0          , SIPKEY_F9       ,     +0             , EditFunction::ReconversionAlphaFull },
    { 0          , SIPKEY_HENKAN   ,     +0             , EditFunction::ReconversionDefault   },
    { 0          , SIPKEY_KATAHIRA ,     +0             , EditFunction::ReconversionHiragana  },
    { 0          , SIPKEY_SPACE    ,     +0             , EditFunction::ReconversionDefault   },
#endif
    { 0 +4 +2    , SIPKEY_SPACE    ,     +1             , EditFunction::ModeOnOff             },
    { 0 +4       , SIPKEY_ESCAPE   ,     +1             , EditFunction::CandidateCancel       },
    { 0 +4    +1 , SIPKEY_ENTER    ,     +1             , EditFunction::ModeOnOff             },
    { 0    +2    , SIPKEY_SPACE    ,     +1             , EditFunction::ModeOnOff             },
    { 0    +2 +1 , SIPKEY_SPACE    ,     +1             , EditFunction::CaretInsertFullSpace  },
    { 0    +2    , SIPKEY_BACK     ,     +1             , EditFunction::ReconversionUndo      },
    { 0    +2    , SIPKEY_F10      ,     +1             , EditFunction::ExternalPropertyMenu  },
    { 0    +2    , SIPKEY_F7       ,     +1             , EditFunction::ExternalWordDialog    },
    { 0    +2    , SIPKEY_HENKAN   ,     +1             , EditFunction::ExternalPropertyMenu  },
    { 0       +1 , SIPKEY_KATAHIRA ,     +1             , EditFunction::ModeKatakana          },
    { 0       +1 , SIPKEY_MUHENKAN ,     +1             , EditFunction::ModeAlphabetToggle    },
    { 0       +1 , SIPKEY_SPACE    ,     +1             , EditFunction::CaretInsertSpaceR     },
    { 0          , SIPKEY_EISU     ,     +1             , EditFunction::ModeKanaLock          },
    { 0          , SIPKEY_KATAHIRA ,     +1             , EditFunction::ModeHiragana          },
    { 0          , SIPKEY_MUHENKAN ,     +1             , EditFunction::ModeKanaToggle        },
    { 0          , SIPKEY_ROMAN    ,     +1             , EditFunction::ModeRomanToggle       },
    { 0          , SIPKEY_SPACE    ,     +1             , EditFunction::CaretInsertSpace      },
    { 0          , SIPKEY_ZENHAN   ,     +1             , EditFunction::ModeOnOff             },
    { 0    +2 +1 , SIPKEY_SPACE    ,         +2         , EditFunction::CaretInsertFullSpace  },
    { 0    +2    , SIPKEY_A        ,         +2         , EditFunction::CaretTop              },
    { 0    +2    , SIPKEY_BACK     ,         +2         , EditFunction::CaretBackspace        },
    { 0    +2    , SIPKEY_D        ,         +2         , EditFunction::CaretRight            },
    { 0    +2    , SIPKEY_DOWN     ,         +2         , EditFunction::CaretTop              },
    { 0    +2    , SIPKEY_ENTER    ,         +2         , EditFunction::AllDetermine          },
    { 0    +2    , SIPKEY_F        ,         +2         , EditFunction::CaretBottom           },
    { 0    +2    , SIPKEY_F8       ,         +2         , EditFunction::ExternalSearchDefault },
    { 0    +2    , SIPKEY_F9       ,         +2         , EditFunction::ExternalSearchBy      },
    { 0    +2    , SIPKEY_G        ,         +2         , EditFunction::CaretDelete           },
    { 0    +2    , SIPKEY_H        ,         +2         , EditFunction::CaretBackspace        },
    { 0    +2    , SIPKEY_HENKAN   ,         +2         , EditFunction::ExternalPropertyMenu  },
    { 0    +2    , SIPKEY_I        ,         +2         , EditFunction::ReplaceKatakana       },
    { 0    +2    , SIPKEY_K        ,         +2         , EditFunction::CaretLeft             },
    { 0    +2    , SIPKEY_L        ,         +2         , EditFunction::CaretRight            },
    { 0    +2    , SIPKEY_LEFT     ,         +2         , EditFunction::CaretTop              },
    { 0    +2    , SIPKEY_M        ,         +2         , EditFunction::AllDetermine          },
    { 0    +2    , SIPKEY_N        ,         +2         , EditFunction::CaretTop              },
    { 0    +2    , SIPKEY_O        ,         +2         , EditFunction::ReplaceKanaHalf       },
    { 0    +2    , SIPKEY_P        ,         +2         , EditFunction::ReplaceAlphaFull      },
    { 0    +2    , SIPKEY_RIGHT    ,         +2         , EditFunction::CaretBottom           },
    { 0    +2    , SIPKEY_S        ,         +2         , EditFunction::CaretLeft             },
    { 0    +2    , SIPKEY_SPACE    ,         +2         , EditFunction::CaretInsertHalfSpace  },
    { 0    +2    , SIPKEY_T        ,         +2         , EditFunction::RepalceAlphaHalf      },
    { 0    +2    , SIPKEY_U        ,         +2         , EditFunction::ReplaceHiragana       },
    { 0    +2    , SIPKEY_UP       ,         +2         , EditFunction::CaretRight            },
    { 0    +2    , SIPKEY_Y        ,         +2         , EditFunction::ExternalCodeInput     },
    { 0    +2    , SIPKEY_Z        ,         +2         , EditFunction::AllClear              },
    { 0       +1 , SIPKEY_BACK     ,         +2         , EditFunction::CaretBackspace        },
    { 0       +1 , SIPKEY_DOWN     ,         +2         , EditFunction::CaretDetermineChar    },
    { 0       +1 , SIPKEY_ESCAPE   ,         +2         , EditFunction::AllClear              },
    { 0       +1 , SIPKEY_KATAHIRA ,         +2         , EditFunction::ModeKatakana          },
    { 0       +1 , SIPKEY_LEFT     ,         +2         , EditFunction::CaretLeft             },
    { 0       +1 , SIPKEY_MUHENKAN ,         +2         , EditFunction::ReplaceAlphaToggle    },
    { 0       +1 , SIPKEY_RIGHT    ,         +2         , EditFunction::CaretRight            },
    { 0       +1 , SIPKEY_SPACE    ,         +2         , EditFunction::ReplaceConversion     },
    { 0          , SIPKEY_BACK     ,         +2         , EditFunction::CaretBackspace        },
    { 0          , SIPKEY_DELETE   ,         +2         , EditFunction::CaretDelete           },
    { 0          , SIPKEY_EISU     ,         +2         , EditFunction::ModeKanaLock          },
    { 0          , SIPKEY_END      ,         +2         , EditFunction::CaretBottom           },
    { 0          , SIPKEY_ENTER    ,         +2         , EditFunction::AllDetermine          },
    { 0          , SIPKEY_ESCAPE   ,         +2         , EditFunction::AllClear              },
    { 0          , SIPKEY_F10      ,         +2         , EditFunction::RepalceAlphaHalf      },
    { 0          , SIPKEY_F5       ,         +2         , EditFunction::ExternalCodeInput     },
    { 0          , SIPKEY_F6       ,         +2         , EditFunction::ReplaceHiragana       },
    { 0          , SIPKEY_F7       ,         +2         , EditFunction::ReplaceKatakana       },
    { 0          , SIPKEY_F8       ,         +2         , EditFunction::ReplaceKanaHalf       },
    { 0          , SIPKEY_F9       ,         +2         , EditFunction::ReplaceAlphaFull      },
    { 0          , SIPKEY_HENKAN   ,         +2         , EditFunction::ReplaceConversion     },
    { 0          , SIPKEY_HOME     ,         +2         , EditFunction::CaretTop              },
    { 0          , SIPKEY_KANJI    ,         +2         , EditFunction::ReplaceConversion     },
    { 0          , SIPKEY_KATAHIRA ,         +2         , EditFunction::ModeHiragana          },
    { 0          , SIPKEY_LEFT     ,         +2         , EditFunction::CaretLeft             },
    { 0          , SIPKEY_MUHENKAN ,         +2         , EditFunction::RepalceKanaToggle     },
    { 0          , SIPKEY_RIGHT    ,         +2         , EditFunction::CaretRight            },
    { 0          , SIPKEY_ROMAN    ,         +2         , EditFunction::ReplaceWidthToggle    },
    { 0          , SIPKEY_SPACE    ,         +2         , EditFunction::ReplaceConversion     },
    { 0          , SIPKEY_TAB      ,         +2         , EditFunction::CandidateViewMove     },
    { 0          , SIPKEY_ZENHAN   ,         +2         , EditFunction::CaretDetermineOff     },
    { 0    +2    , SIPKEY_A        ,             +4     , EditFunction::ClauseTop             },
    { 0    +2    , SIPKEY_BACK     ,             +4     , EditFunction::ClauseCancel          },
    { 0    +2    , SIPKEY_D        ,             +4     , EditFunction::ClauseRight           },
    { 0    +2    , SIPKEY_DOWN     ,             +4     , EditFunction::ClauseDetermineCurrent},
    { 0    +2    , SIPKEY_E        ,             +4     , EditFunction::ClauseShorten         },
    { 0    +2    , SIPKEY_ENTER    ,             +4     , EditFunction::AllDetermine          },
    { 0    +2    , SIPKEY_F        ,             +4     , EditFunction::ClauseBottom          },
    { 0    +2    , SIPKEY_F8       ,             +4     , EditFunction::ExternalSearchDefault },
    { 0    +2    , SIPKEY_F9       ,             +4     , EditFunction::ExternalSearchBy      },
    { 0    +2    , SIPKEY_G        ,             +4     , EditFunction::ClauseCancel          },
    { 0    +2    , SIPKEY_H        ,             +4     , EditFunction::ClauseCancel          },
    { 0    +2    , SIPKEY_HENKAN   ,             +4     , EditFunction::ExternalPropertyMenu  },
    { 0    +2    , SIPKEY_I        ,             +4     , EditFunction::ReplaceKatakana       },
    { 0    +2    , SIPKEY_K        ,             +4     , EditFunction::ClauseShorten         },
    { 0    +2    , SIPKEY_L        ,             +4     , EditFunction::ClauseExtend          },
    { 0    +2    , SIPKEY_LEFT     ,             +4     , EditFunction::ClauseTop             },
    { 0    +2    , SIPKEY_M        ,             +4     , EditFunction::AllDetermine          },
    { 0    +2    , SIPKEY_N        ,             +4     , EditFunction::ClauseDetermineCurrent},
    { 0    +2    , SIPKEY_O        ,             +4     , EditFunction::ReplaceHalfWidth      },
    { 0    +2    , SIPKEY_P        ,             +4     , EditFunction::ReplaceAlphaFull      },
    { 0    +2    , SIPKEY_RIGHT    ,             +4     , EditFunction::ClauseBottom          },
    { 0    +2    , SIPKEY_S        ,             +4     , EditFunction::ClauseLeft            },
    { 0    +2    , SIPKEY_T        ,             +4     , EditFunction::RepalceAlphaHalf      },
    { 0    +2    , SIPKEY_U        ,             +4     , EditFunction::ReplaceHiragana       },
    { 0    +2    , SIPKEY_UP       ,             +4     , EditFunction::CandidatePrev         },
    { 0    +2    , SIPKEY_X        ,             +4     , EditFunction::ClauseExtend          },
    { 0    +2    , SIPKEY_Y        ,             +4     , EditFunction::ExternalCodeInput     },
    { 0    +2    , SIPKEY_Z        ,             +4     , EditFunction::ClauseCancel          },
    { 0       +1 , SIPKEY_BACK     ,             +4     , EditFunction::ClauseCancel          },
    { 0       +1 , SIPKEY_DOWN     ,             +4     , EditFunction::ClauseDetermineTop    },
    { 0       +1 , SIPKEY_ESCAPE   ,             +4     , EditFunction::AllCancel             },
    { 0       +1 , SIPKEY_HENKAN   ,             +4     , EditFunction::CandidatePrev         },
    { 0       +1 , SIPKEY_KATAHIRA ,             +4     , EditFunction::ModeKatakana          },
    { 0       +1 , SIPKEY_LEFT     ,             +4     , EditFunction::ClauseShorten         },
    { 0       +1 , SIPKEY_MUHENKAN ,             +4     , EditFunction::ReplaceAlphaToggle    },
    { 0       +1 , SIPKEY_RIGHT    ,             +4     , EditFunction::ClauseExtend          },
    { 0       +1 , SIPKEY_SPACE    ,             +4     , EditFunction::CandidatePrev         },
    { 0          , SIPKEY_BACK     ,             +4     , EditFunction::ClauseCancel          },
    { 0          , SIPKEY_DELETE   ,             +4     , EditFunction::ClauseCancel          },
    { 0          , SIPKEY_DOWN     ,             +4     , EditFunction::ClauseExtend          },
    { 0          , SIPKEY_END      ,             +4     , EditFunction::ClauseBottom          },
    { 0          , SIPKEY_ENTER    ,             +4     , EditFunction::AllDetermine          },
    { 0          , SIPKEY_ESCAPE   ,             +4     , EditFunction::ClauseCancel          },
    { 0          , SIPKEY_F10      ,             +4     , EditFunction::RepalceAlphaHalf      },
    { 0          , SIPKEY_F5       ,             +4     , EditFunction::ExternalCodeInput     },
    { 0          , SIPKEY_F6       ,             +4     , EditFunction::ReplaceHiragana       },
    { 0          , SIPKEY_F7       ,             +4     , EditFunction::ReplaceKatakana       },
    { 0          , SIPKEY_F8       ,             +4     , EditFunction::ReplaceHalfWidth      },
    { 0          , SIPKEY_F9       ,             +4     , EditFunction::ReplaceAlphaFull      },
    { 0          , SIPKEY_HENKAN   ,             +4     , EditFunction::ClauseConversion      },
    { 0          , SIPKEY_HOME     ,             +4     , EditFunction::ClauseTop             },
    { 0          , SIPKEY_KANJI    ,             +4     , EditFunction::ClauseConversion      },
    { 0          , SIPKEY_KATAHIRA ,             +4     , EditFunction::ModeHiragana          },
    { 0          , SIPKEY_LEFT     ,             +4     , EditFunction::ClauseLeft            },
    { 0          , SIPKEY_MUHENKAN ,             +4     , EditFunction::RepalceKanaToggle     },
    { 0          , SIPKEY_RIGHT    ,             +4     , EditFunction::ClauseRight           },
    { 0          , SIPKEY_ROMAN    ,             +4     , EditFunction::ReplaceWidthToggle    },
    { 0          , SIPKEY_SPACE    ,             +4     , EditFunction::ClauseConversion      },
    { 0          , SIPKEY_UP       ,             +4     , EditFunction::ClauseShorten         },
    { 0          , SIPKEY_ZENHAN   ,             +4     , EditFunction::CaretDetermineOff     },
    { 0    +2    , SIPKEY_BACK     ,                 +8 , EditFunction::ClauseCancel          },
    { 0    +2    , SIPKEY_D        ,                 +8 , EditFunction::CandidateNextPage     },
    { 0    +2    , SIPKEY_DOWN     ,                 +8 , EditFunction::ClauseDetermineCurrent},
    { 0    +2    , SIPKEY_E        ,                 +8 , EditFunction::CandidatePrev         },
    { 0    +2    , SIPKEY_ENTER    ,                 +8 , EditFunction::AllDetermine          },
    { 0    +2    , SIPKEY_F8       ,                 +8 , EditFunction::ExternalSearchDefault },
    { 0    +2    , SIPKEY_F9       ,                 +8 , EditFunction::ExternalSearchBy      },
    { 0    +2    , SIPKEY_G        ,                 +8 , EditFunction::CandidateCancel       },
    { 0    +2    , SIPKEY_H        ,                 +8 , EditFunction::CandidateCancel       },
    { 0    +2    , SIPKEY_HENKAN   ,                 +8 , EditFunction::ClauseOpenRead        },
    { 0    +2    , SIPKEY_M        ,                 +8 , EditFunction::CandidateDetermine    },
    { 0    +2    , SIPKEY_N        ,                 +8 , EditFunction::ClauseDetermineCurrent},
    { 0    +2    , SIPKEY_S        ,                 +8 , EditFunction::CandidatePrevPage     },
    { 0    +2    , SIPKEY_X        ,                 +8 , EditFunction::CandidateNext         },
    { 0       +1 , SIPKEY_BACK     ,                 +8 , EditFunction::ClauseCancel          },
    { 0       +1 , SIPKEY_DOWN     ,                 +8 , EditFunction::CandidateNextGroup    },
    { 0       +1 , SIPKEY_ESCAPE   ,                 +8 , EditFunction::AllCancel             },
    { 0       +1 , SIPKEY_HENKAN   ,                 +8 , EditFunction::CandidatePrev         },
    { 0       +1 , SIPKEY_SPACE    ,                 +8 , EditFunction::CandidatePrev         },
    { 0          , SIPKEY_BACK     ,                 +8 , EditFunction::CandidateCancel       },
    { 0          , SIPKEY_DELETE   ,                 +8 , EditFunction::CandidateCancel       },
    { 0          , SIPKEY_DOWN     ,                 +8 , EditFunction::CandidateNext         },
    { 0          , SIPKEY_END      ,                 +8 , EditFunction::CandidateBottom       },
    { 0          , SIPKEY_ENTER    ,                 +8 , EditFunction::CandidateDetermine    },
    { 0          , SIPKEY_ESCAPE   ,                 +8 , EditFunction::CandidateCancel       },
    { 0          , SIPKEY_HENKAN   ,                 +8 , EditFunction::CandidateNext         },
    { 0          , SIPKEY_HOME     ,                 +8 , EditFunction::CandidateTop          },
    { 0          , SIPKEY_KANJI    ,                 +8 , EditFunction::CandidateSecond       },
    { 0          , SIPKEY_LEFT     ,                 +8 , EditFunction::CandidatePrevPage     },
    { 0          , SIPKEY_PAGEDOWN ,                 +8 , EditFunction::CandidateNextGroup    },
    { 0          , SIPKEY_PAGEUP   ,                 +8 , EditFunction::CandidatePrevGroup    },
    { 0          , SIPKEY_RIGHT    ,                 +8 , EditFunction::CandidateNextPage     },
    { 0          , SIPKEY_SPACE    ,                 +8 , EditFunction::CandidateNext         },
    { 0          , SIPKEY_TAB      ,                 +8 , EditFunction::CanddaiteChangeView   },
    { 0          , SIPKEY_UP       ,                 +8 , EditFunction::CandidatePrev         },
	{ 0          , SIPKEY_JPN12_KANA   , +1  +2  +4  +8 , EditFunction::Jpn12keyKanaInput     },
	{ 0          , SIPKEY_JPN12_MOD    , +1  +2  +4  +8 , EditFunction::Jpn12keyModifierInput },
	{ 0          , SIPKEY_JPN12_BRK    , +1  +2  +4  +8 , EditFunction::Jpn12keyBreakersInput },
	{ 0          , SIPKEY_JPN12_BCKTGL , +1  +2         , EditFunction::Jpn12keyBackToggleInput },
	{ 0          , SIPKEY_SWITCH   ,     +1 + 2 + 4 + 8 , EditFunction::ChangeSIPLayout       },
	{ 0          , SIPKEY_CATEGORY ,     +1  +2  +4  +8 , EditFunction::ChangeCategory        },
};
} // anonymous namespace

struct KeyEventProcessor :
	public std::enable_shared_from_this<KeyEventProcessor>,
	public IKeyEventProcessor,
	public IObject
{
	std::shared_ptr<IKeyCodeHelper> m_keyCodeHelper;
	std::vector<const KeyAssignEntry*> m_sortedEntry;

	struct KeyAssignIndex {
		const KeyAssignEntry** topAddr;
		uint16_t sipKeyCode;
		uint16_t countOfEntry;
	};
	std::vector<KeyAssignIndex> m_keyAssignTable;

	KeyEventProcessor()
	{
		size_t entryCount = COUNT_OF(s_keyAssignTable);
		m_sortedEntry.reserve(entryCount);
		for (const auto& keyAssignEntry: s_keyAssignTable) {
			m_sortedEntry.emplace_back(&keyAssignEntry);
		}
		std::sort(m_sortedEntry.begin(), m_sortedEntry.end(),
			[](const KeyAssignEntry* lhs, const KeyAssignEntry* rhs) -> bool {
				if (lhs->sipKeyCode != rhs->sipKeyCode) {
					return lhs->sipKeyCode < rhs->sipKeyCode;
				}
				return lhs < rhs; // keep order
			});

		size_t keyTopIndex = 0;
		int sipKeyCode = m_sortedEntry[0]->sipKeyCode;
		for (size_t idx = 1; idx < entryCount; ++idx) {
			if (m_sortedEntry[idx]->sipKeyCode == sipKeyCode) {
				continue;
			}
			m_keyAssignTable.emplace_back(KeyAssignIndex());
			m_keyAssignTable.back().topAddr = &m_sortedEntry[keyTopIndex];
			m_keyAssignTable.back().countOfEntry = static_cast<uint16_t>(idx - keyTopIndex);
			m_keyAssignTable.back().sipKeyCode = static_cast<uint16_t>(sipKeyCode);

			keyTopIndex = idx;
			sipKeyCode = m_sortedEntry[idx]->sipKeyCode;
		}
		// last item
		m_keyAssignTable.emplace_back(KeyAssignIndex());
		m_keyAssignTable.back().topAddr = &m_sortedEntry[keyTopIndex];
		m_keyAssignTable.back().countOfEntry = static_cast<uint16_t>(entryCount - keyTopIndex);
		m_keyAssignTable.back().sipKeyCode = static_cast<uint16_t>(sipKeyCode);

		// helper
		m_keyCodeHelper = FACTORYCREATE(KeyCodeHelper);
	}

	KeyHandlerType FindHandler(IEditContext* editContext, bool isControl, bool isShift, bool isIMEOff, int sipKeyCode)
	{
		KeyAssignIndex index;
		index.sipKeyCode = static_cast<uint16_t>(sipKeyCode);
		auto it = std::lower_bound(m_keyAssignTable.begin(), m_keyAssignTable.end(), index,
			[](const KeyAssignIndex& lhs, const KeyAssignIndex& rhs) {
				return lhs.sipKeyCode < rhs.sipKeyCode;
			});
		if (it == m_keyAssignTable.end() || it->sipKeyCode != static_cast<uint16_t>(sipKeyCode)) {
			return EditFunction::UnknownKeyHandler;
		}

		uint16_t modifierBits = (isControl ? WITH_CONTROL : 0U) + (isShift ? WITH_SHIFT : 0U) + (isIMEOff ? WITH_IMEOFF : 0U);
		uint32_t stateBits = editContext->IsSelectionRange() ? 0 :
			((editContext->IsNoComposition() ? ON_NOCOMPOSITION : 0) +
			(editContext->IsCharacterCaret() ? ON_CHARACTERCURSOR : 0) +
			(editContext->IsClauseCaret() ? ON_CLAUSECURSOR : 0) +
			(editContext->IsCandiateCaret() ? ON_CANDIDATECURSOR : 0));

		for (size_t entry = 0; entry < it->countOfEntry; ++entry) {
			if (it->topAddr[entry]->modifierBits == modifierBits &&
				((it->topAddr[entry]->stateBits & stateBits) != 0)) {
				return it->topAddr[entry]->handler;
			}
		}
		return EditFunction::FallbackHandler;
	}

	// IKeyEvent
	const long long c_repeastThreshold = 1000;
	std::shared_ptr<KeyEvent> m_lastKeyEvent;
	std::chrono::time_point<std::chrono::system_clock> m_lastKeyTime;
	std::chrono::time_point<std::chrono::system_clock> m_currentKeyTime;
	uint32_t m_modifierState = 0;

	uint32_t ModifierState() override {
		return m_modifierState;
	}
	void ModifierState(uint32_t newState) override {
		m_modifierState = newState;
	}

	bool CompleteKeyEvent(const std::shared_ptr<KeyEvent>& keyEvent)
	{
		if (keyEvent->sipKeyCode <= 0) {
			// HWKB
			keyEvent->sipKeyCode = m_keyCodeHelper->OSKeyToSIPKey(keyEvent->osKeyCode);
			switch (keyEvent->sipKeyCode) {
			case SIPKEY_LSHIFT:
			case SIPKEY_RSHIFT:
				if (keyEvent->eventType == BaseEvent::EventType::KeyDown) {
					m_modifierState |= SHIFT_ON;
				}
				else if (keyEvent->eventType == BaseEvent::EventType::KeyUp) {
					m_modifierState &= ~SHIFT_ON;
				}
				return true;
			case SIPKEY_LCTRL:
			case SIPKEY_RCTRL:
				if (keyEvent->eventType == BaseEvent::EventType::KeyDown) {
					m_modifierState |= CONTROL_ON;
				}
				else if (keyEvent->eventType == BaseEvent::EventType::KeyUp) {
					m_modifierState &= ~CONTROL_ON;
				}
				return true;
			case SIPKEY_CAPSLOCK:
				if (keyEvent->eventType == BaseEvent::EventType::KeyDown) {
					m_modifierState ^= CAPS_ON;
				}
				return true;
			case SIPKEY_LALT:
			case SIPKEY_RALT:
				if (keyEvent->eventType == BaseEvent::EventType::KeyDown) {
					m_modifierState |= ALT_ON;
				}
				else if (keyEvent->eventType == BaseEvent::EventType::KeyUp) {
					m_modifierState &= ~ALT_ON;
				}
				return true;
			}
			keyEvent->modifierState = m_modifierState;
		}
		if (keyEvent->osKeyCode <= 0) {
			keyEvent->osKeyCode = m_keyCodeHelper->SIPKeyToOSKey(keyEvent->sipKeyCode);
		}
		if (keyEvent->eventType == BaseEvent::EventType::KeyDown) {
			auto currentKeyTime = std::chrono::system_clock::now();
			auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentKeyTime - m_lastKeyTime).count();
			if (diffTime < c_repeastThreshold) {
				if (m_lastKeyEvent->IsEqualTo(keyEvent)) {
					keyEvent->isRepeatedKey = true;
				}
			}
			m_lastKeyTime = std::chrono::system_clock::now();
			m_lastKeyEvent = keyEvent;
		}
		return false;
	}

	// ITaskProcessor
	void ProcessTask(TaskType taskType, const std::shared_ptr<BaseEvent>& baseEvent, IEditContext* editContext) override
	{
		switch (taskType)
		{
		case TaskType::KeyDown:
			Handle_KeyDown(taskType, baseEvent, editContext);
			return;

		case TaskType::KeyUp:
			Handle_KeyUp(taskType, baseEvent, editContext);
			return;
		}
	}

	// Handlers
	void Handle_KeyDown(TaskType /*taskType*/, const std::shared_ptr<BaseEvent>& baseEvent, IEditContext* editContext)
	{
		THROW_IF_FALSE(baseEvent->eventType == BaseEvent::EventType::KeyDown);
		auto keyEvent = std::static_pointer_cast<KeyEvent>(baseEvent);
		if (CompleteKeyEvent(keyEvent)) {
			// just modifier change key
			editContext->SetPassthroughKey(keyEvent->osKeyCode);
			return;
		}

		// Key event dispatcher
		bool withControl = (m_modifierState & CONTROL_ON) != 0;
		bool withShift = (m_modifierState & SHIFT_ON) != 0;
		bool withIMEOff = keyEvent->isHardwareKey ? ((m_modifierState & IME_ON) == 0) : false;
		auto keyHandler = FindHandler(editContext, withControl, withShift, withIMEOff, keyEvent->sipKeyCode);

		bool handled = false;
		if (keyHandler) {
			if (keyHandler(editContext, keyEvent)) {
				handled = true;
			}
		}
		if (!handled) {
			editContext->SetPassthroughKey(keyEvent->osKeyCode);
		}
	}

	void Handle_KeyUp(TaskType /*taskType*/, const std::shared_ptr<BaseEvent>& baseEvent, IEditContext* /*editContext*/)
	{
		THROW_IF_FALSE(baseEvent->eventType == BaseEvent::EventType::KeyUp);
		auto keyEvent = std::static_pointer_cast<KeyEvent>(baseEvent);
		CompleteKeyEvent(keyEvent);
	}

	virtual ~KeyEventProcessor() {}
	KeyEventProcessor& operator = (const KeyEventProcessor&) = delete;
	IOBJECT_COMMON_METHODS
};

FACTORYDEFINE(KeyEventProcessor);
} // Ribbon
