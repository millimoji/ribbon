<template>
  <div class="candview" :class="{grid: isGrid}" :style="localStyle">
    <div class="candcanvas">
      <div class="canditem" v-for="(item, index) in candList"
        :style="itemStyle"
        @touchstart.stop.prevent.capture="onCandItemPressed(index, $event)"
        @mousedown.stop.prevent.capture="onCandItemPressed(index, $event)"
        :key="index">{{item}}</div>
    </div>
    <div class="expandButton" :style="expandButtonStyle"
      @touchstart.stop.prevent.capture="onExpandClick"
      @mousedown.stop.prevent.capture="onExpandClick"
      >{{expandMark}}</div>
  </div>
</template>

<script>
export default {
  name: 'candView',
  props: [ 'candType', 'candLineHeight', 'candViewWidth', 'candViewHeight', 'panelHeight' ],
  computed: {
    candList () {
      return this.$store.state.candidateList
    },
    localStyle () {
      return {
        overflowY: (this.isGrid || this.$store.state.expandedCandView ? 'scroll' : 'hidden')
      }
    },
    itemStyle () {
      let fontSize = this.candLineHeight * 2 / 5
      return {
        fontSize: fontSize + 'px',
        hegiht: this.candLineHeight + 'px',
        lineHeight: this.candLineHeight + 'px'
      }
    },
    expandMark () {
      return this.$store.state.expandedCandView ? '\u25b3' : '\u25bd'
    },
    expandButtonStyle () {
      return {
        left: (this.candViewWidth - 48) + 'px'
      }
    },
    isGrid () {
      return this.candType === 'gird'
    }
  },
  created: function () {
    this.candiateItemHandler = new CandidateItemHandler()
  },
  methods: {
    onExpandClick () {
      this.$store.commit('setExpandedCandView', !this.$store.state.expandedCandView)
    },
    onCandItemPressed (candIdx, event) {
      let _this = this
      this.candiateItemHandler.Start(
        candIdx,
        event,
        function () { _this.submitCandidate(candIdx) },
        function () { _this.searchWordWeb(candIdx) },
        function () { _this.cancelSeletion() }
      )
    },
    submitCandidate (candIdx) {
      var dataToSend = {
        command: 'CandChosen',
        candIdx: candIdx
      }
      var jsonToSend = JSON.stringify(dataToSend)
      window.MediatorCall(jsonToSend)
    },
    searchWordWeb (candIdx) {
      this.$store.commit('setShowSearchUi', this.$store.state.candidateList[candIdx])
    },
    cancelSelection () {
    }
  }
}

class CandidateItemHandler {
  constructor () {
    this.timeOutCookie = -1
    this.candItemIndex = -1

    let _this = this
    this._onMouseMove = function (e) { return _this.onMouseMove(e) }
    this._onMouseEnd = function (e) { return _this.onMouseEnd(e) }
    this._onTouchMove = function (e) { return _this.onTouchMove(e) }
    this._onTouchEnd = function (e) { return _this.onTouchEnd(e) }
    this._onTouchCancel = function (e) { return _this.onTouchCancel(e) }
  }
  Start (candIdx, e, submitMethod, searchMethod, cancelMethod) {
    // members
    this.setupEventListener()
    let _this = this
    this.timeOutCookie = setTimeout(() => { _this.onTimeOut() }, 1000)
    this.candItemIndex = candIdx
    this.submitMethod = submitMethod
    this.searchMethod = searchMethod
    this.cancelMethod = cancelMethod
  }
  // touch action handlers
  onMouseMove (e) {
    return this.onTouchMove(e)
  }
  onMouseEnd (e) {
    return this.onTouchEnd(e)
  }
  onTouchMove (e) {
    // e.preventDefault()
    e.stopPropagation()
  }
  onTouchEnd (e) {
    let beforeTimeOut = (this.timeOutCookie >= 0)
    this.onTouchCancel(e)
    if (beforeTimeOut) {
      this.submitMethod()
    }
  }
  onTouchCancel (e) {
    if (e) {
      e.preventDefault()
      e.stopPropagation()
    }
    if (this.timeOutCookie >= 0) {
      clearTimeout(this.timeOutCookie)
    }
    this.timeOutCookie = -1
    this.cleanUpEventListener()
  }
  onTimeOut () {
    let beforeTimeOut = (this.timeOutCookie >= 0)
    this.onTouchCancel(null)
    if (beforeTimeOut) {
      this.searchMethod()
    }
  }
  setupEventListener () {
    document.addEventListener('mousemove', this._onMouseMove, false)
    document.addEventListener('mouseup', this._onMouseEnd, false)
    document.addEventListener('touchmove', this._onTouchMove, false)
    document.addEventListener('touchend', this._onTouchEnd, false)
    document.addEventListener('touchcancel', this._onTouchCancel, false)
  }
  cleanUpEventListener () {
    document.removeEventListener('mousemove', this._onMouseMove)
    document.removeEventListener('mouseup', this._onMouseEnd)
    document.removeEventListener('touchmove', this._onTouchMove)
    document.removeEventListener('touchend', this._onTouchEnd)
    document.removeEventListener('touchcancel', this._onTouchCancel)
  }
}
</script>

<style>
.candview {
  width: 100%;
  height: 100%;
  text-align: left;
}
.candcanvas {
  width: 100%;
}
.canditem {
  text-align: center;
  font-size: 20px;
  display: inline-block;
  padding: 0 8px;
}
.candview.grid .canditem {
  width: 10%;
  padding: 0;
}
.expandButton {
  font-size: 32px;
  overflow: hidden;
  text-align: center;
  position: absolute;
  width: 48px;
  height: 72px;
  line-height: 72px;
  left: 0px;
  top: 0px;
}
.candview.grid .expandButton {
  display: none
}
</style>
