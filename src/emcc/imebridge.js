'use strict';

var Module = {
	noInitialRun: true,
	noExitRuntime: true,

	onRuntimeInitialized: function() {
		console.log('onRuntimeInitialized');
		Module['imeInitialize']();

		window.MediatorCall = function (jsonArg) {
			var jsonArgUtf8 = unescape(encodeURIComponent(jsonArg));
        	Module['imeJsonCommand'](jsonArgUtf8);
			var canidateStateUtf8 = Module['imeCandidateState']();
			var candidateState = decodeURIComponent(escape(canidateStateUtf8));
			window.MediatorUpdateCandidate(candidateState);

			var compositionStateUtf8 = Module['imeCompositionState']();
			var compositionState = decodeURIComponent(escape(compositionStateUtf8));
			console.log(compositionState);
		};
	},

	// preRun: [],
	// postRun: [],
	print: function(text) {
		console.log(text);
	},
	printErr: function(text) {
		console.error(text);
	}
};


/**
 * Namespace for this extension.
 */
var ribbonIme = window.ribbonIme || {};

/**
 * Ribbon IME with IME extension API.
 * @constructor
 */
ribbonIme.RibbonIme = function() {
  /**
   * Context information which is provided by Chrome.
   * @type {Object}
   * @private
   */
  this.context_ = null;

  /**
   * Engine id which is specified on manifest.js.
   * @type {string}
   * @private
   */
  this.engineID_ = '';

  // Some properties are initialized on
  // {@code ribbonIme.RibbonIme.clear_} and
  // {@code ribbonIme.RibbonIme.initializeMenuItems_}.
  this.clear_();
  this.initializeMenuItems_();

  var ime = this;
  chrome.input.ime.onActivate.addListener(function(engineID) {
        ime.onActivate(engineID); });
  chrome.input.ime.onDeactivated.addListener(function(engineID) {
        ime.onDeactivated(engineID); });
  chrome.input.ime.onFocus.addListener(function(context) {
        ime.onFocus(context); });
  chrome.input.ime.onBlur.addListener(function(contextID) {
        ime.onBlur(contextID); });
  chrome.input.ime.onInputContextUpdate.addListener(function(context) {
        ime.onInputContextUpdate(context); });
  chrome.input.ime.onKeyEvent.addListener(function(engineID, keyData) {
        return ime.onKeyEvent(engineID, keyData); });
  chrome.input.ime.onCandidateClicked.addListener(function(engineID, candidateID, button) {
        ime.onCandidateClicked(engineID, candidateID, button); });
  chrome.input.ime.onMenuItemActivated.addListener(function(engineID, name) {
        ime.onMenuItemActivated(engineID, name); });
};

/**
 * Stringifies key event data.
 * @param {!Object} keyData Key event data.
 * @return {string} Stringified key event data.
 * @private
 */
ribbonIme.RibbonIme.prototype.stringifyKeyAndModifiers_ =
    function(keyData) {
  var keys = [keyData.key];
  if (keyData.altKey) { keys.push('alt'); }
  if (keyData.ctrlKey) { keys.push('ctrl'); }
  if (keyData.shiftKey) { keys.push('shift'); }
  return keys.join(' ');
};

/**
 * Ignorable key set to determine we handle the key event or not.
 * @type {!Object.<boolean>}
 * @private
 */
ribbonIme.RibbonIme.IGNORABLE_KEY_SET_ = (function() {
  var IGNORABLE_KEYS = [
    {  // PrintScreen shortcut.
      key: 'ChromeOSSwitchWindow',
      ctrlKey: true
    }, {  // PrintScreen shortcut.
      key: 'ChromeOSSwitchWindow',
      ctrlKey: true,
      shiftKey: true
    }
  ];

  var ignorableKeySet = [];
  for (var i = 0; i < IGNORABLE_KEYS.length; ++i) {
    var key = ribbonIme.RibbonIme.prototype.
        stringifyKeyAndModifiers_(IGNORABLE_KEYS[i]);
    ignorableKeySet[key] = true;
  }

  return ignorableKeySet;
})();

