import { SIPKEY, kbdData } from './kbddata'

class TouchActionBase {
  constructor (keyNode, errorPixel, resultSink) {
    this.resultSink = resultSink
    this.keyNode = keyNode
    this.errorPixel = errorPixel
    this.dataIsSent = false
  }
  onTouchStart (pointArray) {}
  onTouchMove (pointArray) {}
  onTouchEnd (pointArray) {
    this.sendNormalKey(this.keyNode)
  }
  onRepatTimer (pointArray) {}

  // Utility
  sendNormalKey (keyNode) {
    var dataToSend = keyNode.getKeyData()
    dataToSend.command = 'keydown'
    this.sendJsonKey(dataToSend)
  }
  sendJsonKey (keyData) {
    var jsonToSend = JSON.stringify(keyData)
    window.MediatorCall(jsonToSend)
  }
}

class SwitchKeyHandler extends TouchActionBase {
  constructor (keyNode, errorPixel, resultSink) {
    super(keyNode, errorPixel, resultSink)
    this.goPrev = false
  }
  onRepatTimer (pointArray) {
    this.goPrev = true
  }
  onTouchEnd (pointArray) {
    let keyData = this.keyNode.getKeyData()
    let keyDef = kbdData.keydefs[keyData.keyId]

    if (this.goPrev) {
      if ('prev' in keyDef) {
        this.resultSink.$store.commit('setLayoutName', keyDef.prev)
        this.sendJsonKey({command: 'keydown', keyCode: SIPKEY.SWITCH, keyId: keyDef.prev})
        return
      }
    } else {
      if ('next' in keyDef) {
        this.resultSink.$store.commit('setLayoutName', keyDef.next)
        this.sendJsonKey({command: 'keydown', keyCode: SIPKEY.SWITCH, keyId: keyDef.next})
        return
      }
    }
    this.sendNormalKey(this.keyNode)
  }
}

class ShiftKeyHandler extends TouchActionBase {
  onTouchEnd (pointArray) {
    this.resultSink.$store.commit('setShift', !this.resultSink.$store.state.shiftState)
    this.sendNormalKey(this.keyNode)
  }
}

class QwertySpaceHandler extends TouchActionBase {
  onTouchEnd (pointArray) {
    if (!this.dataIsSent) {
      this.sendSpaceKey(pointArray)
    }
  }
  onRepatTimer (pointArray) {
    this.sendSpaceKey(pointArray)
    this.dataIsSent = true
  }
  sendSpaceKey (pointArray) {
    var lastPoint = pointArray[pointArray.length - 1]
    var moveX = lastPoint.x - pointArray[0].x
    if (moveX < -this.errorPixel) {
      this.sendJsonKey({ command: 'keydown', keyCode: SIPKEY.LEFT })
    } else if (moveX > this.errorPixel) {
      this.sendJsonKey({ command: 'keydown', keyCode: SIPKEY.RIGHT })
    } else {
      this.sendNormalKey(this.keyNode)
    }
  }
}

class SlideChildKeyHandler extends TouchActionBase {
  constructor (keyNode, errorPixel, resultSink) {
    super(keyNode, errorPixel, resultSink)
    this.slideKeyOpened = false
    this.currentPoint = null
  }
  onTouchMove (pointArray) {
    if (this.slideKeyOpened) {
      let targetNode = this.findNearestNode(pointArray[pointArray.length - 1])
      this.resultSink.$store.commit('setSlideFocusNode', targetNode)
    }
  }
  onTouchEnd (pointArray) {
    if (this.slideKeyOpened) {
      let targetNode = this.findNearestNode(pointArray[pointArray.length - 1])
      let keyCode = this.keyNode.defaultKC()
      this.sendJsonKey({ command: 'keydown', keyCode: keyCode, label: targetNode.textContent })
      this.resultSink.$store.commit('setSlideKey', null)
      this.resultSink.$store.commit('setSlideFocusNode', null)
    } else {
      this.sendNormalKey(this.keyNode)
    }
  }
  onRepatTimer (pointArray) {
    if (!this.slideKeyOpened) {
      this.resultSink.$store.commit('setSlideKey', this.keyNode)
      this.slideKeyOpened = true
      this.currentPoint = pointArray[pointArray.length - 1]
      // delayed update visual
      let _this = this
      setTimeout(() => {
        let focusNode = _this.findNearestNode(_this.currentPoint)
        _this.resultSink.$store.commit('setSlideFocusNode', focusNode)
      }, 50)
    }
  }
  findNearestNode (currentPoint) {
    let slideKeyList = document.querySelectorAll('.slidekey')
    let minDiff = -1
    let minIdx = -1
    for (let idx = slideKeyList.length - 1; idx >= 0; --idx) {
      let diffX = slideKeyList[idx].offsetLeft + slideKeyList[idx].offsetWidth / 2 - currentPoint.x
      let diffY = slideKeyList[idx].offsetTop + slideKeyList[idx].offsetHeight / 2 - currentPoint.y
      let diffSqr = diffX * diffX + diffY * diffY
      if (minIdx < 0 || diffSqr < minDiff) {
        minIdx = idx
        minDiff = diffSqr
      }
    }
    return slideKeyList[minIdx]
  }
}

