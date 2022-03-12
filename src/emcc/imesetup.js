'use strict';

document.addEventListener('readystatechange', function() {
  if (document.readyState == 'complete') {
    if (!chrome || !chrome.input || !chrome.input.ime) {
      console.error('chrome.input.ime APIs are not available');
      return;
    }
    new ribbonIme.RibbonIme();
  }
}, true);