/**
 * Page size of a candidate list.
 * This value should not be greater than 12 since we use Roman number to
 * indicates the candidate number on the list.
 * @type {number}
 * @private
 */
ribbonIme.RibbonIme.PAGE_SIZE_ = 5;

/**
 * Enum of IME state.
 * @enum {number}
 */
ribbonIme.RibbonIme.State = {
  /** IME doesn't have any input text. */
  PRECOMPOSITION: 0,
  /**
   * IME has a input text, but no candidate are expressly selected or input text
   * is not segmented.
   */
  COMPOSITION: 1,
  /**
   * IME has a input text, and one of the candidate is selected or input text is
   * segmentated.
   */
  CONVERSION: 2
};

/**
 * Segment information of a composition text.
 * @constructor
 */
ribbonIme.RibbonIme.Segment = function() {
  /**
   * Start position of the segment.
   * @type {number}
   */
  this.start = 0;

  /**
   * Candidates list.
   * @type {!Array.<string>}
   */
  this.candidates = [];

  /**
   * Focused candidate index.
   * @type {number}
   */
  this.focusedIndex = 0;
};

/**
 * Initializes menu items and some member variables.
 * @private
 */
ribbonIme.RibbonIme.prototype.initializeMenuItems_ = function() {
  var menuItems = [];
  var callbacks = {};
  var that = this;

  /**
   * Indicates IME suggests dummy candidates or not.
   * @type {boolean}
   * @private
   */
  this.useDummyCandidates_ = true;

  var radioItems = [{
    id: 'enable_dummy_candidates',
    label: 'Enable dummy candidates',
    style: 'radio',
    checked: true,
    enabled: true,
    visible: true
  }, {
    id: 'disable_dummy_candidates',
    label: 'Disable dummy candidates',
    style: 'radio',
    checked: false,
    enabled: true,
    visible: true
  }];

  for (var i = 0; i < radioItems.length; ++i) {
    callbacks[radioItems[i].id] = (function(index) {
      return function() {
        for (var j = 0; j < radioItems.length; ++j) {
          var isChecked = (index == j);
          radioItems[j].checked = isChecked;
        }
        var isEnabled = index == 0;
        that.useDummyCandidates_ = isEnabled;
      };
    })(i);
    menuItems.push(radioItems[i]);
  }

  var enableRadioMenuItem = {
    id: 'enable_radio_menu',
    label: 'Enable radio menu',
    style: 'check',
    checked: true
  };
  menuItems.push(enableRadioMenuItem);
  callbacks[enableRadioMenuItem.id] = function() {
    enableRadioMenuItem.checked = !enableRadioMenuItem.checked;
    for (var i = 0; i < radioItems.length; ++i) {
      radioItems[i].enabled = enableRadioMenuItem.checked;
    }
  };

  var displayRadioMenuItem = {
    id: 'display_radio_menu',
    label: 'Display radio menu',
    style: 'check',
    checked: true
  };

  menuItems.push(displayRadioMenuItem);
  callbacks[displayRadioMenuItem.id] = function() {
    displayRadioMenuItem.checked = !displayRadioMenuItem.checked;
    for (var i = 0; i < radioItems.length; ++i) {
      radioItems[i].visible = displayRadioMenuItem.checked;
    }
  };

  /**
   * Menu items of this IME.
   */
  this.menuItems_ = menuItems;

  /**
   * Callback function table which is called when menu item is clicked.
   */
  this.menuItemCallbackTable_ = callbacks;
};

/**
 * Clears properties of IME.
 * @private
 */