class FlickKeyHandler extends TouchActionBase {
  constructor (keyNode, errorPixel, resultSink) {
    super(keyNode, errorPixel, resultSink)
    this.sqrt05 = 1.0 / Math.sqrt(2.0)
  }
  onTouchMove (pointArray) {
  }
  onTouchEnd (pointArray) {
    let lastPoint = pointArray[pointArray.length - 1]
    let diffX = lastPoint.x - pointArray[0].x
    let diffY = lastPoint.y - pointArray[0].y
    let flickLength = Math.sqrt(diffX * diffX + diffY * diffY)

    if (flickLength < this.errorPixel) {
      this.sendNormalKey(this.keyNode)
      return
    }

    let cosDir = diffX / flickLength
    let sinDir = diffY / flickLength
    let flickIndex = -1
    if (cosDir <= -this.sqrt05) {
      flickIndex = 0
    } else if (cosDir >= this.sqrt05) {
      flickIndex = 2
    } else if (sinDir < 0) {
      flickIndex = 1
    } else {
      flickIndex = 3
    }

    let keyData = this.keyNode.getKeyData()
    let flickArray = kbdData.keydefs[keyData.keyId].flick
    if (!flickArray[flickIndex]) {
      this.sendNormalKey(this.keyNode)
      return
    }
    let flickKC = this.keyNode.flickKeyCode()

    this.sendJsonKey({ command: 'keydown', keyCode: flickKC, label: flickArray[flickIndex] })
  }
  onRepatTimer (pointArray) {
    // TODO: Petal should be drawn
  }
}

export default class ActionMonitor {
  constructor (resultSink) {
    // members
    this.resultSink = resultSink
    this.keyNode = {}
    this.pointArray = []
    this.timeOutCookie = 0
    this.errorPixel = 10
    this.actionHandler = null

    var _this = this
    this._onMouseMove = function (e) { return _this.onMouseMove(e) }
    this._onMouseEnd = function (e) { return _this.onMouseEnd(e) }
    this._onTouchMove = function (e) { return _this.onTouchMove(e) }
    this._onTouchEnd = function (e) { return _this.onTouchEnd(e) }
    this._onTouchCancel = function (e) { return _this.onTouchCancel(e) }
  }
  // touch action handlers
  onMouseStart (e, keyNode) {
    return this.onTouchStart(e, keyNode)
  }
  onMouseMove (e) {
    return this.onTouchMove(e)
  }
  onMouseEnd (e) {
    return this.onTouchEnd(e)
  }
  onTouchStart (e, keyNode) {
    this.resultSink.$store.commit('setFocusKey', keyNode)
    this.keyNode = keyNode
    this.errorPixel = keyNode.$el.clientHeight / 3
    this.setupEventListener()
    this.pointArray.splice(0, this.pointArray.length)
    if ('touches' in e) {
      this.pointArray.push({x: e.touches[0].clientX, y: e.touches[0].clientY})
    } else {
      this.pointArray.push({x: e.clientX, y: e.clientY})
    }
    var _this = this
    this.timeOutCookie = setTimeout(() => { _this.onTimeOut() }, 1000)
    this.actionHandler = this.selectActionHandler()
    if (this.actionHandler) {
      this.actionHandler.onTouchStart(this.pointArray)
    }
  }
  onTouchMove (e) {
    // e.preventDefault()
    e.stopPropagation()
    if ('touches' in e) {
      this.pointArray.push({x: e.touches[0].clientX, y: e.touches[0].clientY})
    } else {
      this.pointArray.push({x: e.clientX, y: e.clientY})
    }
    if (this.actionHandler) {
      this.actionHandler.onTouchMove(this.pointArray)
    }
  }
  onTouchEnd (e) {
    e.preventDefault()
    e.stopPropagation()

    this.onTouchCancel(e)

    if (this.actionHandler) {
      this.actionHandler.onTouchEnd(this.pointArray)
    } else {
      this.resultSink.onKeyPressed(this.keyNode)
    }
  }
  onTouchCancel (e) {
    e.preventDefault()
    e.stopPropagation()
    clearTimeout(this.timeOutCookie)
    this.cleanUpEventListener()
  }
  onTimeOut () {
    if (this.actionHandler) {
      this.actionHandler.onRepatTimer(this.pointArray)
    }
    var _this = this
    this.timeOutCookie = setTimeout(() => { _this.onTimeOut() }, 200)
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
    this.resultSink.$store.commit('setFocusKey', null)
  }
  selectActionHandler (keyNode) {
    let keyData = this.keyNode.getKeyData()
    if (keyData.keyId === 'qw-space') {
      return new QwertySpaceHandler(this.keyNode, this.errorPixel, this.resultSink)
    }
    if (keyData.keyId === 'shift') {
      return new ShiftKeyHandler(this.keyNode, this.errorPixel, this.resultSink)
    }
    let keyDef = kbdData.keydefs[keyData.keyId]
    if ('kc' in keyDef && keyDef.kc === SIPKEY.SWITCH) {
      return new SwitchKeyHandler(this.keyNode, this.errorPixel, this.resultSink)
    }
    if ('slide' in kbdData.keydefs[keyData.keyId]) {
      return new SlideChildKeyHandler(this.keyNode, this.errorPixel, this.resultSink)
    }
    if ('flick' in kbdData.keydefs[keyData.keyId]) {
      return new FlickKeyHandler(this.keyNode, this.errorPixel, this.resultSink)
    }
    return new TouchActionBase(this.keyNode, this.errorPixel, this.resultSink)
  }
}
