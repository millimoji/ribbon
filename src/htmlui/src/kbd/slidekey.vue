<template>
  <div class="slideboard"
      v-if="showSlide">
    <div class="slidekey"
      v-for="(item, key) in childKeyList"
      :key="key"
      :style="item.style"
      :class="{ focused: key == focusIndex }"
      v-bind:data-index="key"
    ><div class="slidekey-label">{{item.l}}</div></div>
  </div>
</template>
<script>
import { kbdData } from './kbddata'

export default {
  name: 'slideKey',
  data: function () {
    return {
      childKeyListImpl: []
    }
  },
  computed: {
    showSlide () { return this.$store.state.slideKey != null },
    focusIndex () {
      if (this.$store.state.slideFocusNode) {
        let index = this.$store.state.slideFocusNode.getAttribute('data-index')
        return index
      }
      return -1
    },
    childKeyList () {
      if (!this.$store.state.slideKey) {
        this.childKeyListImpl.splice(0, this.childKeyListImpl.length)
        return this.childKeyListImpl
      }
      let keyData = this.$store.state.slideKey.getKeyData()
      let parentLabel = kbdData.keydefs[keyData.keyId].l
      let charList = kbdData.keydefs[keyData.keyId].slide
      let leftCount = charList.findIndex((value, index, srcArray) => { return value === parentLabel })

      let parentKeyDiv = this.$store.state.slideKey.$el
      let leftCoord = parentKeyDiv.offsetLeft - (parentKeyDiv.offsetWidth * leftCount)
      let topCoord = parentKeyDiv.offsetTop - (parentKeyDiv.offsetHeight * 1.5)
      let lineHeight = parentKeyDiv.offsetHeight - 6
      let fontSize = parentKeyDiv.offsetHeight * 0.4 - 6

      this.childKeyListImpl.splice(0, this.childKeyListImpl.length)
      for (let idx = 0; idx < charList.length; ++idx) {
        this.childKeyListImpl.push(
          { l: charList[idx],
            style: {
              top: topCoord + 'px',
              left: (leftCoord + (parentKeyDiv.offsetWidth * idx)) + 'px',
              width: parentKeyDiv.offsetWidth + 'px',
              height: parentKeyDiv.offsetHeight + 'px',
              lineHeight: lineHeight + 'px',
              fontSize: fontSize + 'px'
            }
          })
      }
      return this.childKeyListImpl
    }
  } /*,
  methods: {
    isKbdKey: function () {
      return true
    },
    getKeyData: function () {
      return {
        label: this.keyLabel,
        keyId: this.keyInfo.id,
        keyCode: ('kc' in this.keyDef) ? this.keyDef.kc : SIPKEY.ANYTEXT,
        shiftState: this.$store.state.shiftState,
        capsState: this.$store.state.capsState
      }
    }
  } */
}
</script>

<style>
.slideboard {
  position: absolute;
  width: 100%;
  height: 100%;
  left: 0px;
  top: 0px;
  box-sizing: border-box;
  background: rgba(0, 0, 0, 0.5);
}
.slidekey {
  position: absolute;
  box-sizing: border-box;
  padding: 4px;
}
.slidekey-label {  
  width: 100%;
  height: 100%;
  color: white;
  background-color: grey;
  border-style: solid;
  border-width: 1px;
  border-radius: 5px;
  cursor: pointer;
  text-align: center;
}
.slidekey.focused .slidekey-label {
  background-color: red;
}
</style>
