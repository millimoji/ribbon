export const state = {
  layoutName: 'numpadkana',
  shiftState: false,
  capsState: false,
  ctrlState: false,
  altState: false,
  focusKey: null,
  slideKey: null,
  slideFocusNode: null,
  expandedCandView: false,
  candidateList: [],
  showSearchUi: null
}

export const mutations = {
  setLayoutName (state, layoutName) {
    state.layoutName = layoutName
  },
  setShift (state, shiftState) {
    state.shiftState = shiftState
  },
  setCaps (state, capsState) {
    state.capsState = capsState
  },
  setCtrl (state, ctrltState) {
    state.ctrltState = ctrltState
  },
  setAlt (state, altState) {
    state.altState = altState
  },
  setFocusKey (state, key) {
    state.focusKey = key
  },
  setSlideKey (state, key) {
    state.slideKey = key
  },
  setSlideFocusNode (state, node) {
    state.slideFocusNode = node
  },
  setExpandedCandView (state, expanded) {
    state.expandedCandView = expanded
  },
  updateCandidateList (state, newCandidateList) {
    state.candidateList.splice(0)
    Array.prototype.push.apply(state.candidateList, newCandidateList)
  },
  setShowSearchUi (state, showSarchUi) {
    state.showSearchUi = showSarchUi
  }
}