ribbonIme.RibbonIme.prototype.clear_ = function() {
  /**
   * Raw input text.
   * @type {string}
   * @private
   */
  this.inputText_ = '';

  /**
   * Commit text.
   * This is a volatile property, and will be cleared by
   * {@code ribbonIme.RibbonIme.updateCommitText_}.
   * @type {?string}
   * @private
   */
  this.commitText_ = null;

  /**
   * Segments information.
   * @type {!Array.<!ribbonIme.RibbonIme.Segment>}
   * @private
   */
  this.segments_ = [];

  /**
   * Cursor position.
   * @type {number}
   * @private
   */
  this.cursor_ = 0;

  /**
   * Focused segment index.
   * @type {number}
   * @private
   */
  this.focusedSegmentIndex_ = 0;

  /**
   * The state of the IME.
   * @type {ribbonIme.RibbonIme.State}
   * @private
   */
  this.state_ = ribbonIme.RibbonIme.State.PRECOMPOSITION;
};

/**
 * Determines that IME is enabled or not using a context information.
 * @return {boolean} IME is enabled or not.
 * @private
 */
ribbonIme.RibbonIme.prototype.isImeEnabled_ = function() {
  return this.context_ != null;
};

/**
 * Appends a new empty segment on
 * {@code ribbonIme.RibbonIme.segments}.
 * @private
 */
ribbonIme.RibbonIme.prototype.appendNewSegment_ = function() {
  var startPosition = this.inputText_.length;
  if (this.segments_.length == 0) {
    startPosition = 0;
  }

  var newSegment = new ribbonIme.RibbonIme.Segment();
  newSegment.start = startPosition;

  this.segments_.push(newSegment);
};

/**
 * Gets input text on the segment.
 * @param {number=} opt_segmentIndex Index of the segment you want to get
 *     a text. this.focusedSegmentIndex_ is used as a default value.
 * @return {string} Input text of the segment.
 * @private
 */
ribbonIme.RibbonIme.prototype.getInputTextOnSegment_ = function(opt_segmentIndex) {
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return '';
  }

  var segmentIndex = (opt_segmentIndex == undefined) ? this.focusedSegmentIndex_ : opt_segmentIndex;
  if (segmentIndex < 0 || this.segments_.length <= segmentIndex) {
    return '';
  }

  var start = this.segments_[segmentIndex].start;
  var end = (segmentIndex + 1 == this.segments_.length) ?
  	this.inputText_.length : this.segments_[segmentIndex + 1].start;
  var length = end - start;

  return this.inputText_.substr(start, length);
};

/**
 * Generates and sets candidates of the segment.
 * @param {number=} opt_segmentIndex Index of the segment you want to get a
 *     text. {@code ribbonIme.RibbonIme.focusedSegmentIndex_}
 *     is used as a default value.
 * @private
 */
ribbonIme.RibbonIme.prototype.generateCandidates_ = function(opt_segmentIndex) {
/*
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return;
  }

  var segmentIndex = (opt_segmentIndex == undefined) ? this.focusedSegmentIndex_ : opt_segmentIndex;
  if (segmentIndex < 0 || this.segments_.length <= segmentIndex) {
    return;
  }

  var segment = this.segments_[segmentIndex];
  var text = this.getInputTextOnSegment_(segmentIndex);

  segment.focusedIndex = 0;

  if (text == '') {
    segment.candidates = [];
    return;
  }

  segment.candidates = [
    text.replace(/(\w)/g, function(match) {
      // Converts ASCII alphabet characters to its fullwidth versions.
      var offset = 65248;
      return String.fromCharCode(match.charCodeAt(0) + offset);
    }),
    text,
    text.toUpperCase(),
    text.substr(0, 1).toUpperCase() + text.substr(1).toLowerCase()
  ];

  if (this.useDummyCandidates_) {
    segment.candidates.push('DummyCandidate1');
    segment.candidates.push('DummyCandidate2');
  }
*/
};

/**
 * Gets preedit text.
 * @return {string} Preedit text.
 * @private
 */
ribbonIme.RibbonIme.prototype.getPreeditText_ = function() {
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return '';
  }

  var texts = [];
  for (var i = 0; i < this.segments_.length; ++i) {
    var segment = this.segments_[i];
    texts.push(segment.candidates[segment.focusedIndex]);
  }

  return texts.join('');
};

/**
 * Updates preedit text.
 * @private
 */
