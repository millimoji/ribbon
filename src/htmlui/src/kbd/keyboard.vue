<template>
  <div class="keyboard" :style="rectStyle" ref="keyboard"
      @touchstart.stop.prevent.capture="onTouchStart($event)"
      @mousedown.stop.prevent.capture="onMouseStart($event)">
    <kbd-row v-for="(item, key) in keyRows"
        :key-row="item"
        :key="key"/>
  </div>
</template>

<script>
import kbdRow from './kbdRow'
import { kbdData } from './kbddata'
import ActionMonitor from './actionmon'

export default {
  name: 'keyboard',
  props: [ 'keyboardWidth', 'keyboardHeight' ],
  data: function () {
    return {
      keyRowsImpl: [],
      actionMonitor: null
    }
  },
  created: function () {
    this.actionMonitor = new ActionMonitor(this)
  },
  methods: {
    onMouseStart: function (e) {
      var kbdkey = this.findCoresspondingKbdkey(e.target)
      this.actionMonitor.onMouseStart(e, kbdkey)
    },
    onTouchStart: function (e) {
      var kbdkey = this.findCoresspondingKbdkey(e.target)
      this.actionMonitor.onTouchStart(e, kbdkey)
    },

    // Helper methods
    findCoresspondingKbdkey: function (node) {
      while (node) {
        if (('__vue__' in node) && ('isKbdKey' in node.__vue__)) {
          return node.__vue__
        }
        node = node.parentNode
      }
    }
  },
  computed: {
    rectStyle: function () {
      return {
        width: this.keyboardWidth + 'px',
        height: this.keyboardHeight + 'px'
      }
    },
    keyRows: function () {
      var layoutData = kbdData.layouts[this.$store.state.layoutName]
      var frameData = kbdData.frames[layoutData.frames[0]]
      var rowCount = frameData.data.length

      this.keyRowsImpl.splice(0, this.keyRowsImpl.length)
      for (var rowIdx = 0; rowIdx < rowCount; ++rowIdx) {
        var curTop = this.keyboardHeight * rowIdx / rowCount
        var curNext = this.keyboardHeight * (rowIdx + 1) / rowCount
        var curHeight = curNext - curTop

        this.keyRowsImpl.push({
          layout: layoutData.data[rowIdx],
          frame: frameData.data[rowIdx],
          top: curTop,
          height: curHeight,
          width: this.keyboardWidth
        })
      }
      return this.keyRowsImpl
    }
  },
  components: { kbdRow }
}
</script>

<style>
.keyboard {
  background-color: #444444;
  position: relative;
}
.kbdkey {
  color: #e5e5e5;
}
.kbdkey .label {
  /* border-color: #626262; */
  background-color: #626262;
}
.kbdkey.pressed .label {
  color: #444444;
  background-color: #e5e5e5;
}
</style>
