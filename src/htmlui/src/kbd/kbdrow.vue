<template>
  <span>
    <kbd-key v-for="(item, key) in keyItems"
        :key-info="item"
        :key="key"/>
  </span>
</template>

<script>
import kbdKey from './kbdKey'
import { kbdData } from './kbddata'

export default {
  name: 'kbdRow',
  props: [ 'keyRow' ],
  data: function () {
    return {
      keyItemsImpl: [],

      // temporal vars
      keyIdArray: [],
      totalKeyWidth: 0,
      jerseyWidth: 0
    }
  },
  methods: {
    concatKeys: function () {
      var keyIdArray = []
      if ('l' in this.keyRow.frame) {
        keyIdArray = this.keyRow.frame.l
      }
      keyIdArray = keyIdArray.concat(this.keyRow.layout)
      if ('r' in this.keyRow.frame) {
        keyIdArray = keyIdArray.concat(this.keyRow.frame.r)
      }
      this.keyIdArray = keyIdArray
    },
    calcTotalKeyWidth: function () {
      var finalWidthForJersey = 0
      var totalWidth = 0
      for (var idx in this.keyIdArray) {
        var keyDef = kbdData.keydefs[this.keyIdArray[idx]]
        if ('w' in keyDef) {
          if (keyDef.w < 0) {
            finalWidthForJersey = -keyDef.w
          } else {
            totalWidth += keyDef.w
          }
        } else {
          totalWidth += 1
        }
      }
      if (finalWidthForJersey > 0) {
        this.totalKeyWidth = finalWidthForJersey
        this.jerseyWidth = finalWidthForJersey - totalWidth
      } else {
        this.totalKeyWidth = totalWidth
      }
    },
    updateEachKeyWidth: function () {
      this.keyItemsImpl.splice(0, this.keyItemsImpl.length)
      var curLeft = 0
      for (var idx in this.keyIdArray) {
        var keyDef = kbdData.keydefs[this.keyIdArray[idx]]
        var keyWidth = 1
        if ('w' in keyDef) {
          if (keyDef.w < 0) {
            keyWidth = this.jerseyWidth
          } else {
            keyWidth = keyDef.w
          }
        }
        var curRight = curLeft + keyWidth
        var actualLeft = this.keyRow.width * curLeft / this.totalKeyWidth
        var actualRight = this.keyRow.width * curRight / this.totalKeyWidth
        var actualWidth = actualRight - actualLeft

        this.keyItemsImpl.push({
          id: this.keyIdArray[idx],
          style: {
            top: this.keyRow.top,
            height: this.keyRow.height,
            left: actualLeft,
            width: actualWidth
          }
        })
        curLeft = curRight
      }
      return this.keyItemsImpl
    }

  },
  computed: {
    keyItems: function () {
      this.concatKeys()
      this.calcTotalKeyWidth()
      this.updateEachKeyWidth()
      return this.keyItemsImpl
    }
  },
  components: { kbdKey }
}
</script>

<style>
</style>
