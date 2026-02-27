const requestAnimationFrame =
  window.requestAnimationFrame ??
  window.webkitRequestAnimationFrame ??
  window.mozRequestAnimationFrame ??
  ((callback) => window.setTimeout(callback, 1000 / 60))

const cancelAnimationFrame = window.cancelAnimationFrame ?? window.mozCancelAnimationFrame ?? window.clearTimeout

let nowOffset = Date.now()
let now = () => Date.now() - nowOffset
if (window.performance?.now) {
  if (window.performance.timing?.navigationStart) {
    nowOffset = window.performance.timing.navigationStart
  }
  now = () => window.performance.now()
}

function Animator(lagAnimation) {
  this.animations = []
  this.lastAnimatedObjectId = 0
  this.startTime = now()
  this.lastFrameTime = this.startTime
  this.lastFramesTimes = [this.startTime]
  this.animate()
  this.frameTime = now()
  if (lagAnimation) setInterval(this.lagAnimate.bind(this), 200)
  this.lastRaf = 0
}

Animator.prototype.addAnimation = function (animation) {
  this.animations.push(animation)
}

Animator.prototype.animate = function () {
  const frameTime = this.getCurrentTime()
  const delta = frameTime - this.lastFrameTime
  for (const animation of this.animations) {
    animation(frameTime, delta)
  }
  this.lastFrameTime = frameTime
  if (!this.raf) this.raf = requestAnimationFrame(this.rafAnimate.bind(this))
}

Animator.prototype.rafAnimate = function (time) {
  this.frameTime = now()
  this.raf = null
  this.lastRaf = this.frameTime
  this.animate()
}

Animator.prototype.lagAnimate = function () {
  const ctime = now()
  if (ctime - this.lastRaf > 200) {
    this.lastRaf = ctime
    this.frameTime = ctime
    this.animate()
  }
}

Animator.prototype.getCurrentTime = function () {
  return this.frameTime
}

export { Animator, now }
export default Animator