ribbonIme.RibbonIme.prototype.updatePreedit_ = function() {
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    chrome.input.ime.clearComposition({
      contextID: this.context_.contextID
    }, function(success) {
      console.log('Composition is cleared. result=' + success);
    });
    return;
  }

/*
  var segmentsData = [];
  for (var i = 0; i < this.segments_.length; ++i) {
    var text = this.segments_[i].candidates[this.segments_[i].focusedIndex];
    var start = i == 0 ? 0 : segmentsData[i - 1].end;
    var end = start + text.length;

    segmentsData.push({
      start: start,
      end: end,
      style: 'underline'
    });
  }
  if (this.state_ == ribbonIme.RibbonIme.State.CONVERSION) {
    segmentsData[this.focusedSegmentIndex_].style = 'doubleUnderline';
  }

  var cursorPos = 0;
  if (this.state_ == ribbonIme.RibbonIme.State.CONVERSION) {
    for (var i = 0; i < this.focusedSegmentIndex_; ++i) {
      var segment = this.segments_[i];
      cursorPos += segment.candidates[segment.focusedIndex].length;
    }
  } else {
    cursorPos = this.cursor_;
  }

  var composition = {
    contextID: this.context_.contextID,
    text: this.getPreeditText_(),
    segments: segmentsData,
    cursor: cursorPos
  };

  if (this.state_ == ribbonIme.RibbonIme.State.CONVERSION) {
    composition.selectionStart = segmentsData[this.focusedSegmentIndex_].start;
    composition.selectionEnd = segmentsData[this.focusedSegmentIndex_].end;
  }

  chrome.input.ime.setComposition(composition, function(success) {
    console.log('Composition is set. result=' + success);
  });
*/

  var composition = {
    contextID: this.context_.contextID,
    text: this.inputText_,
    segments: [{
      start: 0,
      end: this.inputText_.length,
      style: 'underline'
	}],
    cursor: this.cursor_
  };

  chrome.input.ime.setComposition(composition, function(success) {
    console.log('Composition is set. result=' + success);
  });
};

/**
 * Updates candidates.
 * @private
 */
ribbonIme.RibbonIme.prototype.updateCandidates_ = function() {
  var candidateWindowPropertiesCallback = function(success) {
    console.log('Candidate window properties are updated. result=' + success);
  };

  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    chrome.input.ime.setCandidateWindowProperties({
      engineID: this.engineID_,
      properties: {
        visible: false,
        auxiliaryTextVisible: false
      }
    }, candidateWindowPropertiesCallback);
  } else {
    var PAGE_SIZE = ribbonIme.RibbonIme.PAGE_SIZE_;

    var segment = this.segments_[this.focusedSegmentIndex_];
    var labels = [];
    for (var i = 0; i < PAGE_SIZE; ++i) {
      // Roman number.
      labels.push(String.fromCharCode(0x0031 + i));  // '???' + i
    }

    chrome.input.ime.setCandidates({
      contextID: this.context_.contextID,
      candidates: segment.candidates.map(function(value, index) {
        var candidate = {
          candidate: value,
          id: index,
          label: labels[index % PAGE_SIZE]
        };
/*
        if (index == 0) {
          candidate.annotation = '1st candidate';
        }
*/
        return candidate;
      })
    }, function(success) {
      console.log('Candidates are set. result=' + success);
    });
    chrome.input.ime.setCursorPosition({
      contextID: this.context_.contextID,
      candidateID: segment.focusedIndex
    }, function(success) {
      console.log('Cursor position is set. result=' + success);
    });
    chrome.input.ime.setCandidateWindowProperties({
      engineID: this.engineID_,
      properties: {
        visible: true,
        cursorVisible: true,
        vertical: true,
        pageSize: PAGE_SIZE,
        auxiliaryTextVisible: true,
        auxiliaryText: 'Ribbon IME'
      }
    }, candidateWindowPropertiesCallback);
  }
};

/**
 * Updates commit text if {@code commitText_} isn't null.
 * This function clears {@code commitText_} since it is a volatile property.
 * @private
 */
