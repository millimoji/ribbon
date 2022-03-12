<template>
  <span class="searcharea">
    <iframe class="searchui" :src="srcUrl"></iframe>
    <div class="search-close"
        @touchstart.stop.prevent.capture="onCloseSearchUI($event)"
        @mousedown.stop.prevent.capture="onCloseSearchUI($event)"
    >&#x274c;</div>
  </span>
</template>

<script>
export default {
  name: 'searchUi',
  props: [ 'keyword' ],
  computed: {
    srcUrl () {
      return encodeURI('https://www.bing.com/search?q=' + this.$store.state.showSearchUi)
    }
  },
  methods: {
    onCloseSearchUI (e) {
      this.$store.commit('setShowSearchUi', null)
    }
  }
}
</script>

<style>
.searchui {
  position: absolute;
  width: 100%;
  height: 100%;
  left: 0px;
  top: 0px;
  box-sizing: border-box;
}
.search-close {
  position: absolute;
  top: 0px;
  right: 0px;
  width: 32px;
  height: 32px;
  font-size: 32px;
}
</style>
