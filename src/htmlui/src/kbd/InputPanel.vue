<template>
  <span>
    <div class="candbox" :style="candViewStyle">
      <cand-view :cand-type="candType" :cand-line-height="candLineHeight" :cand-view-width="panelWidth" :cand-view-height="candViewHeight" :panel-height="panelHeight"/>
    </div>
    <div class="kbdbox" :style="keyboardStyle">
      <keyboard :keyboard-width="panelWidth" :keyboard-height="keyboardHeight"/>
      <slide-key />
    </div>
    <searchUi v-if="showSearchUi" />
  </span>
</template>

<script>
import CandView from './candview'
import Keyboard from './keyboard'
import slideKey from './slidekey'
import searchUi from './searchui'
import { kbdData } from './kbddata'

export default {
  name: 'InputPanel',
  components: { Keyboard, CandView, searchUi, slideKey },
  data: function () {
    return {
      panelWidth: 100,
      panelHeight: 100,
      totalRowCount: 6
    }
  },
  computed: {
    countOfKeyRow () {
      return kbdData.layouts[this.$store.state.layoutName].data.length
    },
    candLineHeight () {
      return this.panelHeight / this.totalRowCount
    },
    candViewHeight () {
      return this.$store.state.expandedCandView ? this.panelHeight
        : this.panelHeight * (this.totalRowCount - this.countOfKeyRow) / this.totalRowCount
    },
    keyboardHeight () {
      return this.panelHeight * this.countOfKeyRow / this.totalRowCount
    },
    candType () {
      let layout = kbdData.layouts[this.$store.state.layoutName]
      if ('candType' in layout) {
        return layout.candType
      }
      return 'normal'
    },
    candViewStyle () {
      return {
        width: this.panelWidth + 'px',
        height: this.candViewHeight + 'px'
      }
    },
    keyboardStyle () {
      return {
        top: this.candViewHeight + 'px',
        width: this.panelWidth + 'px',
        height: this.keyboardHeight + 'px'
      }
    },
    showSearchUi () {
      return !!this.$store.state.showSearchUi
    }
  },
  created: function () {
    this.onWindowResize(null)
    window.addEventListener('resize', this.onWindowResize)
  },
  beforeDestroy: function () {
    window.removeEventListener('resize', this.onWindowResize)
  },
  methods: {
    // global events
    onWindowResize: function (e) {
      this.panelWidth = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth
      this.panelHeight = window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight
    }
  }
}
</script>

<style>
.candViewBox {
  transition: all 200ms 0s ease;
  position: absolute;
  top: 0px;
  left: 0px;
}
.kbdbox {
  position: absolute;
  left: 0px;
}
</style>