ribbonIme.RibbonIme.prototype.updateCommitText_ = function() {
  if (this.commitText_ === null) {
    return;
  }

  chrome.input.ime.commitText({
    contextID: this.context_.contextID,
    text: this.commitText_
  }, function(success) {
    console.log('Commited. result=' + success);
  });

  this.commitText_ = null;
};

/**
 * Updates output using IME Extension API.
 * @private
 */
ribbonIme.RibbonIme.prototype.update_ = function() {
  this.updatePreedit_();
  this.updateCandidates_();
  this.updateCommitText_();
};

/**
 * Commits a preedit text and clears a context.
 * @private
 */
ribbonIme.RibbonIme.prototype.commit_ = function() {
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return;
  }

  var commitText = this.getPreeditText_();
  this.clear_();
  this.commitText_ = commitText;
};

/**
 * Inserts characters into the cursor position.
 * @param {string} value Text we want to insert into.
 * @private
 */
ribbonIme.RibbonIme.prototype.insertCharacters_ = function(value) {
  if (this.state_ == ribbonIme.RibbonIme.State.CONVERSION) {
    return;
  }

  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    this.appendNewSegment_();
    this.focusedSegmentIndex_ = 0;
  }

  var text = this.inputText_;
  this.inputText_ = text.substr(0, this.cursor_) + value + text.substr(this.cursor_);
  this.state_ = ribbonIme.RibbonIme.State.COMPOSITION;
  this.moveCursor_(this.cursor_ + value.length);

  this.generateCandidates_();
};

/**
 * Removes a character.
 * @param {number} index index of the character you want to remove.
 * @private
 */
ribbonIme.RibbonIme.prototype.removeCharacter_ = function(index) {
  if (this.state_ != ribbonIme.RibbonIme.State.COMPOSITION) {
    return;
  }

  if (index < 0 || this.inputText_.length <= index) {
    return;
  }

  this.inputText_ = this.inputText_.substr(0, index) + this.inputText_.substr(index + 1);

  if (this.inputText_.length == 0) {
    this.clear_();
    return;
  }

  if (index < this.cursor_) {
    this.moveCursor_(this.cursor_ - 1);
  }

  this.generateCandidates_();
};

/**
 * Moves a cursor position.
 * @param {number} index Cursor position.
 * @private
 */
ribbonIme.RibbonIme.prototype.moveCursor_ = function(index) {
  if (this.state_ != ribbonIme.RibbonIme.State.COMPOSITION) {
    return;
  }

  if (index < 0 || this.inputText_.length < index) {
    return;
  }

  this.cursor_ = index;
};

/**
 * Expands a focused segment.
 * @private
 */
ribbonIme.RibbonIme.prototype.expandSegment_ = function() {
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return;
  }

  this.state_ = ribbonIme.RibbonIme.State.CONVERSION;

  var index = this.focusedSegmentIndex_;
  var segments = this.segments_;
  if (index + 1 >= segments.length) {
    return;
  }

  if ((index + 2 == segments.length && segments[index + 1].start + 1 == this.inputText_.length) ||
      (index + 2 < segments.length && segments[index + 1].start + 1 == segments[index + 2].start)) {
    segments.splice(index + 1, 1);
  } else {
    ++segments[index + 1].start;
    this.generateCandidates_(index + 1);
  }

  this.generateCandidates_();
};

/**
 * Shrinks a focused segment.
 * @private
 */
ribbonIme.RibbonIme.prototype.shrinkSegment_ = function() {
  if (this.state_ ==
      ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return;
  }

  this.state_ = ribbonIme.RibbonIme.State.CONVERSION;

  var segments = this.segments_;
  var index = this.focusedSegmentIndex_;

  if (index + 1 == segments.length) {
    if (this.inputText_.length - segments[index].start > 1) {
      this.appendNewSegment_();
      segments[index + 1].start = this.inputText_.length - 1;
      this.generateCandidates_();
      this.generateCandidates_(index + 1);
    }
  } else {
    if (segments[index + 1].start - segments[index].start > 1) {
      --segments[index + 1].start;
      this.generateCandidates_();
      this.generateCandidates_(index + 1);
    }
  }
};

