<template>
  <div id="app">
    <InputPanel/>
  </div>
</template>

<script>
import InputPanel from '@/kbd/inputpanel'
// import { SIPKEY } from '@/kbd/kbddata'

export default {
  name: 'app',
  components: { InputPanel },
  created: function () {
    let _this = this
    // Android
    window.MediatorTrigger = function () {
      if ('mediator' in window) {
        let jsonData = window.mediator.GetCandidateState()
        let candidateList = JSON.parse(jsonData)
        _this.$store.commit('updateCandidateList', candidateList.candidates)
        if (candidateList.candidates.length === 0) {
          _this.$store.commit('setExpandedCandView', false)
        }
      }
    }
    // iOS, Windows XAML
    window.MediatorUpdateCandidate = function (jsonData) {
      let candidateList = JSON.parse(jsonData)
      _this.$store.commit('updateCandidateList', candidateList.candidates)
      if (candidateList.candidates.length === 0) {
        _this.$store.commit('setExpandedCandView', false)
      }
    }

    let ua = window.navigator.userAgent.toLowerCase()

    // iOS
    if (ua.indexOf('iphone') !== -1) {
      window.MediatorCall = function (jsonArg) {
        document.location = 'mediator://' + encodeURIComponent(jsonArg)
      }
    // Android
    } else if ('mediator' in window) {
      window.MediatorCall = function (jsonArg) {
        window.mediator.NativeRequest(jsonArg)
      }
    // Widnows XAML
    } else if ('external' in window && 'notify' in window.external) {
      window.MediatorCall = function (jsonArg) {
        window.external.notify(jsonArg)
      }
    // Debugging code
    } else {
      window.MediatorCall = function (jsonArg) {
        console.log(jsonArg)
/*
        let eventData = JSON.parse(jsonArg)
        if ('command' in eventData && 'keyCode' in eventData && 'keyId' in eventData) {
          if (eventData.command === 'keydown' && eventData.keyCode === SIPKEY.SWITCH) {
            setTimeout(function () {
              _this.$store.commit('updateCandidateList', [
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                '`', '~', '!', '@', '#', '$', '%', '^', '&', '*',
                '(', ')', '-', '=', '_', '+', '[', ']', '\\', '{', '}',
                '|', ';', '\'', ':', '"', ', ', '.', '/', '<', '>', '?'
              ])
            }, 50)
          }
        }
*/
      }
    }
  }
}
</script>

<style>
body {
  margin: 0;
  overflow: hidden;
  user-select: none;
}
#app {
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
}
</style>
