<template>
  <div class="kbdkey"
      v-if="!isGap"
      :style="localStyle"
      :class="{pressed: isPressed, narrow: isNarrowPitch}">
    <div class="label"
      >{{keyLabel}}</div>
  </div>
</template>

<script>
import { SIPKEY, kbdData } from './kbddata'

export default {
  name: 'kbdKey',
  props: [ 'keyInfo' ],
  computed: {
    keyDef () { return kbdData.keydefs[this.keyInfo.id] },
    isGap () { return ('kc' in this.keyDef) && (this.keyDef.kc === SIPKEY.GAP) },
    isSpace () { return ('kc' in this.keyDef) && (this.keyDef.kc === SIPKEY.SPACE) },
    isShift () { return ('kc' in this.keyDef) && (this.keyDef.kc === SIPKEY.LSHIFT) },
    isNarrowPitch () { return (this.keyInfo.id === 'np-.') },

    normalLabel () { return this.keyDef.l },
    shiftLabel () { return ('sl' in this.keyDef) ? this.keyDef.sl : this.keyDef.l },
    capsLabel () { return ('cl' in this.keyDef) ? this.keyDef.cl : this.shiftLabel },

    keyLabel () { return this.$store.state.shiftState ? this.shiftLabel : this.normalLabel },
    isPressed () { return this.$store.state.focusKey === this },
    localStyle () {
      let fontSize = this.keyInfo.style.height * 0.4 - 6
      return {
        top: this.keyInfo.style.top + 'px',
        height: this.keyInfo.style.height + 'px',
        left: this.keyInfo.style.left + 'px',
        width: this.keyInfo.style.width + 'px',
        lineHeight: (this.keyInfo.style.height - 6) + 'px',
        fontSize: fontSize + 'px'
      }
    }
  },
  methods: {
    isKbdKey: function () {
      return true
    },
    getKeyData: function () {
      let keyCode
      if ('kc' in this.keyDef) {
        keyCode = this.keyDef.kc
      } else {
        keyCode = kbdData.layouts[this.$store.state.layoutName].defaultKC
      }
      return {
        label: this.keyLabel,
        keyId: this.keyInfo.id,
        keyCode: keyCode,
        shiftState: this.$store.state.shiftState,
        capsState: this.$store.state.capsState
      }
    },
    flickKeyCode: function () {
      return this.keyDef.flickKC
    },
    defaultKC: function () {
      return kbdData.layouts[this.$store.state.layoutName].defaultKC
    }
  }
}
</script>

<style>
.kbdkey {
  position: absolute;
  box-sizing: border-box;
  padding: 0.4%;
}
.kbdkey .label {
  position: static;
  box-sizing: border-box;
  height: 100%;
  width: 100%;
  text-align: center;
  user-select: none;
  /*
  border-style: solid;
  border-width: 1px;
  border-radius: 5px;
  */
  border: none;
  cursor: pointer;
}
.kbdkey.narrow .label {
  letter-spacing: -0.3em;
}
</style>