/**
 * Resets a segmentation data of the preedit text.
 * @private
 */
ribbonIme.RibbonIme.prototype.resetSegments_ = function() {
  if (this.state_ != ribbonIme.RibbonIme.State.CONVERSION) {
    return;
  }

  this.segments_ = [];
  this.appendNewSegment_();
  this.focusedSegmentIndex_ = 0;
  this.generateCandidates_();
  this.state_ = ribbonIme.RibbonIme.State.COMPOSITION;
};

/**
 * Selects a candidate.
 * @param {number} candidateID index of the candidate.
 * @private
 */
ribbonIme.RibbonIme.prototype.focusCandidate_ = function(candidateID) {
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return;
  }

  var segment = this.segments_[this.focusedSegmentIndex_];
  if (candidateID < 0 || segment.candidates.length <= candidateID) {
    return;
  }

  segment.focusedIndex = candidateID;
  this.state_ = ribbonIme.RibbonIme.State.CONVERSION;
};

/**
 * Focuses a segment.
 * @param {number} segmentID index of the segment.
 * @private
 */
ribbonIme.RibbonIme.prototype.focusSegment_ = function(segmentID) {
  if (this.state_ != ribbonIme.RibbonIme.State.CONVERSION) {
    return;
  }

  if (segmentID < 0 || this.segments_.length <= segmentID) {
    return;
  }

  this.focusedSegmentIndex_ = segmentID;
};

/**
 * Handles a alphabet key.
 * @param {!Object} keyData key event data.
 * @return {boolean} true if key event is consumed.
 * @private
 */
ribbonIme.RibbonIme.prototype.handleKey_ = function(keyData) {
/*
  var keyValue = keyData.key;

  if (keyData.altKey || keyData.ctrlKey || keyData.shiftKey) {
    return false;
  }

  if (!keyValue.match(/^[a-z]$/i)) {
    return false;
  }

  if (this.state_ == ribbonIme.RibbonIme.State.CONVERSION) {
    this.commit_();
  }

  this.insertCharacters_(keyValue);

  this.update_();
*/
  return true;
};

/**
 * Handles a non-alphabet key.
 * @param {!Object} keyData key event data.
 * @return {boolean} true if key event is consumed.
 * @private
 */
ribbonIme.RibbonIme.prototype.handleSpecialKey_ = function(keyData) {
/*
  if (this.state_ == ribbonIme.RibbonIme.State.PRECOMPOSITION) {
    return false;
  }

  var segment = this.segments_[this.focusedSegmentIndex_];

  if (!keyData.altKey && !keyData.ctrlKey && !keyData.shiftKey) {
    switch (keyData.key) {
    case 'Backspace':
      if (this.state_ == ribbonIme.RibbonIme.State.CONVERSION) {
        this.resetSegments_();
      } else if (this.cursor_ != 0) {
        this.removeCharacter_(this.cursor_ - 1);
      }
      break;
    case 'Delete':
      if (this.state_ ==
          ribbonIme.RibbonIme.State.CONVERSION) {
        this.resetSegments_();
      } else if (this.cursor_ != this.inputText_.length) {
        this.removeCharacter_(this.cursor_);
      }
      break;
    case 'Up':
      var previous_index = segment.focusedIndex - 1;
      if (previous_index == -1) {
        previous_index = segment.candidates.length - 1;
      }
      this.focusCandidate_(previous_index);
      break;
    case 'Down':
    case ' ':
      var next_index = segment.focusedIndex + 1;
      if (next_index == segment.candidates.length) {
        next_index = 0;
      }
      this.focusCandidate_(next_index);
      break;
    case 'Left':
      if (this.state_ ==
          ribbonIme.RibbonIme.State.CONVERSION) {
        if (this.focusedSegmentIndex_ != 0) {
          this.focusSegment_(this.focusedSegmentIndex_ - 1);
        }
      } else {
        this.moveCursor_(this.cursor_ - 1);
      }
      break;
    case 'Right':
      if (this.state_ ==
          ribbonIme.RibbonIme.State.CONVERSION) {
        if (this.focusedSegmentIndex_ + 1 != this.segments_.length) {
          this.focusSegment_(this.focusedSegmentIndex_ + 1);
        }
      } else {
        this.moveCursor_(this.cursor_ + 1);
      }
      break;
    case 'Enter':
      this.commit_();
      break;
    default:
      return true;
    }
  } else if (!keyData.altKey && !keyData.ctrlKey && keyData.shiftKey) {
    switch (keyData.key) {
    case 'Left':
      if (this.state_ !=
          ribbonIme.RibbonIme.State.PRECOMPOSITION) {
        this.shrinkSegment_();
      }
      break;
    case 'Right':
      if (this.state_ !=
          ribbonIme.RibbonIme.State.PRECOMPOSITION) {
        this.expandSegment_();
      }
      break;
    default:
      return true;
    }
  } else {
    return true;
  }

  this.update_();
*/
  return true;
};

/**
 * Sets up a menu on a uber tray.
 * @private
 */
ribbonIme.RibbonIme.prototype.setUpMenu_ = function() {
  chrome.input.ime.setMenuItems({
    engineID: this.engineID_,
    items: this.menuItems_
  }, function() {
    console.log('Menu items are set.');
  });
};

/**
 * Callback method. It is called when IME is activated.
 * @param {string} engineID engine ID.
 */
ribbonIme.RibbonIme.prototype.onActivate = function(engineID) {
  this.engineID_ = engineID;
  this.clear_();
  this.setUpMenu_();
};

/**
 * Callback method. It is called when IME is deactivated.
 * @param {string} engineID engine ID.
 */
ribbonIme.RibbonIme.prototype.onDeactivated = function(engineID) {
  this.clear_();
  this.engineID_ = '';
};

/**
 * Callback method. It is called when a context acquires a focus.
 * @param {!Object} context context information.
 */
ribbonIme.RibbonIme.prototype.onFocus = function(context) {
  this.context_ = context;
  this.clear_();
};

/**
 * Callback method. It is called when a context lost a focus.
 * @param {number} contextID ID of the context.
 */
ribbonIme.RibbonIme.prototype.onBlur = function(contextID) {
  this.clear_();
  this.context_ = null;
};

/**
 * Callback method. It is called when properties of the context is changed.
 * @param {!Object} context context information.
 */
ribbonIme.RibbonIme.prototype.onInputContextUpdate = function(context) {
  this.context_ = context;
  if (!this.isImeEnabled_()) {
    this.clear_();
  }

  this.update_();
};

/**
 * Callback method. It is called when IME catches a new key event.
 * @param {string} engineID ID of the engine.
 * @param {!Object} keyData key event data.
 * @return {boolean} true if the key event is consumed.
 */
ribbonIme.RibbonIme.prototype.onKeyEvent = function(engineID, keyData) {
  if (keyData.type != 'keydown' || !this.isImeEnabled_()) {
    return false;
  }

  var key = this.stringifyKeyAndModifiers_(keyData);
  if (ribbonIme.RibbonIme.IGNORABLE_KEY_SET_[key]) {
    return false;
  }

	var keyValue = keyData.key;
	var keyEvent = {
		label: keyValue,
		keyId: "qw-" + keyValue,
		keyCode: 111,
		shiftState: false,
		capsState: false,
		command: "keydown"
	};

    switch (keyData.key) {
    case 'Backspace':
      keyEvent.keyCode = 17;
      break;
    case 'Delete':
      keyEvent.keyCode = 81;
      break;
	case 'Up':
      keyEvent.keyCode = 84;
      break;
	case 'Down':
      keyEvent.keyCode = 86;
      break;
	case 'Left':
      keyEvent.keyCode = 85;
      break;
	case 'Right':
      keyEvent.keyCode = 87;
      break;
    case ' ':
      keyEvent.keyCode = 70;
      break;
    case 'Enter':
      keyEvent.keyCode = 44;
      break;
    }

	var keyEventText = JSON.stringify(keyEvent);
console.log(keyEventText);
	Module['imeJsonCommand'](keyEventText);

/*
	var canidateStateUtf8 = Module['imeCandidateState']();
	var candidateState = JSON.parse(decodeURIComponent(escape(canidateStateUtf8)));

	if (this.segments_.length == 0) {
		var newSegment = new ribbonIme.RibbonIme.Segment();
		this.segments_.push(newSegment);
	}

	this.segments_[0].start = 0;
	this.segments_[0].candidates = candidateState.candidates;
	this.segments_[0].focusedIndex = 0;

	var compositionStateUtf8 = Module['imeCompositionState']();
console.log(compositionStateUtf8);
	var compositionState = JSON.parse(decodeURIComponent(escape(compositionStateUtf8)))

	this.inputText_ = null;
	this.cursor_ = 0;
	if (compositionState.composition[0].display) {
		this.inputText_ = compositionState.composition[0].display;
		this.cursor_ = compositionState.caret;
	}

	this.commitText_ = null;
	if (compositionState.commit) {
		this.commitText_ = compositionState.commit;
	}

	this.state_ = ribbonIme.RibbonIme.State.PRECOMPOSITION;
	if (this.inputText_) {
		this.state_ = ribbonIme.RibbonIme.State.COMPOSITION;
	}

	this.update_();

    return compositionState.through < 0;

  //return this.handle(keyData) || this.handleSpecialKey_(keyData);
*/
	return this.postProcess(engineID);
};

ribbonIme.RibbonIme.prototype.postProcess = function(engineID) {
	var canidateStateUtf8 = Module['imeCandidateState']();
	var candidateState = JSON.parse(decodeURIComponent(escape(canidateStateUtf8)));

	if (this.segments_.length == 0) {
		var newSegment = new ribbonIme.RibbonIme.Segment();
		this.segments_.push(newSegment);
	}

	this.segments_[0].start = 0;
	this.segments_[0].candidates = candidateState.candidates;
	this.segments_[0].focusedIndex = 0;

	var compositionStateUtf8 = Module['imeCompositionState']();
console.log(compositionStateUtf8);
	var compositionState = JSON.parse(decodeURIComponent(escape(compositionStateUtf8)))

	this.inputText_ = null;
	this.cursor_ = 0;
	if (compositionState.composition[0].display) {
		this.inputText_ = compositionState.composition[0].display;
		this.cursor_ = compositionState.caret;
	}

	this.commitText_ = null;
	if (compositionState.commit) {
		this.commitText_ = compositionState.commit;
	}

	this.state_ = ribbonIme.RibbonIme.State.PRECOMPOSITION;
	if (this.inputText_) {
		this.state_ = ribbonIme.RibbonIme.State.COMPOSITION;
	}

	this.update_();

    return compositionState.through < 0;
}

/**
 * Callback method. It 
        called when candidates on candidate window is clicked.
 * @param {string} engineID ID of the engine.
 
        param {number} candidateID Index of the candidate.
 * @param {string} button Which mouse button was clicked.
 */
ribbonIme.RibbonIme.prototype.onCandidateClicked = function(engineID, candidateID, button) {
  if (button == 'left') {
/*
    this.focusCandidate_(candidateID);
*/

	var candEvent = {
		command: "CandChosen",
		candIdx: candidateID
	};

	var candEventText = JSON.stringify(candEvent);
	Module['imeJsonCommand'](candEventText);

	this.postProcess(engineID);
  }
};

/**
 * Callback method. It is called when menu item on uber tray is activated.
 * @param {string} engineID ID of the engine.
 * @param {string} name name of the menu item.
 */ 
ribbonIme.RibbonIme.prototype.onMenuItemActivated = function(engineID, name) {
  var callback = this.menuItemCallbackTable_[name];
  if (typeof(callback) != 'function') {
    return;
  }

  callback();

  chrome.input.ime.updateMenuItems({
    engineID: engineID,
    items: this.menuItems_
  }, function() {
    console.log('Menu items are updated.');
  });
};
